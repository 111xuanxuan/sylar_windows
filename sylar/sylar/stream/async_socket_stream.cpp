#include "async_socket_stream.h"


namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


	AsyncSocketStream::Ctx::Ctx()
		:sn(0)
		, timeout(0)
		, result(0)
		, timed(false)
		, scheduler(nullptr) {

	}

	void AsyncSocketStream::Ctx::doRsp()
	{
		Scheduler* scd = scheduler.load();

		if (!scheduler.compare_exchange_strong(scd, nullptr)) {
			return;
		}

		SYLAR_LOG_DEBUG(g_logger) << "scd=" << scd << " fiber=" << fiber;

		if (!scd || !fiber) {
			return;
		}

		//如果定时器还存在就取消定时器
		if (timer) {
			timer->cancel();
			timer = nullptr;
		}

		//如果超时，设置超时
		if(timed) {
			result = TIMEOUT;
			resultStr = "timeout";
		}

		//执行协程
		scd->schedule(&fiber);

	}

	AsyncSocketStream::AsyncSocketStream(Socket::ptr sock, bool owner /*= true*/)
		:SocketStream{sock,owner}
		,m_waitSem(2)
		,m_sn(0)
		,m_autoConnect(false)
		,m_tryConnectCount(0)
		,m_iomanager(nullptr)
		,m_worker(nullptr)
	{

	}

	bool AsyncSocketStream::start()
	{
		//初始化io管理器
		if (!m_iomanager) {
			m_iomanager = sylar::IOManager::GetThis();
		}

		//初始化工作器
		if (!m_worker) {
			m_worker = sylar::IOManager::GetThis();
		}

		do {
			//减少两个信号量
			waitFiber();

			//取消定时器
			if (m_timer) {
				m_timer->cancel();
				m_timer = nullptr;
			}

			//如果未连接，重新连接
			if (!isConnected()) {
				//如果连接未成功
				if (!m_socket->reconnect()) {
					//关闭
					innerClose();
					//增加俩个信号量
					m_waitSem.notify();
					m_waitSem.notify();
					break;
				}
			}

			//如果有连接回调
			if (m_connectCb) {
				//如果执行连接回调失败
				if (!m_connectCb(shared_from_this())) {
					//关闭连接
					innerClose();
					m_waitSem.notify();
					m_waitSem.notify();
					break;
				}
			}

			//开始读
			startRead();

			//开始写
			startWrite();

			m_tryConnectCount = 0;
			return true;

		} while (false);
		
		//前面出错，尝试重新连接
		++m_tryConnectCount;

		//如果自动连接
		if (m_autoConnect) {

			//如果有定时器
			if (m_timer) {
				//取消定时器
				m_timer->cancel();
				m_timer = nullptr;
			}

			uint64_t wait_ts = (uint64_t)m_tryConnectCount * 2 * 50;

			if (wait_ts > 2000) {
				wait_ts = 2000;
			}

			//设置定时器，wait_ts时间后执行start
			m_timer = m_iomanager->addTimer(wait_ts,
				std::bind(&AsyncSocketStream::start, shared_from_this()));

		}

		//start失败
		return false;

	}

	void AsyncSocketStream::close()
	{
		//不自动连接
		m_autoConnect = false;
		//切换到新的调度器
		SchedulerSwitcher ss(m_iomanager);

		//取消定时器
		if (m_timer) {
			m_timer->cancel();
		}
		//关闭Socket流
		SocketStream::close();
	}

	void AsyncSocketStream::doRead()
	{
		try
		{
			//如果是连接状态
			while (isConnected())
			{
				recving = true;
				//接受数据
				auto ctx = doRecv();
				recving = false;
				if (ctx) {
					//做出响应
					ctx->doRsp();
				}
			}
		}
		catch (...)
		{

		}

		SYLAR_LOG_DEBUG(g_logger) << "doRead out " << this;
		//关闭连接
		innerClose();
		//唤醒一个
		m_waitSem.notify();

		//如果自动连接
		if (m_autoConnect) {
			m_iomanager->addTimer(10, std::bind(&AsyncSocketStream::start, shared_from_this()));
		}

	}

	void AsyncSocketStream::doWrite()
	{
		try {
			//如果是连接状态
			while (isConnected()) {
				//等待信号量
				m_sem.wait();
				//获取队列中的发送任务
				std::list<SendCtx::ptr> ctxs;
				{
					WriteLock lock(m_queueMutex);
					m_queue.swap(ctxs);
				}
				auto self = shared_from_this();
				for (auto& i : ctxs) {
					//发送数据
					if (!i->doSend(self)) {
						innerClose();
						break;
					}
				}
			}
		}
		catch (...) {
			//TODO log
		}

		SYLAR_LOG_DEBUG(g_logger) << "doWrite out " << this;
		{
			WriteLock lock(m_queueMutex);
			m_queue.clear();
		}

		//唤醒一个信号量
		m_waitSem.notify();
	}

	void AsyncSocketStream::startRead()
	{
		m_iomanager->schedule(std::bind(&AsyncSocketStream::doRead, shared_from_this()));
	}

	void AsyncSocketStream::startWrite()
	{
		m_iomanager->schedule(std::bind(&AsyncSocketStream::doWrite, shared_from_this()));
	}

	void AsyncSocketStream::onTimeOut(Ctx::ptr ctx)
	{
		SYLAR_LOG_DEBUG(g_logger) << "onTimeOut " << ctx;
		{
			WriteLock lock(m_mutex);
			m_ctxs.erase(ctx->sn);
		}
		ctx->timed = true;
		ctx->doRsp();
	}

	sylar::AsyncSocketStream::Ctx::ptr AsyncSocketStream::getCtx(uint32_t sn)
	{
		ReadLock lock(m_mutex);
		auto it = m_ctxs.find(sn);
		return it != m_ctxs.end() ? it->second : nullptr;
	}
	

	sylar::AsyncSocketStream::Ctx::ptr AsyncSocketStream::getAndDelCtx(uint32_t sn)
	{
		Ctx::ptr ctx;
		WriteLock lock(m_mutex);
		auto it = m_ctxs.find(sn);
		if (it != m_ctxs.end()) {
			ctx = it->second;
			m_ctxs.erase(it);
		}
		return ctx;
	}

	bool AsyncSocketStream::addCtx(Ctx::ptr ctx)
	{
		WriteLock lock(m_mutex);
		m_ctxs.insert(std::make_pair(ctx->sn, ctx));
		return true;
	}

	bool AsyncSocketStream::enqueue(SendCtx::ptr ctx)
	{
		SYLAR_ASSERT(ctx);
		WriteLock lock(m_queueMutex);
		bool empty = m_queue.empty();
		m_queue.push_back(ctx);
		lock.unlock();
		if (empty) {
			m_sem.notify();
		}
		return empty;
	}

	bool AsyncSocketStream::innerClose()
	{
		SYLAR_ASSERT(m_iomanager == sylar::IOManager::GetThis());

		//如果是连接状态且有断开连接回调
		if (isConnected() && m_disconnectCb) {
			//执行断开连接回调
			m_disconnectCb(shared_from_this());
		}
		//执行close事件
		onClose();
		//关闭Socket流
		SocketStream::close();
		//唤醒一个在等待的协程
		m_sem.notify();

		std::unordered_map<uint32_t, Ctx::ptr> ctxs;
		{
			WriteLock lock(m_mutex);
			ctxs.swap(m_ctxs);
		}

		{
			WriteLock lock(m_queueMutex);
			m_queue.clear();
		}

		for (auto& i : ctxs) {
			i.second->result = IO_ERROR;
			i.second->resultStr = "io_error";
			//执行每个上下文的响应
			i.second->doRsp();
		}

		return true;

	}

	bool AsyncSocketStream::waitFiber()
	{
		//减少两个信号量
		m_waitSem.wait();
		m_waitSem.wait();
		return true;
	}

	AsyncSocketStreamManager::AsyncSocketStreamManager()
		:m_size{0}
		,m_idx{0}
	{

	}

	void AsyncSocketStreamManager::add(AsyncSocketStream::ptr stream)
	{
		WriteLock lock{ m_mutex };
		m_datas.push_back(stream);
		++m_size;

		if (m_connectCb) {
			stream->setConnectCb(m_connectCb);
		}

		if (m_disconnectCb) {
			stream->setDisconnectCb(m_disconnectCb);
		}

	}

	void AsyncSocketStreamManager::clear()
	{
		WriteLock lock(m_mutex);
		for (auto& i : m_datas) {
			i->close();
		}
		m_datas.clear();
		m_size = 0;
	}

	void AsyncSocketStreamManager::setConnection(const std::vector<AsyncSocketStream::ptr>& streams)
	{
		auto cs = streams;
		WriteLock lock{ m_mutex };
		cs.swap(m_datas);
		m_size = m_datas.size();
		if (m_connectCb || m_disconnectCb) {
			for (auto& i : m_datas) {
				if (m_connectCb) {
					i->setConnectCb(m_connectCb);
				}
				if (m_disconnectCb) {
					i->setDisconnectCb(m_disconnectCb);
				}
			}
		}
		lock.unlock();

		for (auto& i:cs)
		{
			i->close();
		}
	}

	sylar::AsyncSocketStream::ptr AsyncSocketStreamManager::get()
	{
		ReadLock lock(m_mutex);
		for (uint32_t i = 0; i < m_size; ++i) {
			auto idx = m_idx.fetch_add(1);
			if (m_datas[idx % m_size]->isConnected()) {
				return m_datas[idx % m_size];
			}
		}
		return nullptr;
	}

	void AsyncSocketStreamManager::setConnectCb(connect_callback v)
	{
		m_connectCb = v;
		WriteLock lock(m_mutex);
		for (auto& i : m_datas) {
			i->setConnectCb(m_connectCb);
		}
	}

	void AsyncSocketStreamManager::setDisconnectCb(disconnect_callback v)
	{
		m_disconnectCb = v;
		WriteLock lock(m_mutex);
		for (auto& i : m_datas) {
			i->setDisconnectCb(m_disconnectCb);
		}
	}

}

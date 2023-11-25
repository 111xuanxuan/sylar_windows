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

		//�����ʱ�������ھ�ȡ����ʱ��
		if (timer) {
			timer->cancel();
			timer = nullptr;
		}

		//�����ʱ�����ó�ʱ
		if(timed) {
			result = TIMEOUT;
			resultStr = "timeout";
		}

		//ִ��Э��
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
		//��ʼ��io������
		if (!m_iomanager) {
			m_iomanager = sylar::IOManager::GetThis();
		}

		//��ʼ��������
		if (!m_worker) {
			m_worker = sylar::IOManager::GetThis();
		}

		do {
			//���������ź���
			waitFiber();

			//ȡ����ʱ��
			if (m_timer) {
				m_timer->cancel();
				m_timer = nullptr;
			}

			//���δ���ӣ���������
			if (!isConnected()) {
				//�������δ�ɹ�
				if (!m_socket->reconnect()) {
					//�ر�
					innerClose();
					//���������ź���
					m_waitSem.notify();
					m_waitSem.notify();
					break;
				}
			}

			//��������ӻص�
			if (m_connectCb) {
				//���ִ�����ӻص�ʧ��
				if (!m_connectCb(shared_from_this())) {
					//�ر�����
					innerClose();
					m_waitSem.notify();
					m_waitSem.notify();
					break;
				}
			}

			//��ʼ��
			startRead();

			//��ʼд
			startWrite();

			m_tryConnectCount = 0;
			return true;

		} while (false);
		
		//ǰ�����������������
		++m_tryConnectCount;

		//����Զ�����
		if (m_autoConnect) {

			//����ж�ʱ��
			if (m_timer) {
				//ȡ����ʱ��
				m_timer->cancel();
				m_timer = nullptr;
			}

			uint64_t wait_ts = (uint64_t)m_tryConnectCount * 2 * 50;

			if (wait_ts > 2000) {
				wait_ts = 2000;
			}

			//���ö�ʱ����wait_tsʱ���ִ��start
			m_timer = m_iomanager->addTimer(wait_ts,
				std::bind(&AsyncSocketStream::start, shared_from_this()));

		}

		//startʧ��
		return false;

	}

	void AsyncSocketStream::close()
	{
		//���Զ�����
		m_autoConnect = false;
		//�л����µĵ�����
		SchedulerSwitcher ss(m_iomanager);

		//ȡ����ʱ��
		if (m_timer) {
			m_timer->cancel();
		}
		//�ر�Socket��
		SocketStream::close();
	}

	void AsyncSocketStream::doRead()
	{
		try
		{
			//���������״̬
			while (isConnected())
			{
				recving = true;
				//��������
				auto ctx = doRecv();
				recving = false;
				if (ctx) {
					//������Ӧ
					ctx->doRsp();
				}
			}
		}
		catch (...)
		{

		}

		SYLAR_LOG_DEBUG(g_logger) << "doRead out " << this;
		//�ر�����
		innerClose();
		//����һ��
		m_waitSem.notify();

		//����Զ�����
		if (m_autoConnect) {
			m_iomanager->addTimer(10, std::bind(&AsyncSocketStream::start, shared_from_this()));
		}

	}

	void AsyncSocketStream::doWrite()
	{
		try {
			//���������״̬
			while (isConnected()) {
				//�ȴ��ź���
				m_sem.wait();
				//��ȡ�����еķ�������
				std::list<SendCtx::ptr> ctxs;
				{
					WriteLock lock(m_queueMutex);
					m_queue.swap(ctxs);
				}
				auto self = shared_from_this();
				for (auto& i : ctxs) {
					//��������
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

		//����һ���ź���
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

		//���������״̬���жϿ����ӻص�
		if (isConnected() && m_disconnectCb) {
			//ִ�жϿ����ӻص�
			m_disconnectCb(shared_from_this());
		}
		//ִ��close�¼�
		onClose();
		//�ر�Socket��
		SocketStream::close();
		//����һ���ڵȴ���Э��
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
			//ִ��ÿ�������ĵ���Ӧ
			i.second->doRsp();
		}

		return true;

	}

	bool AsyncSocketStream::waitFiber()
	{
		//���������ź���
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

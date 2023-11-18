#include "iomanager.h"



namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	//用于通知事件
	static constexpr ULONG_PTR INTERNAL_NOTIFY_KEY = 1; 


	sylar::IOManager::FdContext::EventContext::ptr IOManager::FdContext::getContext(Event event)
	{
		switch (event)
		{
		case IOManager::READ:
			return read;
		case IOManager::WRITE:
			return write;
		default:
			SYLAR_ASSERT2(false, "getContext");
		}
		throw std::invalid_argument("getContext invalid event");
	}


	void IOManager::FdContext::resetContext(Event event)
	{
		switch (event)
		{
		case IOManager::READ:
			read.reset();
			return;
		case IOManager::WRITE:
			write.reset();
			return ;
		default:
			SYLAR_ASSERT2(false, "resetContext");
		}
		throw std::invalid_argument("resetContext invalid event");
	}

	void IOManager::FdContext::triggerEvent(Event event)
	{
		if (SYLAR_UNLIKELY(!(events & event))) {
			SYLAR_LOG_ERROR(g_logger) << "fd=" << fd
				<< " triggerEvent event=" << event
				<< " events=" << events
				<< "\nbacktrace:\n"
				<< sylar::BacktraceToString(100, 2, "    ");
			return;
		}

		events = (Event)(events & ~event);

		FdContext::EventContext::ptr ctx = getContext(event);

		if (ctx->cb) {
			ctx->scheduler->schedule(&ctx->cb);
		}
		else {
			ctx->scheduler->schedule(&ctx->fiber);
		}

		resetContext(event);

		return;
	}


	IOManager::IOManager(size_t threads /*= 1*/, bool use_caller /*= true*/, const std::string& name /*= ""*/) :Scheduler(threads, use_caller, name)
	{
		//创建iocp
		m_iocpfd = (uintptr_t)CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

		SYLAR_ASSERT(m_iocpfd);

		start();
		
	}

	IOManager::~IOManager()
	{
		stop();

		CloseHandle((HANDLE)m_iocpfd);

		for (auto it=m_fdContexts.begin();it!=m_fdContexts.end();++it)
		{
			delete m_fdContexts.erase(it)->second;
		}

	}

	int IOManager::addEvent(uintptr_t fd, Event event,FdContext::EventContext::ptr event_ctx, std::function<void()> cb /*= nullptr*/)
	{
		FdContext* fd_ctx = nullptr;
		BOOL isNew = false;
		ReadLock lock{ m_mutex };
		fd_ctx = m_fdContexts[fd];

		if (fd_ctx) {
			lock.unlock();
		}
		else
		{
			lock.unlock();

			isNew = TRUE;
			fd_ctx = new FdContext(fd);

			WriteLock lock2{ m_mutex };
			m_fdContexts[fd] = fd_ctx;
		}

		std::lock_guard lock2{ fd_ctx->mutex };

		if (SYLAR_UNLIKELY(fd_ctx->events & event)) {
			SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
				<< " event=" << event
				<< " fd_ctx.event=" << fd_ctx->events;
			SYLAR_ASSERT(!(fd_ctx->events & event));
		}

		fd_ctx->events = (Event)(fd_ctx->events | event);

		if (event & READ) {
			fd_ctx->read = event_ctx;
		}else if (event	& WRITE){
			fd_ctx->write = event_ctx;
		}


		SYLAR_ASSERT(!event_ctx->scheduler&& !event_ctx->fiber&& !event_ctx->cb);

		event_ctx->scheduler = Scheduler::GetThis();

		if (cb) {
			event_ctx->cb.swap(cb);
		}else{
			event_ctx->fiber = Fiber::GetThis();
			SYLAR_ASSERT2(event_ctx->fiber->getState() == Fiber::EXEC
				, "state=" << event_ctx->fiber->getState());
		}

		if(isNew)
		CreateIoCompletionPort((HANDLE)fd, (HANDLE)m_iocpfd, (ULONG_PTR)fd_ctx, 0);

		++m_pendingEventCount;

		return 0;
	}

	bool IOManager::delEvent(uintptr_t fd, Event event)
	{
		FdContext* fd_ctx = nullptr;
		ReadLock lock{ m_mutex };
		fd_ctx = m_fdContexts[fd];
		lock.unlock();

		if (!fd_ctx) {
			return false;
		}


		FdContext::Lock lock2{ fd_ctx->mutex };

		if (SYLAR_UNLIKELY(!(fd_ctx->events & event))) {
			return false;
		}
;
		Event new_events = (Event)(fd_ctx->events & ~event);

		// 重置该fd对应的event事件上下文
		fd_ctx->events = new_events;
		// 待执行事件数减1
		--m_pendingEventCount;

		fd_ctx->resetContext(event);

		return true;
	}

	bool IOManager::cancelEvent(uintptr_t fd, Event event)
	{
		ReadLock lock{ m_mutex };
		FdContext* fd_ctx = m_fdContexts[fd];
		lock.unlock();

		if (!fd_ctx) {
			return false;
		}


		FdContext::Lock lock2{ fd_ctx->mutex };
		if (SYLAR_UNLIKELY(!(fd_ctx->events & event))) {
			return false;
		}

		// 如果可能，取消特定的I/O操作
		if (event & READ) {
			// 如果是读事件，取消读操作
			CancelIoEx((HANDLE)fd, (LPOVERLAPPED)fd_ctx->read.get());
		}
		else if (event & WRITE) {
			// 如果是写事件，取消写操作
			CancelIoEx((HANDLE)fd, (LPOVERLAPPED)fd_ctx->write.get());
		}

		Event new_events = (Event)(fd_ctx->events & ~event);
		fd_ctx->events = new_events;
		--m_pendingEventCount;

		fd_ctx->triggerEvent(event);

		return true;

	}

	bool IOManager::cancelAll(uintptr_t fd)
	{
		FdContext* fd_ctx = nullptr;
		ReadLock lock{ m_mutex };
		fd_ctx = m_fdContexts[fd];
		lock.unlock();

		if (!fd_ctx) {
			return false;
		}

		FdContext::Lock lock2{ fd_ctx->mutex };

		if (SYLAR_UNLIKELY(!(fd_ctx->events))) {
			return false;
		}

		CancelIoEx((HANDLE)fd, nullptr);

		if (fd_ctx->events & READ) {
			fd_ctx->triggerEvent(READ);
			--m_pendingEventCount;
		}

		if (fd_ctx->events & WRITE) {
			fd_ctx->triggerEvent(WRITE);
			--m_pendingEventCount;
		}

		fd_ctx->events = NONE;

		SYLAR_ASSERT(fd_ctx->events == 0);
		return true;

	}

	IOManager* IOManager::GetThis()
	{
		return dynamic_cast<IOManager*>(Scheduler::GetThis());
	}

	void IOManager::tickle()
	{
		if (!hasIdleThreads()) {
			return;
		}

		BOOL rt = PostQueuedCompletionStatus((HANDLE)m_iocpfd, 0, INTERNAL_NOTIFY_KEY, nullptr);

		SYLAR_ASSERT(rt);
	}

	bool IOManager::stopping()
	{
		uint64_t timeout = 0;
		return stopping(timeout);
	}

	bool IOManager::stopping(uint64_t& timeout)
	{
		timeout = getNextTimer();
		return timeout == ~0ull
			&& m_pendingEventCount == 0
			&& Scheduler::stopping();
	}

	void IOManager::idle()
	{
		SYLAR_LOG_DEBUG(g_logger) << "idle";

		DWORD dwTrans;
		ULONG_PTR completion_key;
		FdContext::EventContext* overlapped;
		FdContext* fd_ctx;

		while (true)
		{
			uint64_t next_timeout = 0;
			if (SYLAR_UNLIKELY(stopping(next_timeout))) {
				SYLAR_LOG_INFO(g_logger) << "name=" << getName()
					<< " idle stopping exit";
				break;
			}


			BOOL rt = 0;
			int erro_code;
			do {
				static const int MAX_TIMEOUT = 3000;
				if (next_timeout != ~0ull) {
					next_timeout = (int)next_timeout > MAX_TIMEOUT
						? MAX_TIMEOUT : next_timeout;
				}
				else {
					next_timeout = MAX_TIMEOUT;
				}
				rt = GetQueuedCompletionStatus((HANDLE)m_iocpfd, &dwTrans, &completion_key, (LPOVERLAPPED*)&overlapped, next_timeout);
				if (!rt ) {

					erro_code = WSAGetLastError();

					if (erro_code == WSA_WAIT_TIMEOUT)
						break;
				}
				else {
					break;
				}
			} while (true);
			

			std::vector<std::function<void()>> cbs;

			listExpiredCb(cbs);

			if (!cbs.empty()) {
				//SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
				schedule(cbs.begin(), cbs.end());
				cbs.clear();
			}

			if ((rt)&&(completion_key != INTERNAL_NOTIFY_KEY)) {

				fd_ctx = (FdContext*)completion_key;
				FdContext::Lock lock{ fd_ctx->mutex };

				if ( (fd_ctx->events & READ)&&(fd_ctx->read.get() ==overlapped)) {
					fd_ctx->triggerEvent(READ);
					--m_pendingEventCount;
				}
				else if ((fd_ctx->events & WRITE)&&(fd_ctx->write.get()== overlapped)) {
					fd_ctx->triggerEvent(WRITE);
					--m_pendingEventCount;
				}

			}


			Fiber::ptr cur = Fiber::GetThis();
			auto raw_ptr = cur.get();
			cur.reset();

			raw_ptr->swapOut();
		}

	}

	void IOManager::onTimerInsertedAtFront()
	{
		tickle();
	}

}
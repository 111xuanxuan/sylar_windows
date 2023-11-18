#include "fd_manager.h"


namespace sylar {


	FdCtx::FdCtx(uintptr_t fd)
		:m_isInit(false)
		, m_isSocket(false)
		, m_sysNonblock(false)
		, m_userNonblock(false)
		, m_isClosed(false)
		, m_fd(fd)
		, m_recvTimeout(-1)
		, m_sendTimeout(-1){
		init();
	}

	FdCtx::~FdCtx() {

	}

	void FdCtx::setTimeout(int type, uint64_t v)
	{
		if (type == SO_RCVTIMEO) {
			m_recvTimeout = v;
		}
		else
		{
			m_sendTimeout = v;
		}
	}

	uint64_t FdCtx::getTimeout(int type)
	{
		if (type == SO_RCVTIMEO) {
			return m_recvTimeout;
		}
		else {
			return m_sendTimeout;
		}
	}

	static bool isSocket(SOCKET s) {
		char optval;
		int optlen = sizeof(optval);

		if (getsockopt(s, SOL_SOCKET, SO_TYPE, &optval, &optlen) == SOCKET_ERROR) {
			//使用之前必须初始化网络
			if (WSAGetLastError() == WSAENOTSOCK) {
				return false; 
			}
		}
		return true; 
	}

	bool FdCtx::init()
	{
		if (m_isInit) {
			return true;
		}
		m_recvTimeout = -1;
		m_sendTimeout = -1;

		DWORD flag;
		if (!GetHandleInformation((HANDLE)m_fd,&flag)) {
			m_isInit = false;
			m_isSocket = false;
		}
		else {
			m_isInit = true;
			m_isSocket = sylar::isSocket((SOCKET)m_fd);
		}

		if (m_isSocket) {
			u_long	mode = 1;
			//设置套接字为非阻塞
			ioctlsocket((SOCKET)m_fd, FIONBIO,&mode);
			m_sysNonblock = true;
		}
		else {
			m_sysNonblock = false;
		}

		m_userNonblock = false;
		m_isClosed = false;
		return m_isInit;

	}

	FdManager::FdManager()
	{

	}

	sylar::FdCtx::ptr FdManager::get(uintptr_t fd, bool auto_create /*= false*/)
	{
		if (fd == -1) {
			return nullptr;
		}

		ReadLock lock{ m_mutex };
		FdCtx::ptr fd_ctx = m_datas[fd];

		if (!fd_ctx) {
			if (!auto_create) {
				return nullptr;
			}
		}
		else
		{
			if (fd_ctx || !auto_create) {
				return fd_ctx;
			}
		}

		lock.unlock();

		WriteLock lock2{ m_mutex };
		fd_ctx = std::make_shared<FdCtx>(fd);

		m_datas[fd] = fd_ctx;

		return fd_ctx;
	}

	void FdManager::del(uintptr_t fd)
	{
		WriteLock lock{ m_mutex };

		FdCtx::ptr ctx = m_datas[fd];

		if (!ctx) {
			return;
		}

		m_datas[fd].reset();
	}

}
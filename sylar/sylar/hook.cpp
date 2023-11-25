#include "hook.h"
#include "log.h"
#include "config.h"
#include "fiber.h"
#include "iomanager.h"
#include "fd_manager.h"
#include "util.h"


sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


namespace sylar {

	static sylar::ConfigVar<uint64_t>::ptr g_tcp_connect_timeout = sylar::Config::Lookup("tcp.connect.timeout", (uint64_t)5000, "tcp connect timeout");

	static thread_local bool t_hook_enable = false;


#define HOOK_FUN(XX) \
	XX(Sleep,sleep)	\
	XX(getsockopt,getsockopt)	\
	XX(setsockopt,setsockopt)	\
	XX(WSASocketA, socket)		\
	XX(WSAConnect, connect)		\
	XX(closesocket, closesocket)	\
	XX(CloseHandle,closeHandle)	\
	XX(AcceptEx, accept)	\
	XX(WSARecv, recv)	\
	XX(WSARecvFrom, recvfrom)	\
	XX(WSASend, send)	\
	XX(WSASendTo, sendto)
	//XX(WSAIoctl, ioctl)

	bool hook_init() {
		static bool is_inited = false;

		if (is_inited) {
			return true;
		}

		//#define XX(name,f_name) f_name ## _f = (f_name ## _fun)GetProcAddress(module,#name);
#define XX(name,f_name)  f_name ## _f = (f_name ## _fun)(&##name);
		HOOK_FUN(XX);
#undef  XX

		return true;
	}

	static uint64_t s_connect_timeout = -1;

	struct _HookIniter {
		WSADATA wsData;
		_HookIniter() :wsData{0} {

			WSAStartup(MAKEWORD(2, 2), &wsData);

			hook_init();

			s_connect_timeout = g_tcp_connect_timeout->getValue();

			g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value) {
				SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from "
					<< old_value << " to " << new_value;
				s_connect_timeout = new_value;
				});
		}

		~_HookIniter() {
			WSACleanup();

		}
	};

	static _HookIniter s_hook_initer;

	bool is_hook_enable()
	{
		return t_hook_enable;
	}

	void set_hook_enable(bool flag)
	{
		t_hook_enable = flag;
	}

	struct timer_info {
		int cancelled = 0;
	};

	template<std::size_t N, typename T, typename... Args>
	typename std::enable_if<(N == 1), T>::type
		get_arg(T&& first, Args&&... args) {
		return std::forward<T>(first);
	}


	template<std::size_t N, typename T, typename... Args>
	typename std::enable_if<(N > 1), decltype(get_arg<N-1>(std::declval<Args>()...)) >::type
		get_arg(T&& first, Args&& ... args) {
			return get_arg<N - 1>(std::forward<Args>(args)...);
	}


	template<typename OriginFun, typename... Args>
	static SSIZE_T do_socket(SOCKET fd, OriginFun fun, const char* hook_fun_name,typename IOManager::Event event, int timeout_so,IOManager::FdContext::EventContext::ptr eventCtx ,Args&&...args) {


		//获取句柄上下文
		typename FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);

		//如果句柄关闭了
		if (ctx->isClose()) {
			errno = EBADF;
			return 0;
		}

		//如果句柄不是socket||用户主动设置非阻塞
		if (!ctx->isSocket() ) {
			return 0;
		}

		//获取超时时间
		uint64_t to = ctx->getTimeout(timeout_so);
		std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();

		SSIZE_T n = fun(fd, std::forward<Args>(args)...);

		if (n == 0) {

			WSABUF* wsabufs = get_arg<1>(std::forward<Args>(args)...);
			DWORD dwBufferCount = get_arg<2>(std::forward<Args>(args)...);

			DWORD num = 0;
			
			for (int i=0;i<dwBufferCount;++i)
			{
				num += wsabufs[i].len;
			}
			return num;
		}

		//如果失败
		if (n== SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			return 0;
		}

		//获取io管理器
		IOManager* iom = IOManager::GetThis();
		Timer::ptr timer;
		std::weak_ptr<timer_info> winfo(tinfo);

		//如果设置的超时时间
		if (to != (uint64_t)-1) {
			//添加to时间的条件定时器，如果to时间内执行完成，则winfo所指对象被销毁，否则取消这个事件
			timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
				auto t = winfo.lock();
				//已经完成或者取消事件
				if (!t || t->cancelled) {
					return;
				}
				//超时
				t->cancelled = ETIMEDOUT;
				//取消事件
				iom->cancelEvent(fd, event);
			}, winfo);
		}

		//添加事件,设为当前协程
		int rt = iom->addEvent(fd, event, eventCtx);

		//如果添加失败了
		if (SYLAR_UNLIKELY(rt)) {
			SYLAR_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
				<< fd << ", " << event << ")";
			//取消定时器
			if (timer) {
				timer->cancel();
			}
			return 0;
		}
		else {
			//让出执行权
			sylar::Fiber::YieldToHold();

			if (timer) {
				timer->cancel();
			}

			//如果事件被取消了
			if (tinfo->cancelled) {
				errno = tinfo->cancelled;
				return 0;
			}

		}

		return eventCtx->dwDataLength;
	}
	
#define XX(name,f_name) f_name ## _fun f_name ## _f = nullptr;
	HOOK_FUN(XX);
#undef  XX

	void s_sleep(DWORD Milliseconds) {

		if (!t_hook_enable) {
			return sleep_f(Milliseconds);
		}

		Fiber::ptr fiber = Fiber::GetThis();
		IOManager* iom = IOManager::GetThis();
		iom->addTimer(Milliseconds, std::bind((void(Scheduler::*)(Fiber::ptr, int thread)) & IOManager::schedule, iom, fiber, -1));
		Fiber::YieldToHold();
	}

	//创建socket
	SOCKET s_socket(int af, int type, int protocol) {

		if (!t_hook_enable) {
			return socket_f(af, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
		}

		//创建不阻塞socket
		uintptr_t fd = socket_f(af, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);

		if (fd == INVALID_SOCKET) {
			SYLAR_LOG_INFO(g_logger) << WSAGetLastError();
			return fd;
		}

		FdMgr::GetInstance()->get(fd, true);

		return fd;
	}

	INT connect_with_timeout(SOCKET fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {

		FdCtx::ptr ctx = FdMgr::GetInstance()->get(fd);

		if (!ctx || ctx->isClose()) {
			errno = EBADF;
			return 0;
		}

		int n= connect(fd, addr, addrlen);

		if (n == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
			return n;
		}

		fd_set wfds;
		FD_ZERO(&wfds);
		FD_SET(fd, &wfds);
		timeval tv = { timeout_ms/1000, timeout_ms%1000 };
		int rt=select(0, nullptr, &wfds, nullptr, &tv);
		if (rt != 1)
		{
			SYLAR_LOG_INFO(g_logger) << "can not connect to server";
			return 1;
		}

		return 0;

	}

	int s_connect(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen) {
		return connect_with_timeout(sockfd, addr, addrlen, s_connect_timeout);
	}

	BOOL s_accept(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived)
	{
		auto ectx = IOManager::CreateEventContext();
		//获取句柄上下文
		typename FdCtx::ptr ctx = FdMgr::GetInstance()->get(sListenSocket);

		//如果句柄关闭了
		if (ctx->isClose()) {
			errno = EBADF;
			return false;
		}

		//如果句柄不是socket||用户主动设置非阻塞
		if (!ctx->isSocket()) {
			return false;
		}

		//获取超时时间
		uint64_t to = ctx->getTimeout(SO_RCVTIMEO);
		std::shared_ptr<timer_info> tinfo = std::make_shared<timer_info>();

		BOOL n = AcceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived,(LPOVERLAPPED)ectx.get() );

		if (n ) {
			return true;
		}

		//如果失败
		if (!n  && WSAGetLastError() != WSA_IO_PENDING)
		{
			
			return false;
		}

		//获取io管理器
		IOManager* iom = IOManager::GetThis();
		Timer::ptr timer;
		std::weak_ptr<timer_info> winfo(tinfo);

		IOManager::Event event = IOManager::READ;
		uintptr_t fd = sListenSocket;

		//如果设置的超时时间
		if (to != (uint64_t)-1) {
			//添加to时间的条件定时器，如果to时间内执行完成，则winfo所指对象被销毁，否则取消这个事件
			timer = iom->addConditionTimer(to, [winfo, fd, iom, event](){
				auto t = winfo.lock();
				//已经完成或者取消事件
				if (!t || t->cancelled) {
					return;
				}
				//超时
				t->cancelled = ETIMEDOUT;
				//取消事件
				iom->cancelEvent(fd, event);
			}, winfo);
		}

		//添加事件,设为当前协程
		int rt = iom->addEvent(fd, event,ectx);

		//如果添加失败了
		if (SYLAR_UNLIKELY(rt)) {
			SYLAR_LOG_ERROR(g_logger)  << "acceptEx addEvent("
				<< fd << ", " << event << ")";
			//取消定时器
			if (timer) {
				timer->cancel();
			}
			return false;
		}
		else {
			//让出执行权
			sylar::Fiber::YieldToHold();

			if (timer) {
				timer->cancel();
			}

			//如果事件被取消了
			if (tinfo->cancelled) {
				errno = tinfo->cancelled;
				return false;
			}

		}

		return true;

	}

	DWORD s_recv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD  lpNumberOfBytesRecvd,DWORD flags)
	{
		auto ctx = IOManager::CreateEventContext();
		return do_socket(s, recv_f, "recv", IOManager::READ, SO_RCVTIMEO, ctx,lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, &flags, (LPOVERLAPPED)ctx.get(), nullptr);
	}

	DWORD s_recvfrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, sockaddr* lpFrom, LPINT lpFromlen, LPDWORD  lpNumberOfBytesRecvd, DWORD flags)
	{
		auto ctx = IOManager::CreateEventContext();
		return do_socket(s, recvfrom_f, "recvfrom", IOManager::READ, SO_RCVTIMEO,ctx ,lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, &flags, lpFrom, lpFromlen, (LPOVERLAPPED)ctx.get(), nullptr);
	}

	DWORD s_send(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,LPDWORD  lpNumberOfBytesSent,DWORD flags)
	{
		auto ctx = IOManager::CreateEventContext();
		return do_socket(s, send_f, "send", IOManager::READ, SO_RCVTIMEO,ctx ,lpBuffers, dwBufferCount, lpNumberOfBytesSent, flags, (LPOVERLAPPED)ctx.get(), nullptr);
	}

	DWORD s_sendto(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,  const sockaddr* lpTo, int iTolen, LPDWORD  lpNumberOfBytesSent, DWORD flags)
	{
		auto ctx = IOManager::CreateEventContext();
		return  do_socket(s, sendto_f, "recvfrom", IOManager::READ, SO_RCVTIMEO, ctx, lpBuffers, dwBufferCount, lpNumberOfBytesSent, flags, lpTo, iTolen, (LPOVERLAPPED)ctx.get(), nullptr);
	}

	BOOL s_closesocket(SOCKET s)
	{
		if (!sylar::t_hook_enable) {
			return closesocket_f(s)==0;
		}

		FdCtx::ptr ctx = FdMgr::GetInstance()->get(s);

		if (ctx) {
			auto iom = sylar::IOManager::GetThis();
			if (iom) {
				iom->cancelAll(s);
			} 
			sylar::FdMgr::GetInstance()->del(s);
		}

		return closesocket_f(s) == 0;

	}

	BOOL s_closeHandle(HANDLE s)
	{
		if (!sylar::t_hook_enable) {
			return closeHandle_f(s) == 0;
		}

		FdCtx::ptr ctx = FdMgr::GetInstance()->get((uintptr_t)s);

		if (ctx) {
			auto iom = sylar::IOManager::GetThis();
			if (iom) {
				iom->cancelAll((uintptr_t)s);
			}
			sylar::FdMgr::GetInstance()->del((uintptr_t)s);
		}

		return closeHandle_f(s);
	}

	BOOL s_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen)
	{
		return getsockopt_f(s, level, optname, optval, optlen)==0;
	}

	int s_setsockopt(SOCKET s, int level, int optname, char* optval, int optlen)
	{
		if (!sylar::t_hook_enable) {
			return setsockopt_f(s, level, optname, optval, optlen);
		}

		if (level == SOL_SOCKET) {
			if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
				FdCtx::ptr ctx = FdMgr::GetInstance()->get(s);
				if (ctx) {
					const timeval* v = (const timeval*)optval;
					ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
				}
			}
		}

		return setsockopt_f(s, level, optname, optval, optlen) ;

	}

}
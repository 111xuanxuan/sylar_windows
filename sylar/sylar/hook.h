#pragma once
#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include "util.h"
#include <MSWSock.h>
#pragma  comment(lib,"mswsock.lib")

namespace sylar {
	/**
	* @brief 当前线程是否hook
	*/
	bool is_hook_enable();

	/**
	 * @brief 设置当前线程的hook状态
	 */
	void set_hook_enable(bool flag);

	using sleep_fun = decltype(+Sleep);
	using socket_fun = decltype(+WSASocketA);
	using connect_fun = decltype(+WSAConnect);
	using accept_fun = decltype(+AcceptEx);
	using recv_fun = decltype(+WSARecv);
	using recvfrom_fun = decltype(+WSARecvFrom);
	using recvmsg_fun = int(WINAPI*)(SOCKET s, LPWSAMSG lpMsg, LPDWORD lpdwNumberOfBytesRecvd, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
	using send_fun = decltype(+WSASend);
	using sendto_fun = decltype(+WSASendTo);
	using sendmsg_fun = int (WSAAPI*)(SOCKET Handle, LPWSAMSG lpMsg, DWORD  dwFlags, LPDWORD lpNumberOfBytesSent, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
	using closesocket_fun = decltype(+closesocket);
	using closeHandle_fun = decltype(+CloseHandle);
	//using ioctl_fun = decltype(+WSAIoctl);
	using getsockopt_fun = decltype(+getsockopt);
	using setsockopt_fun = decltype(+setsockopt);

	//原函数
	extern sleep_fun sleep_f;
	extern socket_fun socket_f;
	extern connect_fun connect_f;
	extern accept_fun accept_f;
	extern recv_fun recv_f;
	extern recvfrom_fun recvfrom_f;
	//extern recvmsg_fun recvmsg_f;
	extern send_fun send_f;
	extern sendto_fun sendto_f;
	//extern sendmsg_fun sendmsg_f;
	extern closesocket_fun closesocket_f;
	extern closeHandle_fun closeHandle_f;
	//extern ioctl_fun ioctl_f;
	extern getsockopt_fun getsockopt_f;
	extern setsockopt_fun setsockopt_f;

	//睡眠
	void s_sleep(DWORD Milliseconds);

	//创建socket
	SOCKET s_socket(int af, int type, int protocol = 0);

	int connect_with_timeout(SOCKET fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

	int s_connect(SOCKET sockfd, const struct sockaddr* addr, socklen_t addrlen);

	BOOL s_accept(SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD  lpdwBytesReceived);

	BOOL s_recv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD  lpNumberOfBytesRecvd, DWORD flags);

	BOOL s_recvfrom(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, sockaddr* lpFrom, LPINT lpFromlen, LPDWORD  lpNumberOfBytesRecvd, DWORD flags);

	BOOL s_send(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD  lpNumberOfBytesSent, DWORD flags);

	BOOL s_sendto(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, const sockaddr* lpTo, int iTolen, LPDWORD  lpNumberOfBytesSent, DWORD flags);

	BOOL s_closesocket(SOCKET s);

	BOOL s_closeHandle(HANDLE s);

	BOOL s_getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);

	int s_setsockopt(SOCKET s, int level, int optname, char* optval, int optlen);


}
#endif
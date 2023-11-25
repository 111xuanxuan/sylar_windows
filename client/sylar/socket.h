#pragma once
#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include "util.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")
#include "address.h"

namespace sylar {

	/*
 * Socket类
 */
	class Socket :public std::enable_shared_from_this<Socket> {
	public:
		using ptr = std::shared_ptr<Socket>;
		using weak_ptr = std::weak_ptr<Socket>;
		Socket(Socket&) = delete;
		Socket& operator=(Socket&) = delete;

		//Socket类型
		enum  Type {
			TCP = SOCK_STREAM,
			UDP = SOCK_DGRAM
		};

		//Socket协议
		enum  Family {
			IPv4 = PF_INET,
			IPv6 = PF_INET6,
			UNIX = PF_UNIX
		};

		/*创建TCP Socket*/
		static Socket::ptr CreateTCP(Address::ptr address);

		/*创建UDP Socket*/
		static Socket::ptr CreateUDP(Address::ptr address);

		/*创建IPv4的TCP Socket*/
		static Socket::ptr CreateTCPSocket();

		/*创建IPv4的UDP Socket*/
		static Socket::ptr CreateUDPSocket();

		/*创建IPv6的TCP Socket*/
		static Socket::ptr CreateTCPSocket6();

		/*创建IPv6的UDP Socket*/
		static Socket::ptr CreateUDPSocket6();

		/*创建Unix的TCP Socket*/
		static Socket::ptr CreateUnixTCPSocket();

		/*创建Unix的UDP Socket*/
		static Socket::ptr CreateUnixUDPSocket();

		/*创建Socket*/
		Socket(int family, int type, int protocol = 0);

		virtual ~Socket();

		/*获取发送超时时间(毫秒)*/
		int64_t getSendTimeout();

		/*设置发送超时时间(毫秒)*/
		void setSendTimeout(int64_t v);

		/*获取接收超时时间(毫秒)*/
		int64_t getRecvTimeout();

		/*设置接收超时时间(毫秒)*/
		void setRecvTimeout(int64_t v);

		/*获取sockopt*/
		bool getOption(int level, int option, void* result, socklen_t* len);

		/*获取sockopt模板*/
		template<typename T>
		bool getOption(int level, int option, T& result) {
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		/*设置sockopt*/
		bool setOption(int level, int option, const void* result, socklen_t len);

		/*设置sockopt模板*/
		template<typename T>
		bool setOption(int level, int option, const T& value) {
			return setOption(level, option, &value, sizeof(T));
		}

		/*绑定地址*/
		virtual bool bind(const Address::ptr addr);

		/*连接地址*/
		virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

		virtual bool reconnect(uint64_t timeout_ms = -1);

		/*关闭Socket*/
		virtual bool close();

		/*
		发送数据
		返回>0发送成功对应大小数据
		=0连接断开
		<0发送失败
		*/
		virtual int send(const void* buffer, size_t length, int flags = 0);

		virtual int send(const WSABUF* buffers, size_t length, int flags = 0);

		/*
		发送数据
		返回>0发送成功对应大小数据
		=0连接断开
		<0发送失败
		*/
		virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

		virtual int sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags = 0);

		/*
		接受数据
		返回>0接受成功对应大小数据
		=0连接断开
		<0接受失败
		*/
		virtual int recv(void* buffer, size_t length, int flags = 0);

		virtual int recv(WSABUF* buffers, size_t length, int flags = 0);

		/*
		接受数据
		返回>0接受成功对应大小数据
		=0连接断开
		<0接受失败
		*/
		virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

		virtual int recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags = 0);

		/*获取本地地址*/
		Address::ptr getLocalAddress();

		/*获取远端地址*/
		Address::ptr getRemoteAddress();

		/*获取协议簇*/
		int getFamily() const { return m_family; }

		/*获取类型*/
		int getType() const { return m_type; }

		/*获取协议*/
		int getProtocol() const { return m_protocol; }

		/*返回是否连接*/
		bool isConnected() const { return m_isConnected; }

		bool checkConnected();

		/*是否有效*/
		bool isValid() const;

		/*获取Socket错误*/
		int getError();

		/*输出信息到流中*/
		virtual std::ostream& dump(std::ostream& os) const;

		virtual std::string toString() const;

		/*返回Socket句柄*/
		SOCKET getSocket() const { return m_sock; }


	protected:

		/*初始化Socket*/
		void initSock();

		/*创建Socket*/
		void newSock();

		/*初始化sock*/
		virtual bool init(size_t sock);

	protected:

		SOCKET m_sock; //Socket句柄
		int m_family; //Socket协议族
		int m_type; //Socket类型
		int m_protocol; //Socket协议
		bool m_isConnected; //是否连接
		Address::ptr m_localAddress; //本地地址
		Address::ptr m_remoteAddress;//远端地址
	};



	class SSLSocket :public Socket {
	public:
		using ptr = std::shared_ptr<SSLSocket>;

		static SSLSocket::ptr CreateTCP(sylar::Address::ptr address);
		static SSLSocket::ptr CreateTCPSocket();
		static SSLSocket::ptr CreateTCPSocket6();

		SSLSocket(int family, int type, int protocol = 0);
		virtual bool bind(const Address::ptr addr) override;
		virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
		virtual bool close() override;
		virtual int send(const void* buffer, size_t length, int flags = 0) override;
		virtual int send(const WSABUF* buffers, size_t length, int flags = 0) override;
		virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
		virtual int sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags = 0) override;
		virtual int recv(void* buffer, size_t length, int flags = 0) override;
		virtual int recv(WSABUF* buffers, size_t length, int flags = 0) override;
		virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
		virtual int recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags = 0) override;

		bool loadCertificates(const std::string& cert_file, const std::string& key_file);
		virtual std::ostream& dump(std::ostream& os) const override;
	protected:
		virtual bool init(size_t sock) override;
	private:
		std::shared_ptr<SSL_CTX> m_ctx;
		std::shared_ptr<SSL> m_ssl;
	};


	std::ostream& operator<<(std::ostream& os, const Socket& sock);


}
#endif
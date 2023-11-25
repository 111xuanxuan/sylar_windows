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
 * Socket��
 */
	class Socket :public std::enable_shared_from_this<Socket> {
	public:
		using ptr = std::shared_ptr<Socket>;
		using weak_ptr = std::weak_ptr<Socket>;
		Socket(Socket&) = delete;
		Socket& operator=(Socket&) = delete;

		//Socket����
		enum  Type {
			TCP = SOCK_STREAM,
			UDP = SOCK_DGRAM
		};

		//SocketЭ��
		enum  Family {
			IPv4 = PF_INET,
			IPv6 = PF_INET6,
			UNIX = PF_UNIX
		};

		/*����TCP Socket*/
		static Socket::ptr CreateTCP(Address::ptr address);

		/*����UDP Socket*/
		static Socket::ptr CreateUDP(Address::ptr address);

		/*����IPv4��TCP Socket*/
		static Socket::ptr CreateTCPSocket();

		/*����IPv4��UDP Socket*/
		static Socket::ptr CreateUDPSocket();

		/*����IPv6��TCP Socket*/
		static Socket::ptr CreateTCPSocket6();

		/*����IPv6��UDP Socket*/
		static Socket::ptr CreateUDPSocket6();

		/*����Unix��TCP Socket*/
		static Socket::ptr CreateUnixTCPSocket();

		/*����Unix��UDP Socket*/
		static Socket::ptr CreateUnixUDPSocket();

		/*����Socket*/
		Socket(int family, int type, int protocol = 0);

		virtual ~Socket();

		/*��ȡ���ͳ�ʱʱ��(����)*/
		int64_t getSendTimeout();

		/*���÷��ͳ�ʱʱ��(����)*/
		void setSendTimeout(int64_t v);

		/*��ȡ���ճ�ʱʱ��(����)*/
		int64_t getRecvTimeout();

		/*���ý��ճ�ʱʱ��(����)*/
		void setRecvTimeout(int64_t v);

		/*��ȡsockopt*/
		bool getOption(int level, int option, void* result, socklen_t* len);

		/*��ȡsockoptģ��*/
		template<typename T>
		bool getOption(int level, int option, T& result) {
			socklen_t length = sizeof(T);
			return getOption(level, option, &result, &length);
		}

		/*����sockopt*/
		bool setOption(int level, int option, const void* result, socklen_t len);

		/*����sockoptģ��*/
		template<typename T>
		bool setOption(int level, int option, const T& value) {
			return setOption(level, option, &value, sizeof(T));
		}

		/*�󶨵�ַ*/
		virtual bool bind(const Address::ptr addr);

		/*���ӵ�ַ*/
		virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

		virtual bool reconnect(uint64_t timeout_ms = -1);

		/*�ر�Socket*/
		virtual bool close();

		/*
		��������
		����>0���ͳɹ���Ӧ��С����
		=0���ӶϿ�
		<0����ʧ��
		*/
		virtual int send(const void* buffer, size_t length, int flags = 0);

		virtual int send(const WSABUF* buffers, size_t length, int flags = 0);

		/*
		��������
		����>0���ͳɹ���Ӧ��С����
		=0���ӶϿ�
		<0����ʧ��
		*/
		virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);

		virtual int sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags = 0);

		/*
		��������
		����>0���ܳɹ���Ӧ��С����
		=0���ӶϿ�
		<0����ʧ��
		*/
		virtual int recv(void* buffer, size_t length, int flags = 0);

		virtual int recv(WSABUF* buffers, size_t length, int flags = 0);

		/*
		��������
		����>0���ܳɹ���Ӧ��С����
		=0���ӶϿ�
		<0����ʧ��
		*/
		virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);

		virtual int recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags = 0);

		/*��ȡ���ص�ַ*/
		Address::ptr getLocalAddress();

		/*��ȡԶ�˵�ַ*/
		Address::ptr getRemoteAddress();

		/*��ȡЭ���*/
		int getFamily() const { return m_family; }

		/*��ȡ����*/
		int getType() const { return m_type; }

		/*��ȡЭ��*/
		int getProtocol() const { return m_protocol; }

		/*�����Ƿ�����*/
		bool isConnected() const { return m_isConnected; }

		bool checkConnected();

		/*�Ƿ���Ч*/
		bool isValid() const;

		/*��ȡSocket����*/
		int getError();

		/*�����Ϣ������*/
		virtual std::ostream& dump(std::ostream& os) const;

		virtual std::string toString() const;

		/*����Socket���*/
		SOCKET getSocket() const { return m_sock; }


	protected:

		/*��ʼ��Socket*/
		void initSock();

		/*����Socket*/
		void newSock();

		/*��ʼ��sock*/
		virtual bool init(size_t sock);

	protected:

		SOCKET m_sock; //Socket���
		int m_family; //SocketЭ����
		int m_type; //Socket����
		int m_protocol; //SocketЭ��
		bool m_isConnected; //�Ƿ�����
		Address::ptr m_localAddress; //���ص�ַ
		Address::ptr m_remoteAddress;//Զ�˵�ַ
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
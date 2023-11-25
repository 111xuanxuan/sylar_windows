 #pragma once
#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include "util.h"
#include <map>
#include <afunix.h>


namespace sylar {

	class IPAddress;

	/*
 * �����ַ����
 */
	class Address {
	public:
		using ptr = std::shared_ptr<Address>;

		//����sockaddr����Address
		static Address::ptr Create(const sockaddr* addr, int len);

		//����host���ض�Ӧ����������Address
		static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//ͨ��host���ض�Ӧ����������һ��Address
		static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//ͨ��host��ַ���ض�Ӧ����������IPAddress
		static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//��ȡ��������������<����������ַ����������λ��>
		static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

		//��ȡָ�������ĵ�ַ����������λ��
		static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family = AF_INET);

		virtual ~Address() {}

		int getFamily() const;

		//����sockaddrָ��,ֻ��
		virtual const sockaddr* getAddr() const = 0;

		virtual sockaddr* getAddr() = 0;

		virtual socklen_t getAddrLen() const = 0;

		//�ɶ��������ַ
		virtual std::ostream& insert(std::ostream& os) const = 0;

		//���ؿɶ����ַ���
		std::string toString() const;

		bool operator<(const Address& rhs) const;

		bool operator==(const Address& rhs) const;

		bool operator!=(const Address& rhs) const;

	};

	/*
	 * IP��ַ����
	 */
	class IPAddress :public Address {
	public:
		using ptr = std::shared_ptr<IPAddress>;

		//ͨ��������IP��������������IPAddress
		static IPAddress::ptr Create(const char* address, uint16_t port = 0);

		//��ȡ�õ�ַ�Ĺ㲥��ַ
		virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

		//��ȡ�õ�ַ������
		virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

		//��ȡ�õ�ַ����������
		virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

		//���ض˿ں�
		virtual uint32_t getPort() const = 0;

		//���ö˿ں�
		virtual void setPort(uint16_t v) = 0;
	};


	/*
	 * IPv4��ַ��
	 */

	class IPv4Address :public IPAddress {
	public:
		using ptr = std::shared_ptr<IPv4Address>;

		//ʹ�õ��ʮ���Ƶ�ַ����IPv4Address
		static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

		//ͨ��sockaddr_in����IPv4Address
		IPv4Address(const sockaddr_in& address);

		//ͨ��2���Ƶ�ַ�Ͷ˿ںŴ���IPv4Address
		IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);


		//��ȡֻ����sockaddr_inָ��
		const sockaddr* getAddr() const override;

		//��ȡ�ɶ�д��sockaddr_inָ��
		sockaddr* getAddr()override;

		//��ȡsockaddr_in����
		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

		//��ȡ�㲥��ַ
		IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

		//��ȡ����
		IPAddress::ptr networdAddress(uint32_t prefix_len) override;

		//��ȡ��������
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		//��ȡ�˿ں�
		uint32_t getPort() const override;

		//���ö˿ں�
		void setPort(uint16_t v) override;
	private:
		sockaddr_in m_addr;

	};

	/*
	 * IPv6��ַ��
	 */
	class IPv6Address :public IPAddress {

	public:
		using ptr = std::shared_ptr<IPv6Address>;

		//ʹ�õ��ʮ���Ƶ�ַ����IPv6Address
		static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

		//Ĭ�Ϲ��캯��
		IPv6Address();

		//ͨ��sockaddr_in6����IPv6Address
		IPv6Address(const sockaddr_in6& address);

		//ͨ��2���Ƶ�ַ�Ͷ˿ںŴ���IPv6Address
		IPv6Address(const uint8_t address[16], uint16_t port = 0);

		//��ȡֻ����sockaddr_in6ָ��
		const sockaddr* getAddr() const override;

		//��ȡ�ɶ�д��sockaddr_in6ָ��
		sockaddr* getAddr()override;

		//��ȡsockaddr_in6����
		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

		//��ȡ�㲥��ַ
		IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

		//��ȡ����
		IPAddress::ptr networdAddress(uint32_t prefix_len) override;

		//��ȡ��������
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		//��ȡ�˿ں�
		uint32_t getPort() const override;

		//���ö˿ں�
		void setPort(uint16_t v) override;

	private:
		//windows ipv6��ַ�ṹ��
		sockaddr_in6 m_addr;
	};


	/*
	 * Unix��ַ��
	 */
	class UnixAddress :public Address {
	public:
		using ptr = std::shared_ptr<UnixAddress>;

		//Ĭ�Ϲ��캯��
		UnixAddress();

		//ͨ��·������UnixAddress
		UnixAddress(const std::string& path);

		//��ȡֻ����sockaddr_unָ��
		const sockaddr* getAddr() const override;

		//��ȡ�ɶ�д��sockaddr_unָ��
		sockaddr* getAddr()override;

		//��ȡsockaddr_un����
		socklen_t getAddrLen() const override;

		//����sockaddr_un����
		void setAddrLen(uint32_t v);

		//��ȡ·��
		std::string getPath() const;

		//����·��
		std::ostream& insert(std::ostream& os) const override;

	private:
		sockaddr_un m_addr;
		int m_length;

	};


	/*
	 * δ֪��ַ��
	 */
	class UnknownAddress :public Address {
	public:
		using ptr = std::shared_ptr<UnknownAddress>;

		UnknownAddress(int family);

		UnknownAddress(const sockaddr& addr);

		const sockaddr* getAddr() const override;

		sockaddr* getAddr()override;

		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

	private:
		sockaddr m_addr;
	};

	std::ostream& operator<<(std::ostream& os, const Address& addr);



}
#endif
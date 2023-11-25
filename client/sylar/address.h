 #pragma once
#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include "util.h"
#include <map>
#include <afunix.h>


namespace sylar {

	class IPAddress;

	/*
 * 网络地址基类
 */
	class Address {
	public:
		using ptr = std::shared_ptr<Address>;

		//根据sockaddr创建Address
		static Address::ptr Create(const sockaddr* addr, int len);

		//根据host返回对应条件的所有Address
		static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//通过host返回对应条件的任意一个Address
		static Address::ptr LookupAny(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//通过host地址返回对应条件的任意IPAddress
		static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

		//获取本机所有网卡的<网卡名，地址，子网掩码位数>
		static bool GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family = AF_INET);

		//获取指定网卡的地址和子网掩码位数
		static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family = AF_INET);

		virtual ~Address() {}

		int getFamily() const;

		//返回sockaddr指针,只读
		virtual const sockaddr* getAddr() const = 0;

		virtual sockaddr* getAddr() = 0;

		virtual socklen_t getAddrLen() const = 0;

		//可读性输出地址
		virtual std::ostream& insert(std::ostream& os) const = 0;

		//返回可读性字符串
		std::string toString() const;

		bool operator<(const Address& rhs) const;

		bool operator==(const Address& rhs) const;

		bool operator!=(const Address& rhs) const;

	};

	/*
	 * IP地址基类
	 */
	class IPAddress :public Address {
	public:
		using ptr = std::shared_ptr<IPAddress>;

		//通过域名，IP，服务器名创建IPAddress
		static IPAddress::ptr Create(const char* address, uint16_t port = 0);

		//获取该地址的广播地址
		virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

		//获取该地址的网段
		virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

		//获取该地址的子网掩码
		virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

		//返回端口号
		virtual uint32_t getPort() const = 0;

		//设置端口号
		virtual void setPort(uint16_t v) = 0;
	};


	/*
	 * IPv4地址类
	 */

	class IPv4Address :public IPAddress {
	public:
		using ptr = std::shared_ptr<IPv4Address>;

		//使用点分十进制地址创建IPv4Address
		static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

		//通过sockaddr_in创建IPv4Address
		IPv4Address(const sockaddr_in& address);

		//通过2进制地址和端口号创建IPv4Address
		IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);


		//获取只读的sockaddr_in指针
		const sockaddr* getAddr() const override;

		//获取可读写的sockaddr_in指针
		sockaddr* getAddr()override;

		//获取sockaddr_in长度
		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

		//获取广播地址
		IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

		//获取网段
		IPAddress::ptr networdAddress(uint32_t prefix_len) override;

		//获取子网掩码
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		//获取端口号
		uint32_t getPort() const override;

		//设置端口号
		void setPort(uint16_t v) override;
	private:
		sockaddr_in m_addr;

	};

	/*
	 * IPv6地址类
	 */
	class IPv6Address :public IPAddress {

	public:
		using ptr = std::shared_ptr<IPv6Address>;

		//使用点分十进制地址创建IPv6Address
		static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

		//默认构造函数
		IPv6Address();

		//通过sockaddr_in6创建IPv6Address
		IPv6Address(const sockaddr_in6& address);

		//通过2进制地址和端口号创建IPv6Address
		IPv6Address(const uint8_t address[16], uint16_t port = 0);

		//获取只读的sockaddr_in6指针
		const sockaddr* getAddr() const override;

		//获取可读写的sockaddr_in6指针
		sockaddr* getAddr()override;

		//获取sockaddr_in6长度
		socklen_t getAddrLen() const override;

		std::ostream& insert(std::ostream& os) const override;

		//获取广播地址
		IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;

		//获取网段
		IPAddress::ptr networdAddress(uint32_t prefix_len) override;

		//获取子网掩码
		IPAddress::ptr subnetMask(uint32_t prefix_len) override;

		//获取端口号
		uint32_t getPort() const override;

		//设置端口号
		void setPort(uint16_t v) override;

	private:
		//windows ipv6地址结构体
		sockaddr_in6 m_addr;
	};


	/*
	 * Unix地址类
	 */
	class UnixAddress :public Address {
	public:
		using ptr = std::shared_ptr<UnixAddress>;

		//默认构造函数
		UnixAddress();

		//通过路径构造UnixAddress
		UnixAddress(const std::string& path);

		//获取只读的sockaddr_un指针
		const sockaddr* getAddr() const override;

		//获取可读写的sockaddr_un指针
		sockaddr* getAddr()override;

		//获取sockaddr_un长度
		socklen_t getAddrLen() const override;

		//设置sockaddr_un长度
		void setAddrLen(uint32_t v);

		//获取路径
		std::string getPath() const;

		//设置路径
		std::ostream& insert(std::ostream& os) const override;

	private:
		sockaddr_un m_addr;
		int m_length;

	};


	/*
	 * 未知地址类
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
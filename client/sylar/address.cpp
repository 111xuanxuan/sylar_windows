#include "address.h"
#include <sstream>
#include <stddef.h>
#include "endia.h"
#include <iphlpapi.h>
#pragma comment(lib,"iphlpapi.lib")

namespace sylar {

	template<typename T>
	static T CreateMask(uint32_t bits) {
		return (1 << (sizeof(T) * 8 - bits)) - 1;
	}

	template<typename T>
	static uint32_t CountBytes(T value) {
		uint32_t result = 0;
		for (; value; ++result) {
			value &= value - 1;
		}
		return result;
	}




	static uint32_t MaskStringToInt32(std::string mask) {

		std::stringstream ss(mask);
		std::string part;

		uint32_t resut = 0;
		int cnt = 3;
		while (std::getline(ss, part, '.'))
		{
			int partValue = std::stoi(part);

			if (partValue < 0 || partValue > 255 || cnt < -1) {
				return 0;
			}

			resut |= partValue << (cnt-- * 8);
		}

		return resut;
	}

	Address::ptr Address::Create(const sockaddr* addr, int len)
	{
		if (addr == nullptr) {
			return nullptr;
		}

		Address::ptr result;

		switch (addr->sa_family)
		{
		case AF_INET:
			result = std::make_shared<IPv4Address>(*(const sockaddr_in*)addr);
			break;
		case AF_INET6:
			result = std::make_shared<IPv6Address>(*(const sockaddr_in6*)addr);
			break;
		default:
			result = std::make_shared<UnknownAddress>(*addr);
			break;
		}

		return result;
	}

	bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host, int family, int type, int protocol)
	{
		addrinfo hints, * results, * next;
		hints.ai_flags = 0;
		hints.ai_family = family;
		hints.ai_socktype = type;
		hints.ai_protocol = protocol;
		hints.ai_addrlen = 0;
		hints.ai_canonname = nullptr;
		hints.ai_addr = nullptr;
		hints.ai_next = nullptr;

		std::string node;
		const char* service = nullptr;

		//¼ì²éipv6µØÖ·
		if (!host.empty() && host[0] == '[') {
			const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
			if (endipv6) {
				if (*(endipv6 + 1) == ':') {
					service = endipv6 + 2;
				}
				node = host.substr(1, endipv6 - host.c_str() - 1);
			}
		}

		if (node.empty()) {
			service = (const char*)memchr(host.c_str(), ':', host.size());
			if (service) {
				if (!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
					node = host.substr(0, service - host.c_str());
					++service;
				}
			}
		}

		if (node.empty()) {
			node = host;
		}

		int error = getaddrinfo(node.c_str(), service, &hints, &results);

		if (error) {
			return false;
		}

		next = results;
		while (next) {
			result.push_back(Create(next->ai_addr, next->ai_addrlen));
			next = next->ai_next;
		}

		freeaddrinfo(results);
		return !result.empty();
	}

	Address::ptr Address::LookupAny(const std::string& host, int family, int type, int protocol)
	{
		std::vector<Address::ptr> result;
		if (Lookup(result, host, family, type, protocol)) {
			return result[0];
		}
		return nullptr;
	}

	std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host, int family, int type, int protocol)
	{
		std::vector<Address::ptr> result;
		if (Lookup(result, host, family, type, protocol)) {
			for (auto& i : result) {
				IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
				if (v) {
					return v;
				}
			}
		}
		return nullptr;
	}

	bool Address::GetInterfaceAddresses(std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result, int family)
	{
		std::multimap<std::string, std::string> masks;

		ULONG bufferLen = 0;

		{
			PIP_ADAPTER_INFO info = nullptr;
			PIP_ADAPTER_INFO adapter = nullptr;

			ULONG ret = GetAdaptersInfo(info, &bufferLen);
			info = reinterpret_cast<PIP_ADAPTER_INFO>(new BYTE[bufferLen]);
			ret = GetAdaptersInfo(info, &bufferLen);

			if (ret != ERROR_SUCCESS) {
				delete info;
				return false;
			}

			adapter = info;
			while (adapter)
			{
				if (adapter->Type == MIB_IF_TYPE_LOOPBACK)
					continue;

				masks.insert({ adapter->AdapterName,adapter->IpAddressList.IpMask.String });

				adapter = adapter->Next;
			}

			delete info;
		}

		bufferLen = 0;
		PIP_ADAPTER_ADDRESSES addr = nullptr;
		PIP_ADAPTER_ADDRESSES adapter = nullptr;

		ULONG ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &bufferLen);
		addr = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(new BYTE[bufferLen]);
		ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, nullptr, addr, &bufferLen);


		if (NO_ERROR != ret) {
			delete addr;
			return false;
		}

		adapter = addr;
		char szIp[256]{ 0 };
		while (adapter)
		{

			std::string name{ adapter->AdapterName };

			decltype(masks)::const_iterator it = masks.find(name);

			if (it == masks.end()) {
				adapter = adapter->Next;
				continue;
			}

			PIP_ADAPTER_UNICAST_ADDRESS Addr = adapter->FirstUnicastAddress;

			Address::ptr address;
			uint32_t maskBytes;

			while (Addr)
			{
				if (Addr->Address.lpSockaddr->sa_family == family) {

					if (Addr->Address.lpSockaddr->sa_family == AF_INET) {
						inet_ntop(PF_INET, &((sockaddr_in*)Addr->Address.lpSockaddr)->sin_addr, szIp, sizeof(szIp));
						address = Create(Addr->Address.lpSockaddr, sizeof(sockaddr_in));
					}
					else if (Addr->Address.lpSockaddr->sa_family == AF_INET6)
					{
						inet_ntop(PF_INET6, &((sockaddr_in6*)Addr->Address.lpSockaddr)->sin6_addr, szIp, sizeof(szIp));
						address = Create(Addr->Address.lpSockaddr, sizeof(sockaddr_in6));
					}

					maskBytes = CountBytes(MaskStringToInt32(it->second));

				}
					
				Addr = Addr->Next;
			}

			result.insert({ name,{address,maskBytes} });

			adapter = adapter->Next;
		}

		delete addr;

		return true;

	}

	bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t>>& result, const std::string& iface, int family)
	{
		if (iface.empty() || iface == "*") {
			if (family == AF_INET || family == AF_UNSPEC) {
				result.push_back(std::make_pair(std::make_shared<IPv4Address>(), 0u));
			}
			if (family == AF_INET6 || family == AF_UNSPEC) {
				result.push_back(std::make_pair(std::make_shared<IPv6Address>(), 0u));
			}
			return true;
		}

		std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;

		if (!GetInterfaceAddresses(results, family)) {
			return false;
		}

		auto its = results.equal_range(iface);

		for (; its.first != its.second; ++its.first) {
			result.push_back(its.first->second);
		}

		return !result.empty();

	}

	int Address::getFamily() const
	{
		return getAddr()->sa_family;
	}

	std::string Address::toString() const
	{
		std::stringstream ss;
		insert(ss);

		return ss.str();
	}

	bool Address::operator<(const Address& rhs) const
	{
		int len = min(getAddrLen(), rhs.getAddrLen());
		int result = memcmp(getAddr(), rhs.getAddr(), len);

		if (result < 0) {
			return true;
		}
		else if (result > 0) {
			return false;
		}
		else if (getAddrLen() < rhs.getAddrLen()) {
			return true;
		}

	}

	bool Address::operator==(const Address& rhs) const
	{
		return getAddrLen() == rhs.getAddrLen()
			&& memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
	}

	bool Address::operator!=(const Address& rhs) const
	{
		return !(*this == rhs);
	}

	IPAddress::ptr IPAddress::Create(const char* address, uint16_t port)
	{
		addrinfo hints, * results;
		memset(&hints, 0, sizeof(addrinfo));

		hints.ai_flags = AI_NUMERICHOST;
		hints.ai_family = AF_UNSPEC;

		int error = getaddrinfo(address, nullptr, &hints, &results);
		if (error) {
			return nullptr;
		}

		try {
			IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
				Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen)
				);

			if (result) {
				result->setPort(port);
			}
			freeaddrinfo(results);
			return result;
		}
		catch (...) {
			freeaddrinfo(results);
			return nullptr;
		}
	}

	IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port)
	{
		IPv4Address::ptr p = std::make_shared<IPv4Address>();
		p->m_addr.sin_port = byteswapOnLittleEndian(port);
		int result = inet_pton(AF_INET, address, &p->m_addr.sin_addr);

		if (result <= 0) {
			return nullptr;
		}

		return p;
	}

	IPv4Address::IPv4Address(const sockaddr_in& address)
	{
		m_addr = address;
	}

	IPv4Address::IPv4Address(uint32_t address, uint16_t port)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = byteswapOnLittleEndian(port);
		m_addr.sin_addr.S_un.S_addr = byteswapOnLittleEndian(address);
	}

	const sockaddr* IPv4Address::getAddr() const
	{
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	sockaddr* IPv4Address::getAddr()
	{
		return  reinterpret_cast<sockaddr*>(&m_addr);
	}

	socklen_t IPv4Address::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::ostream& IPv4Address::insert(std::ostream& os) const
	{
		uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.S_un.S_addr);

		os << ((addr >> 24) & 0xff) << "."
			<< ((addr >> 16) & 0xff) << "."
			<< ((addr >> 8) & 0xff) << "."
			<< (addr & 0xff);

		os << ":" << byteswapOnLittleEndian(m_addr.sin_port);
		return os;
	}

	IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len)
	{
		if (prefix_len > 32) {
			return nullptr;
		}

		sockaddr_in baddr(m_addr);
		baddr.sin_addr.S_un.S_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(baddr);
	}

	IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len)
	{
		if (prefix_len > 32) {
			return nullptr;
		}

		sockaddr_in baddr(m_addr);
		baddr.sin_addr.S_un.S_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(baddr);
	}

	IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in subnet{ 0 };
		subnet.sin_family = AF_INET;
		subnet.sin_addr.S_un.S_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
		return std::make_shared<IPv4Address>(subnet);
	}

	uint32_t IPv4Address::getPort() const
	{
		return byteswapOnLittleEndian(m_addr.sin_port);
	}

	void IPv4Address::setPort(uint16_t v)
	{
		m_addr.sin_port = byteswapOnLittleEndian(v);
	}

	IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port)
	{
		IPv6Address::ptr rt = std::make_shared<IPv6Address>();
		rt->m_addr.sin6_port =byteswapOnLittleEndian(port);
		int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
		if (result <= 0) {
			return nullptr;
		}
		return rt;
	}

	IPv6Address::IPv6Address()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin6_family = AF_INET6;
	}

	IPv6Address::IPv6Address(const sockaddr_in6& address)
	{
		m_addr = address;
	}

	IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sin6_family = AF_INET6;
		m_addr.sin6_port =byteswapOnLittleEndian(port);
		memcpy(&m_addr.sin6_addr, address, 16);
	}

	const sockaddr* IPv6Address::getAddr() const
	{
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	sockaddr* IPv6Address::getAddr()
	{
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	socklen_t IPv6Address::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::ostream& IPv6Address::insert(std::ostream& os) const
	{
		os << "[";
		const uint16_t* addr = reinterpret_cast<const uint16_t*>(&m_addr.sin6_addr);
		bool used_zeros = false;
		for (size_t i = 0; i < 8; ++i)
		{
			if (addr[i] == 0 && !used_zeros) {
				continue;
			}
			if (i && addr[i - 1] == 0 && !used_zeros) {
				os << ":";
				used_zeros = true;
			}
			if (i) {
				os << ":";
			}
			os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
		}

		if (!used_zeros && addr[7] == 0) {
			os << "::";
		}

		os << "]" << byteswapOnLittleEndian(m_addr.sin6_port);
		return os;
	}

	IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len)
	{
		sockaddr_in6 baddr(m_addr);
		baddr.sin6_addr.u.Byte[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);

		for (int i = prefix_len / 8 + 1; i < 16; ++i)
			baddr.sin6_addr.u.Byte[i] = 0xff;

		return std::make_shared<IPv6Address>(baddr);
	}

	IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len)
	{
		sockaddr_in6 baddr(m_addr);
		baddr.sin6_addr.u.Byte[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
		for (int i = prefix_len / 8 + 1; i < 16; ++i) {
			baddr.sin6_addr.u.Byte[i] = 0x00;
		}
		return std::make_shared<IPv6Address>(baddr);
	}

	IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len)
	{
		sockaddr_in6 subnet{ 0 };
		subnet.sin6_family = AF_INET6;
		subnet.sin6_addr.u.Byte[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);

		for (uint32_t i = 0; i < prefix_len / 8; ++i)
		{
			subnet.sin6_addr.u.Byte[i] = 0xff;
		}
		return std::make_shared<IPv6Address>(subnet);
	}

	uint32_t IPv6Address::getPort() const
	{
		return byteswapOnLittleEndian(m_addr.sin6_port);
	}

	void IPv6Address::setPort(uint16_t v)
	{
		m_addr.sin6_port = byteswapOnLittleEndian(v);
	}


	static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un*)0)->sun_path) - 1;

	UnixAddress::UnixAddress()
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sun_family = AF_UNIX;
		m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
	}

	UnixAddress::UnixAddress(const std::string& path)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sun_family = AF_UNIX;
		m_length = path.size() + 1;

		if (!path.empty() && path[0] == '\0') {
			--m_length;
		}

		if (m_length > sizeof(m_addr.sun_path)) {
			throw std::logic_error("path too long");
		}

		memcpy(m_addr.sun_path, path.c_str(), m_length);
		m_length += offsetof(sockaddr_un, sun_path);
	}

	const sockaddr* UnixAddress::getAddr() const
	{
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	sockaddr* UnixAddress::getAddr()
	{
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	socklen_t UnixAddress::getAddrLen() const
	{
		return m_length;
	}

	void UnixAddress::setAddrLen(uint32_t v)
	{
		m_length = v;
	}

	std::string UnixAddress::getPath() const
	{
		std::stringstream ss;
		if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {

			ss << "\\0" << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);

		}
		else
		{
			ss << m_addr.sun_path;
		}

		return ss.str();
	}

	std::ostream& UnixAddress::insert(std::ostream& os) const
	{
		if (m_length > offsetof(sockaddr_un, sun_path)
			&& m_addr.sun_path[0] == '\0') {
			return os << "\\0" << std::string(m_addr.sun_path + 1,
				m_length - offsetof(sockaddr_un, sun_path) - 1);
		}
		return os << m_addr.sun_path;
	}

	UnknownAddress::UnknownAddress(int family)
	{
		memset(&m_addr, 0, sizeof(m_addr));
		m_addr.sa_family = family;
	}

	UnknownAddress::UnknownAddress(const sockaddr& addr)
	{
		m_addr = addr;
	}

	const sockaddr* UnknownAddress::getAddr() const
	{
		return reinterpret_cast<const sockaddr*>(&m_addr);
	}

	sockaddr* UnknownAddress::getAddr()
	{
		return reinterpret_cast<sockaddr*>(&m_addr);
	}

	socklen_t UnknownAddress::getAddrLen() const
	{
		return sizeof(m_addr);
	}

	std::ostream& UnknownAddress::insert(std::ostream& os) const
	{
		os << "[UnknownAddress family=" << m_addr.sa_family << "]";
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const Address& addr)
	{
		return addr.insert(os);
	}


}
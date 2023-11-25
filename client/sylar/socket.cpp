#include "socket.h"

namespace sylar {
	static WSADATA s_wsaData;
	struct SocktInit {
		SocktInit() {
			WSAStartup(MAKEWORD(2, 2), &s_wsaData);
		}

		~SocktInit() {
			WSACleanup();
		}
	};

	static SocktInit s_initer ;

	Socket::ptr Socket::CreateTCP(Address::ptr address)
	{
		return std::make_shared<Socket>(address->getFamily(), TCP, 0);
	}

	Socket::ptr Socket::CreateUDP(Address::ptr address)
	{
		auto sock = std::make_shared < Socket >(address->getFamily(), UDP, 0);
		sock->newSock();
		sock->m_isConnected = true;
		return sock;
	}

	Socket::ptr Socket::CreateTCPSocket()
	{
		return std::make_shared<Socket>(IPv4, TCP, 0);
	}

	Socket::ptr Socket::CreateUDPSocket()
	{
		auto sock = std::make_shared<Socket>(IPv4, UDP, 0);
		sock->newSock();
		sock->m_isConnected = true;
		return sock;
	}

	Socket::ptr Socket::CreateTCPSocket6()
	{
		return std::make_shared<Socket>(IPv6, TCP, 0);
	}

	Socket::ptr Socket::CreateUDPSocket6()
	{
		auto sock = std::make_shared<Socket>(IPv6, UDP, 0);
		sock->newSock();
		sock->m_isConnected = true;
		return sock;
	}

	Socket::ptr Socket::CreateUnixTCPSocket()
	{
		return std::make_shared<Socket>(UNIX, TCP, 0);
	}

	Socket::ptr Socket::CreateUnixUDPSocket()
	{
		return std::make_shared<Socket>(UNIX, UDP, 0);
	}

	Socket::Socket(int family, int type, int protocol)
		:m_sock{ (SOCKET)-1 }
		, m_family{ family }
		, m_type{ type }
		, m_protocol{ protocol }
		, m_isConnected{ false }
	{
	}

	Socket::~Socket()
	{
		close();
	}

	bool Socket::checkConnected()
	{
		int error_code;
		int error_code_size = sizeof(error_code);
		int result = getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&error_code, &error_code_size);
		if (result == 0) {
			m_isConnected = true;
		}
		else {
			m_isConnected = false;
		}

		return m_isConnected;
	}

	bool Socket::isValid() const
	{
		return m_sock != -1;
	}

	int Socket::getError()
	{
		int error = 0;
		socklen_t len = sizeof(error);
		if (!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
			error = errno;
		}
		return error;
	}

	bool Socket::getOption(int level, int option, void* result, socklen_t* len) {
		return ::getsockopt(m_sock, level, option, (char*)result, len)==0;
	}

	bool Socket::setOption(int level, int option, const void* result, socklen_t len) {
		return ::setsockopt(m_sock, level, option, (const char*)result, len)==0;
	}

	std::ostream& Socket::dump(std::ostream& os) const
	{
		os << "[Socket sock=" << m_sock
			<< " is_connected=" << m_isConnected
			<< " family=" << m_family
			<< " type=" << m_type
			<< " protocol=" << m_protocol;
		if (m_localAddress) {
			os << " local_address=" << m_localAddress->toString();
		}
		if (m_remoteAddress) {
			os << " remote_address=" << m_remoteAddress->toString();
		}
		os << "]";
		return os;
	}

	std::string Socket::toString() const
	{
		std::stringstream ss;
		dump(ss);
		return ss.str();
	}

	 
	void Socket::setRecvTimeout(int64_t v)
	{
		struct timeval tv { (int)v / 1000, (int)(v % 1000 * 1000) };

		setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
	}




	bool Socket::bind(const Address::ptr addr)
	{
		if (!isValid())
		{
			newSock();

			if (!isValid())
			{
				return false;
			}
		}

		if (addr->getFamily() != m_family) {
			return false;
		}

		UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);

		if (uaddr) {

			Socket::ptr sock = Socket::CreateUnixTCPSocket();

			if (sock->connect(addr)) {
				return false;
			}
			else
			{
				FSUtil::Unlink(uaddr->getPath(), true);
			}
		}

		if (::bind(m_sock, addr->getAddr(), addr->getAddrLen()))
		{
			return false;
		}

		getLocalAddress();

		return true;
	}

	bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
	{
		m_remoteAddress = addr;

		if (!isValid()) {
			newSock();
			if (!isValid()) {
				return false;
			}
		}

		if (addr->getFamily() != m_family)
		{
			return false;
		}

		if (::connect(m_sock,addr->getAddr(),addr->getAddrLen())) {
			close();
			return false;
		}

		m_isConnected = true;
		getRemoteAddress();
		getLocalAddress();
		return true;

	}

	bool Socket::reconnect(uint64_t timeout_ms /*= -1*/)
	{
		if (!m_remoteAddress) {
			return false;
		}
		m_localAddress.reset();
		return connect(m_remoteAddress, timeout_ms);
	}


	bool Socket::close()
	{
		if (!m_isConnected && m_sock == -1) {
			return true;
		}

		m_isConnected = false;

		if (m_sock != -1) {
			closesocket(m_sock);
			m_sock = -1;
		}
		return false;
	}


	int Socket::send(const void* buffer, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			WSABUF buf;
			buf.buf = (char*)buffer;
			buf.len = length;
			DWORD num;
			 WSASend(m_sock, &buf, 1,&num,flags,nullptr,nullptr);
			return num;

		}
		return -1;
	}

	int Socket::send(const WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			DWORD num;
			WSASend(m_sock, (LPWSABUF)buffers, length, &num, flags, nullptr, nullptr);
			return num;
		}
		return -1;
	}

	int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		if (!isConnected()) {
			WSABUF buf;
			buf.buf = (char*)buffer;
			buf.len = length;
			DWORD num;
			WSASendTo(m_sock, &buf, 1, &num, flags,to->getAddr(),to->getAddrLen() ,nullptr, nullptr);
			return num;
			return num;
		}
		return -1;
	}

	int Socket::sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		if (!isConnected()) {
			DWORD num;
			DWORD rt= WSASendTo(m_sock, (LPWSABUF)buffers, length, &num, flags, to->getAddr(), to->getAddrLen(), nullptr, nullptr);
			return num;
		}
		return -1;
	}

	int Socket::recv(void* buffer, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			WSABUF buf;
			buf.buf = (char*)buffer;
			buf.len = length;
			DWORD num;
			WSARecv(m_sock, &buf, 1, &num, (LPDWORD)&flags,nullptr,nullptr);
			return num;
		}
		return -1;
	}

	int Socket::recv(WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			DWORD num;
			WSARecv(m_sock, (LPWSABUF)buffers, length, &num, (LPDWORD)&flags, nullptr, nullptr);
			return num;
		}
		return -1;
	}

	int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags /*= 0*/)
	{
		if (isConnected()) {
			WSABUF buf;
			buf.buf = (char*)buffer;
			buf.len = length;
			int len = from->getAddrLen();
			DWORD num;
			WSARecvFrom(m_sock, &buf, 1,&num,(LPDWORD)&flags, from->getAddr(), (LPINT)&len, nullptr,nullptr);
			return   num;
		}
		return -1;
	}

	int Socket::recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags /*= 0*/)
	{
		if (isConnected()) {
			int len = from->getAddrLen();
			DWORD num;
			WSARecvFrom(m_sock,buffers, length, &num, (LPDWORD)&flags, from->getAddr(), (LPINT)&len, nullptr, nullptr);
			return   num;
		}
		return -1;
	}

	sylar::Address::ptr Socket::getLocalAddress()
	{
		if (m_localAddress) {
			return m_localAddress;
		}

		Address::ptr result;
		switch (m_family) {
		case AF_INET:
			result = std::make_shared<IPv4Address>();
			break;
		case AF_INET6:
			result = std::make_shared<IPv6Address>();
			break;
		case AF_UNIX:
			result = std::make_shared<UnixAddress>();
			break;
		default:
			result = std::make_shared<UnknownAddress>(m_family);
			break;
		}
		socklen_t addrlen = result->getAddrLen();

		if (getsockname(m_sock, result->getAddr(), &addrlen)) {
			return std::make_shared<UnknownAddress>(m_family);
		}

		if (m_family == AF_UNIX) {
			UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
			addr->setAddrLen(addrlen);
		}
		m_localAddress = result;
		return m_localAddress;
	}

	sylar::Address::ptr Socket::getRemoteAddress()
	{
		if (m_remoteAddress) {
			return m_remoteAddress;
		}

		Address::ptr result;
		switch (m_family)
		{
			case AF_INET:
				result = std::make_shared<IPv4Address>();
				break;
			case AF_INET6:
				result = std::make_shared<IPv6Address>();
				break;
			case AF_UNIX:
				result = std::make_shared<UnixAddress>();
				break;
			default:
				result = std::make_shared<UnknownAddress>(m_family);
				break;
		}

		socklen_t addrlen = result->getAddrLen();
		if (getpeername(m_sock, result->getAddr(), &addrlen)) {
			return std::make_shared<UnknownAddress>(m_family);
		}

		if (m_family == AF_UNIX) {
			UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
			addr->setAddrLen(addrlen);
		}

		m_remoteAddress = result;
		return m_remoteAddress;
	}

	void Socket::initSock()
	{
		int val = 1;
		setOption(SOL_SOCKET, SO_REUSEADDR, val);
		if (m_type == SOCK_STREAM) {
			setOption(IPPROTO_TCP, TCP_NODELAY, val);
		}
	}

	void Socket::newSock()
	{
		SOCKET sock = socket(m_family, m_type, m_protocol);
		if (sock != -1) {
			m_sock = sock;
			initSock();
		}
	}

	bool Socket::init(size_t sock)
	{
			m_sock = sock;
			m_isConnected = true;
			initSock();
			getLocalAddress();
			getRemoteAddress();
			return true;
	}


	namespace {

		struct _SSLInit {
			_SSLInit() {
				SSL_library_init();
				SSL_load_error_strings();
				OpenSSL_add_all_algorithms();
			}
		};

		static _SSLInit s_init;

	}

	sylar::SSLSocket::SSLSocket::ptr SSLSocket::CreateTCP(sylar::Address::ptr address)
	{
		return std::make_shared<SSLSocket>(address->getFamily(), TCP, 0);
	}

	SSLSocket::ptr SSLSocket::CreateTCPSocket() {
		return std::make_shared<SSLSocket>(IPv4, TCP, 0);
	}

	SSLSocket::ptr SSLSocket::CreateTCPSocket6() {
		return std::make_shared<SSLSocket>(IPv6, TCP, 0);
	}

	std::ostream& SSLSocket::dump(std::ostream& os) const {
		os << "[SSLSocket sock=" << m_sock
			<< " is_connected=" << m_isConnected
			<< " family=" << m_family
			<< " type=" << m_type
			<< " protocol=" << m_protocol;
		if (m_localAddress) {
			os << " local_address=" << m_localAddress->toString();
		}
		if (m_remoteAddress) {
			os << " remote_address=" << m_remoteAddress->toString();
		}
		os << "]";
		return os;
	}

	SSLSocket::SSLSocket(int family, int type, int protocol /*= 0*/) :Socket(family, type, protocol)
	{

	}





	bool SSLSocket::bind(const Address::ptr addr)
	{
		return Socket::bind(addr);
	}

	bool SSLSocket::connect(const Address::ptr addr, uint64_t timeout_ms /*= -1*/)
	{
		bool v = Socket::connect(addr, timeout_ms);
		if (v) {
			m_ctx.reset(SSL_CTX_new(SSLv23_client_method()), SSL_CTX_free);
			m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
			SSL_set_fd(m_ssl.get(), m_sock);
			v = (SSL_connect(m_ssl.get()) == 1);
		}
		return v;

	}


	bool SSLSocket::close()
	{
		return Socket::close();
	}

	int SSLSocket::send(const void* buffer, size_t length, int flags /*= 0*/)
	{
		if (m_ssl) {
			return SSL_write(m_ssl.get(),buffer,length);
		}

		return -1;
	}

	int SSLSocket::send(const WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (!m_ssl) {
			return -1;
		}
		int total = 0;
		for (size_t i = 0; i < length; ++i) {
			int tmp = SSL_write(m_ssl.get(), buffers[i].buf, buffers[i].len);
			if (tmp <= 0) {
				return tmp;
			}
			total += tmp;
			if (tmp != (int)buffers[i].len) {
				break;
			}
		}
		return total;
	}

	int SSLSocket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		return -1;
	}

	int SSLSocket::sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		return -1;
	}

	int SSLSocket::recv(void* buffer, size_t length, int flags /*= 0*/)
	{
		if (m_ssl) {
			return SSL_read(m_ssl.get(), buffer, length);
		}
		return -1;
	}

	int SSLSocket::recv(WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (!m_ssl) {
			return -1;
		}
		int total = 0;
		for (size_t i = 0; i < length; ++i) {
			int tmp = SSL_read(m_ssl.get(), buffers[i].buf, buffers[i].len);
			if (tmp <= 0) {
				return tmp;
			}
			total += tmp;
			if (tmp != (int)buffers[i].len) {
				break;
			}
		}
		return total;
	}

	int SSLSocket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags /*= 0*/)
	{
		return -1;
	}

	int SSLSocket::recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags /*= 0*/)
	{

		return -1;
	}

	bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file)
	{
		m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
		if (SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) {
			return false;
		}
		if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
			return false;
		}
		if (SSL_CTX_check_private_key(m_ctx.get()) != 1) {
			return false;
		}
		return true;
	}

	bool SSLSocket::init(size_t sock)
	{
		bool v = Socket::init(sock);
		if (v) {
			m_ssl.reset(SSL_new(m_ctx.get()), SSL_free);
			SSL_set_fd(m_ssl.get(), m_sock);
			int rt = SSL_accept(m_ssl.get());
			v = rt == 1;
			if (!v) {
				int e = SSL_get_error(m_ssl.get(), rt);
			}
		}
		return v;
	}

	std::ostream& operator<<(std::ostream& os, const Socket& sock)
	{
		return sock.dump(os);
	}

}
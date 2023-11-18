#include "socket.h"
#include "log.h"
#include "fd_manager.h"
#include "iomanager.h"

namespace sylar {
	static Logger::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

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

	bool Socket::cancelRead()
	{
		return IOManager::GetThis()->cancelEvent(m_sock,IOManager::READ);
	}

	bool Socket::cancelWrite()
	{
		return IOManager::GetThis()->cancelEvent(m_sock, IOManager::WRITE);
	}

	bool Socket::cancelAccept()
	{
		return IOManager::GetThis()->cancelEvent(m_sock, IOManager::READ);
	}

	bool Socket::cancelAll()
	{
		return IOManager::GetThis()->cancelAll(m_sock);
	}

	int64_t Socket::getSendTimeout()
	{
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);

		if (ctx) {
			return ctx->getTimeout(SO_SNDTIMEO);
		}

		return -1;
	}

	void Socket::setSendTimeout(int64_t v)
	{
		struct timeval tv { (int)v / 1000, (int)(v % 1000 * 1000) };

		setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
	}

	int64_t Socket::getRecvTimeout()
	{
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(m_sock);

		if (ctx) {
			return ctx->getTimeout(SO_RCVTIMEO);
		}

		return -1;
	}
	 
	void Socket::setRecvTimeout(int64_t v)
	{
		struct timeval tv { (int)v / 1000, (int)(v % 1000 * 1000) };

		setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
	}

	bool Socket::getOption(int level, int option, void* result, socklen_t* len)
	{
		int rt = getsockopt(m_sock, level, option, (char*)result, len);

		if (rt) {
			SYLAR_LOG_DEBUG(g_logger) << "getOption sock=" << m_sock
				<< " level=" << level << " option=" << option
				<< " errno=" << errno << " errstr=" << strerror(errno);
			return false;
		}

		return true;
	}

	bool Socket::setOption(int level, int option, const void* result, socklen_t len)
	{
		if (s_setsockopt(m_sock, level, option, (char*)result, len)) {
			SYLAR_LOG_DEBUG(g_logger) << "setOption sock=" << m_sock
				<< " level=" << level << " option=" << option
				<< " errno=" << errno << " errstr=" << strerror(errno);

			return false;
		}

		return true;
	}

	Socket::ptr Socket::accept()
	{
		static char buffer[1024]{ 0 };

		Socket::ptr sock = std::make_shared<Socket>(m_family, m_type, m_protocol);

		SOCKET newsock = s_socket(m_family, m_type);

		int len = m_family == AF_INET ? sizeof(sockaddr) + 16 : sizeof(sockaddr_in6) + 16;

		bool ret= s_accept(m_sock, newsock,buffer,0,len,len,nullptr);

		if (!ret) {
			SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno
				<< " errstr=" << strerror(errno);
			s_closesocket(newsock);
			return nullptr;
		}

		if (sock->init(newsock))
		{
			return sock;
		}

		return nullptr;
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
			SYLAR_LOG_ERROR(g_logger) << "bind sock.family("
				<< m_family << ") addr.family(" << addr->getFamily()
				<< ") not equal, addr=" << addr->toString();
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
			SYLAR_LOG_ERROR(g_logger) << "bind error errno=" << errno
				<< " errstr=" << strerror(errno);
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
			SYLAR_LOG_ERROR(g_logger) << "connect sock.family("
				<< m_family << ") addr.family(" << addr->getFamily()
				<< ") not equal, addr=" << addr->toString();
			return false;
		}

		if (connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(),timeout_ms)) {
			SYLAR_LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
				<< ") error errno=" << errno << " errstr=" << strerror(errno);
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
			SYLAR_LOG_ERROR(g_logger) << "reconnect m_remoteAddress is null";
			return false;
		}
		m_localAddress.reset();
		return connect(m_remoteAddress, timeout_ms);
	}

	bool Socket::listen(int backlog /*= SOMAXCONN*/)
	{
		if (!isValid()) {
			SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
			return false;
		}

		if (::listen(m_sock, backlog)) {
			SYLAR_LOG_ERROR(g_logger) << "listen error errno=" << errno
				<< " errstr=" << strerror(errno);
			return false;
		}

		return true;
	}

	bool Socket::close()
	{
		if (!m_isConnected && m_sock == -1) {
			return true;
		}

		m_isConnected = false;

		if (m_sock != -1) {
			s_closesocket(m_sock);
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
			s_send(m_sock, &buf, 1,&num,flags);
			return num;
		}
		return -1;
	}

	int Socket::send(const WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			DWORD num;
			s_send(m_sock, (LPWSABUF)buffers, length, &num, flags);
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
			s_sendto(m_sock, &buf, 1,to->getAddr(),to->getAddrLen(), &num, flags);
			return num;
		}
		return -1;
	}

	int Socket::sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		if (!isConnected()) {
			DWORD num;
			s_sendto(m_sock, (LPWSABUF)buffers, length, to->getAddr(), to->getAddrLen(), &num, flags);
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
			s_recv(m_sock, &buf, 1, &num, flags);
			return num;
		}
		return -1;
	}

	int Socket::recv(WSABUF* buffers, size_t length, int flags /*= 0*/)
	{
		if (isConnected()) {
			DWORD num;
			s_recv(m_sock,(LPWSABUF )&buffers, length, &num, flags);
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
			DWORD num;
			int len = from->getAddrLen();
			s_recvfrom(m_sock, &buf, 1,from->getAddr(),(LPINT)&len, &num, flags);
			return num;
		}
		return -1;
	}

	int Socket::recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags /*= 0*/)
	{
		if (isConnected()) {
			DWORD num;
			int len = from->getAddrLen();
			s_recvfrom(m_sock, (LPWSABUF)buffers, length, from->getAddr(), (LPINT)&len, &num, flags);
			return num;
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
			SYLAR_LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
				<< " errno=" << errno << " errstr=" << strerror(errno);
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
		SOCKET sock = s_socket(m_family, m_type, m_protocol);
		if (SYLAR_LIKELY(sock != -1)) {
			m_sock = sock;
			initSock();
		}
		else {
			SYLAR_LOG_ERROR(g_logger) << "socket(" << m_family
				<< ", " << m_type << ", " << m_protocol << ") errno="
				<< errno << " errstr=" << strerror(errno);
		}
	}

	bool Socket::init(size_t sock)
	{
		FdCtx::ptr ctx = FdMgr::GetInstance()->get(sock);
		if (ctx && ctx->isSocket() && !ctx->isClose()) {
			m_sock = sock;
			m_isConnected = true;
			initSock();
			getLocalAddress();
			getRemoteAddress();
			return true;
		}
		return false;
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





	sylar::Socket::ptr SSLSocket::accept()
	{
		SSLSocket::ptr sock = std::make_shared<SSLSocket>(m_family, m_type, m_protocol);

		SOCKET newsock = s_socket(m_family, m_type);

		bool ret = s_accept(m_sock, newsock, nullptr, 0, 0, 0, nullptr);

		if (!ret) {
			SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno
				<< " errstr=" << strerror(errno);
			s_closesocket(newsock);
			return nullptr;
		}

		sock->m_ctx = m_ctx;

		if (sock->init(newsock))
		{
			return sock;
		}

		return nullptr;
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

	bool SSLSocket::listen(int backlog /*= SOMAXCONN*/)
	{
		return Socket::listen(backlog);
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
		SYLAR_ASSERT(false);
		return -1;
	}

	int SSLSocket::sendTo(const WSABUF* buffers, size_t length, const Address::ptr to, int flags /*= 0*/)
	{
		SYLAR_ASSERT(false);
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
		SYLAR_ASSERT(false);
		return -1;
	}

	int SSLSocket::recvFrom(WSABUF* buffers, size_t length, Address::ptr from, int flags /*= 0*/)
	{
		SYLAR_ASSERT(false);
		return -1;
	}

	bool SSLSocket::loadCertificates(const std::string& cert_file, const std::string& key_file)
	{
		m_ctx.reset(SSL_CTX_new(SSLv23_server_method()), SSL_CTX_free);
		if (SSL_CTX_use_certificate_chain_file(m_ctx.get(), cert_file.c_str()) != 1) {
			SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_certificate_chain_file("
				<< cert_file << ") error";
			return false;
		}
		if (SSL_CTX_use_PrivateKey_file(m_ctx.get(), key_file.c_str(), SSL_FILETYPE_PEM) != 1) {
			SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_use_PrivateKey_file("
				<< key_file << ") error";
			return false;
		}
		if (SSL_CTX_check_private_key(m_ctx.get()) != 1) {
			SYLAR_LOG_ERROR(g_logger) << "SSL_CTX_check_private_key cert_file="
				<< cert_file << " key_file=" << key_file;
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
				SYLAR_LOG_ERROR(g_logger) << "SSL_accept rt=" << rt
					<< " err=" << e
					<< " errstr=" << ERR_error_string(e, nullptr);
			}
		}
		return v;
	}

	std::ostream& operator<<(std::ostream& os, const Socket& sock)
	{
		return sock.dump(os);
	}

}
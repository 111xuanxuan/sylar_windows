#pragma once
#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__


#include "../stream.h"
#include "../socket.h"
#include "../iomanager.h"


namespace sylar {

	class SocketStream :public Stream {
	public:
		using ptr = std::shared_ptr<SocketStream>;

		SocketStream(Socket::ptr sock, bool owner = true);

		~SocketStream();

		virtual int read(void* buffer, size_t length) override;

		virtual int read(ByteArray::ptr ba, size_t length) override;

		virtual int write(const void* buffer, size_t length) override;

		virtual int write(ByteArray::ptr ba, size_t length) override;

		virtual void close() override;

		Socket::ptr getSocket() const { return m_socket; }

		bool isConnected() const;
		bool checkConnected();

		Address::ptr getRemoteAddress();
		Address::ptr getLocalAddress();
		std::string getRemoteAddressString();
		std::string getLocalAddressString();

		uint64_t getId() const { return m_id; }

		/// SocketÀà
		Socket::ptr m_socket;
		uint64_t m_id : 63;
		/// ÊÇ·ñÖ÷¿Ø
		bool m_owner : 1;
	};
}
#endif
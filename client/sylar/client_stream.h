#pragma once
#ifndef ROCK_STREAM_H
#define ROCK_STREAM_H

#include "socket_stream.h"
#include "rock_protocol.h"

using namespace sylar;



	class ClientStream :public std::enable_shared_from_this<ClientStream> {
	public:
		using ptr = std::shared_ptr<ClientStream>;

		ClientStream(Socket::ptr sock) :m_socket{std::make_shared<SocketStream>(sock)}, m_decoder{ std::make_shared<RockMessageDecoder>() }{
		}

		~ClientStream() {
			m_socket->close();
		}

		static Message::ptr CreateMessage(Message::MessageType type,ByteArray::ptr data);
		static std::string  GetMessageResult(Message::ptr msg);

		Message::ptr doRecv();
		bool doSend(Message::ptr msg);

	private:
		RockMessageDecoder::ptr m_decoder;
		SocketStream::ptr m_socket;
		inline static  std::atomic_uint32_t m_sn;

	};







#endif // 

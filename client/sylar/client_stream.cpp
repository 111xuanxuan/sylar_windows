#include "client_stream.h"
#include "../../sylar/sylar/endia.h"




	sylar::Message::ptr ClientStream::CreateMessage(Message::MessageType type, ByteArray::ptr data)
	{
		if (type == Message::REQUEST) {
			auto req= std::make_shared<RockRequest>();
			req->setSn(m_sn++ );
			req->setBody(data->toString());
			return req;
		}
		else if (type == Message::NOTIFY) {
			return std::make_shared<RockNotify>();
		}
	}

	std::string ClientStream::GetMessageResult(Message::ptr msg)
	{
		auto type = msg->getType();

		if (type == Message::RESPONSE) {
			auto res = std::dynamic_pointer_cast<RockResponse>(msg);
			return res->getResultStr();
		}
		else
		{
			return "";
		}
	}


	Message::ptr ClientStream::doRecv() {
		auto msg = m_decoder->parseFrom(m_socket);

		if (!msg) {
			m_socket->close();
			return nullptr;
		}

		int type = msg->getType();
		if (type == Message::RESPONSE) {
			auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
			if (!rsp) {
				return nullptr;
			}
			return rsp;
		}
		else if (type == Message::REQUEST) {
			auto req = std::dynamic_pointer_cast<RockRequest>(msg);
			if (!req) {
				return nullptr;
			}
			return req;
		}
		else if (type == Message::NOTIFY) {
			auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
			if (!nty) {
				return nullptr;
			}
			return nty;
		}

		return nullptr;
	}

	bool ClientStream::doSend(Message::ptr msg)
	{
		return this->m_decoder->serializeTo(m_socket, msg) > 0;
	}

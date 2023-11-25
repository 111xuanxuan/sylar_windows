#include "rock_protocol.h"
#include "../log.h"
#include "../config.h"
#include "../endia.h"
#include "../stream/zlib_stream.h"


namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	static sylar::ConfigVar<uint32_t>::ptr g_rock_protocol_max_length
		= sylar::Config::Lookup("rock.protocol.max_length",
			(uint32_t)(1024 * 1024 * 64), "rock protocol max length");

	static sylar::ConfigVar<uint32_t>::ptr g_rock_protocol_gzip_min_length
		= sylar::Config::Lookup("rock.protocol.gzip_min_length",
			(uint32_t)(1024 * 1024 * 64), "rock protocol gizp min length");


	bool RockBody::serializeToByteArray(ByteArray::ptr bytearray)
	{
		bytearray->writeStringF32(m_body);
		return true;
	}

	bool RockBody::parseFromByteArray(ByteArray::ptr bytearray)
	{
		m_body = bytearray->readStringF32();
		return true;
	}

	RockResponse::ptr RockRequest::createResponse()
	{
		RockResponse::ptr rt = std::make_shared<RockResponse>();
		rt->setSn(m_sn);
		rt->setCmd(m_cmd);
		return rt;
	}

	std::string RockRequest::toString() const
	{
		std::stringstream ss;
		ss << "[RockRequest sn=" << m_sn
			<< " cmd=" << m_cmd
			<< " body.length=" << m_body.size()
			<< "]";
		return ss.str();
	}

	const std::string& RockRequest::getName() const
	{
		static const std::string& s_name = "RockRequest";
		return s_name;
	}

	int32_t RockRequest::getType() const
	{
		return Message::REQUEST;
	}

	bool RockRequest::serializeToByteArray(ByteArray::ptr bytearray)
	{
		try
		{
			bool v = true;
			v &= Request::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		}
		catch (...)
		{
			SYLAR_LOG_ERROR(g_logger) << "RockRequest serializeToByteArray error";
		}
		return false;

	}

	bool RockRequest::parseFromByteArray(ByteArray::ptr bytearray)
	{
		try
		{
			bool v = true;
			v &= Request::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		}
		catch (...)
		{
			SYLAR_LOG_ERROR(g_logger) << "RockRequest parseFromByteArray error "
				<< bytearray->toHexString();
		}
		return false;
	}

	std::string RockResponse::toString() const
	{
		std::stringstream ss;
		ss << "[RockResponse sn=" << m_sn
			<< " cmd=" << m_cmd
			<< " result=" << m_result
			<< " result_msg=" << m_resultStr
			<< " body.length=" << m_body.size()
			<< "]";
		return ss.str();
	}

	const std::string& RockResponse::getName() const
	{
		static const std::string& s_name = "RockResponse";
		return s_name;
	}

	int32_t RockResponse::getType() const
	{
		return Message::RESPONSE;
	}

	bool RockResponse::serializeToByteArray(ByteArray::ptr bytearray)
	{
		try
		{
			bool v = true;
			v &= Response::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		}
		catch (...)
		{
			SYLAR_LOG_ERROR(g_logger) << "RockResponse serializeToByteArray error";
		}
		return false;
	}

	bool RockResponse::parseFromByteArray(ByteArray::ptr bytearray)
	{
		try
		{
			bool v = true;
			v &= Response::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		}
		catch (...)
		{
			SYLAR_LOG_ERROR(g_logger) << "RockResponse parseFromByteArray error";
		}
		return false;
	}

	std::string RockNotify::toString() const
	{
		std::stringstream ss;
		ss << "[RockNotify notify=" << m_notify
			<< " body.length=" << m_body.size()
			<< "]";
		return ss.str();
	}

	const std::string& RockNotify::getName() const
	{
		static const std::string& s_name = "RockNotify";
		return s_name;
	}

	int32_t RockNotify::getType() const
	{
		return Message::NOTIFY;
	}

	bool RockNotify::serializeToByteArray(ByteArray::ptr bytearray)
	{
		try {
			bool v = true;
			v &= Notify::serializeToByteArray(bytearray);
			v &= RockBody::serializeToByteArray(bytearray);
			return v;
		}
		catch (...) {
			SYLAR_LOG_ERROR(g_logger) << "RockNotify serializeToByteArray error";
		}
		return false;
	}

	bool RockNotify::parseFromByteArray(ByteArray::ptr bytearray)
	{
		try {
			bool v = true;
			v &= Notify::parseFromByteArray(bytearray);
			v &= RockBody::parseFromByteArray(bytearray);
			return v;
		}
		catch (...) {
			SYLAR_LOG_ERROR(g_logger) << "RockNotify parseFromByteArray error";
		}
		return false;
	}

	static const uint8_t s_rock_magic[2] = { 0x12,0x21 };

	RockMsgHeader::RockMsgHeader()
		:magic{ s_rock_magic[0], s_rock_magic[1] }
		, version(1)
		, flag(0)
		, length(0) { 
	}

	sylar::Message::ptr sylar::RockMessageDecoder::parseFrom(Stream::ptr stream)
	{
		try
		{
			RockMsgHeader header;

			//��ȡ��Ϣͷ
			int rt = stream->readFixSize(&header, sizeof(header));

			//��ȡʧ��
			if (rt <= 0) {
				SYLAR_LOG_DEBUG(g_logger) << "RockMessageDecoder decode head error rt=" << rt << " " << strerror(errno);
				return nullptr;
			}

			if (memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic))) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.magic error";
				return nullptr;
			}

			if (header.version != 0x1) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.version != 0x1";
				return nullptr;
			}


			//��Ϣ����
			header.length = byteswapOnLittleEndian(header.length);
			if ((uint32_t)header.length >= g_rock_protocol_gzip_min_length->getValue()) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.length("
					<< header.length << ") >="
					<< g_rock_protocol_max_length->getValue();
				return nullptr;
			}

			sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();

			//������Ϣ
			rt = stream->readFixSize(ba, header.length);
			if (rt <= 0) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder read body fail length=" << header.length << " rt=" << rt
					<< " errno=" << errno << " - " << strerror(errno);
				return nullptr;
			}

			//���ö�ȡλ��
			ba->setPosition(0);
			if (header.flag & 0x1) {//gizp

				//��ѹ
				auto zstream = ZlibStream::CreateGzip(false);

				if (zstream->write(ba, -1) != Z_OK) {
					SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder ungzip error";
					return nullptr;
				}

				if (zstream->flush() != Z_OK) {
					SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder ungzip flush error";
					return nullptr;
				}

				ba = zstream->getByteArray();

			}

			//��ȡ����
			uint8_t type = ba->readFuint8();
			Message::ptr msg;
			switch (type)
			{
			case Message::REQUEST:
				msg = std::make_shared<RockRequest>();
				break;
			case Message::RESPONSE:
				msg = std::make_shared<RockResponse>();
				break;
			case Message::NOTIFY:
				msg = std::make_shared<RockNotify>();
				break;
			default:
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder invalid type=" << (int)type;
				return nullptr;
			}

			if (!msg->parseFromByteArray(ba)) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder parseFromByteArray fail type=" << (int)type;
				return nullptr;
			}

			return msg;

		}
		catch (const std::exception& e)
		{
			SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder except:" << e.what();
		}
		catch (...) {
			SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder except";
		}

		return nullptr;

	}


	int32_t RockMessageDecoder::serializeTo(Stream::ptr stream, Message::ptr msg)
	{
		//Э��ͷ
		RockMsgHeader header;
		auto ba = msg->toByteArray();
		ba->setPosition(0);
		header.length = ba->getSize();

		//���ȴ���ѹ������С����
		if ((uint32_t)header.length >= g_rock_protocol_gzip_min_length->getValue()) {
			auto zstream = sylar::ZlibStream::CreateGzip(true);
			if (zstream->write(ba, -1) != Z_OK) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo gizp error";
				return -1;
			}
			if (zstream->flush() != Z_OK) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo gizp flush error";
				return -2;
			}

			ba = zstream->getByteArray();
			//����Gzip
			header.flag |= 0x1;
			header.length = ba->getSize();
		}

		header.length = sylar::byteswapOnLittleEndian(header.length);
		int rt = stream->writeFixSize(&header, sizeof(header));
		if (rt <= 0) {
			SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write header fail rt=" << rt
				<< " errno=" << errno << " - " << strerror(errno);
			return -3;
		}
		rt = stream->writeFixSize(ba, ba->getReadSize());
		if (rt <= 0) {
			SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder serializeTo write body fail rt=" << rt
				<< " errno=" << errno << " - " << strerror(errno);
			return -4;
		}

		return sizeof(header) + ba->getSize();

	}

}

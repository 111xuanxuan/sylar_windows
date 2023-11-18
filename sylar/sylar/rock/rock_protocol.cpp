#include "rock_protocol.h"
#include "../log.h"
#include "../config.h"
#include "../endia.h"


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

	std::shared_ptr<sylar::RockResponse> RockRequest::createResponse()
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
		catch (const std::exception&)
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
			{

			}

	}

	sylar::Message::ptr sylar::RockMessageDecoder::parseFrom(Stream::ptr stream)
	{
		try
		{
			RockMsgHeader header;

			//读取消息头
			int rt = stream->readFixSize(&header, sizeof(header));

			if (rt <= 0) {
				SYLAR_LOG_DEBUG(g_logger) << "RockMessageDecoder decode head error rt=" << rt << " " << strerror(errno);
				return nullptr;
			}

			if (memcpy(header.magic, s_rock_magic, sizeof(s_rock_magic))) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.magic error";
				return nullptr;
			}

			if (header.version != 0x1) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.version != 0x1";
				return nullptr;
			}

			//消息长度
			header.length = byteswapOnLittleEndian(header.length);
			if ((uint32_t)header.length >= g_rock_protocol_gzip_min_length->getValue()) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder head.length("
					<< header.length << ") >="
					<< g_rock_protocol_max_length->getValue();
				return nullptr;
			}

			sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();

			//接受消息
			rt = stream->readFixSize(ba, header.length);
			if (rt <= 0) {
				SYLAR_LOG_ERROR(g_logger) << "RockMessageDecoder read body fail length=" << header.length << " rt=" << rt
					<< " errno=" << errno << " - " << strerror(errno);
				return nullptr;
			}

			ba->setPosition(0);
			if (header.flag & 0x1) {//gizp
				
			}


		}
		catch (const std::exception&)
		{

		}
	}


}

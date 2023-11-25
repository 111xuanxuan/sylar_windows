#include "rock_protocol.h"
#include "endia.h"
#include "zlib_stream.h"


namespace sylar {

	uint32_t g_rock_protocol_max_length = 1024 * 1024 * 64;
	uint32_t g_rock_protocol_gzip_min_length = 1024 * 1024 * 64;


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

			//读取消息头
			int rt = stream->readFixSize(&header, sizeof(header));

			//读取失败
			if (rt <= 0) {
				return nullptr;
			}

			if (memcmp(header.magic, s_rock_magic, sizeof(s_rock_magic))) {
				return nullptr;
			}

			if (header.version != 0x1) {
				return nullptr;
			}


			//消息长度
			header.length = byteswapOnLittleEndian(header.length);
			if ((uint32_t)header.length >= g_rock_protocol_gzip_min_length) {
				return nullptr;
			}

			sylar::ByteArray::ptr ba = std::make_shared<sylar::ByteArray>();

			//接受消息
			rt = stream->readFixSize(ba, header.length);
			if (rt <= 0) {
				return nullptr;
			}

			//设置读取位置
			ba->setPosition(0);
			if (header.flag & 0x1) {//gizp

				//解压
				auto zstream = ZlibStream::CreateGzip(false);

				if (zstream->write(ba, -1) != Z_OK) {
					return nullptr;
				}

				if (zstream->flush() != Z_OK) {
					return nullptr;
				}

				ba = zstream->getByteArray();

			}

			//获取类型
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
				return nullptr;
			}

			if (!msg->parseFromByteArray(ba)) {
				return nullptr;
			}

			return msg;

		}
		catch (const std::exception& e)
		{
		}
		catch (...) {
		}

		return nullptr;

	}


	int32_t RockMessageDecoder::serializeTo(Stream::ptr stream, Message::ptr msg)
	{
		//协议头
		RockMsgHeader header;
		auto ba = msg->toByteArray();
		ba->setPosition(0);
		header.length = ba->getSize();

		//长度大于压缩的最小长度
		if ((uint32_t)header.length >= g_rock_protocol_gzip_min_length) {
			auto zstream = sylar::ZlibStream::CreateGzip(true);
			if (zstream->write(ba, -1) != Z_OK) {
				return -1;
			}
			if (zstream->flush() != Z_OK) {
				return -2;
			}

			ba = zstream->getByteArray();
			//采用Gzip
			header.flag |= 0x1;
			header.length = ba->getSize();
		}

		header.length = sylar::byteswapOnLittleEndian(header.length);
		int rt = stream->writeFixSize(&header, sizeof(header));
		if (rt <= 0) {
			return -3;
		}
		rt = stream->writeFixSize(ba, ba->getReadSize());
		if (rt <= 0) {
			return -4;
		}

		return sizeof(header) + ba->getSize();

	}

}

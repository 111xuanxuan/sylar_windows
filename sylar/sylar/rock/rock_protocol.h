#pragma once
#ifndef __SYLAR_ROCK_ROCK_PROTOCOL_H__
#define __SYLAR_ROCK_ROCK_PROTOCOL_H__
//Rock协议
#include "../protocol.h"
//#include <google/protobuf/message.h>

namespace sylar {

	//主体
	class RockBody {
	public:
		using ptr = std::shared_ptr<RockBody>;

		virtual ~RockBody(){}

		//设置主体字符串
		void setBody(const std::string& v) {
			m_body = v;
		}

		//获取主体字符串
		const std::string& getBody() const { 
			return m_body; 
		}

		template<class T>
		std::shared_ptr<T> getAsPB() const {
			try {
				std::shared_ptr<T> data = std::make_shared<T>();
				if (data->ParseFromString(m_body)) {
					return data;
				}
			}
			catch (...) {
			}
			return nullptr;
		}

		template<class T>
		bool setAsPB(const T& v) {
			try {
				return v.SerializeToString(&m_body);
			}
			catch (...) {
			}
			return false;
		}


		virtual bool serializeToByteArray(ByteArray::ptr bytearray);
		virtual bool parseFromByteArray(ByteArray::ptr bytearray);


	protected:
		//主体数据
		std::string m_body;

	};


	class RockResponse;

	//请求体
	class RockRequest :public Request, public RockBody {
	public:
		using ptr = std::shared_ptr<RockRequest>;

		//创建响应体
		std::shared_ptr<RockResponse> createResponse();

		virtual std::string toString()const override;
		virtual const std::string& getName()const override;
		virtual int32_t getType()const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray)override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray)override;

	};

	//响应体
	class RockResponse :public Response, public RockBody {
	public:
		using ptr = std::shared_ptr<RockResponse>;

		virtual std::string toString()const override;
		virtual const std::string& getName()const override;
		virtual int32_t getType()const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray)override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray)override;
	};

	//通知体
	class RockNotify :public Notify, public RockBody {
	public:
		using ptr = std::shared_ptr<RockNotify>;

		virtual std::string toString() const override;
		virtual const std::string& getName() const override;
		virtual int32_t getType() const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	};

	//头部
	struct RockMsgHeader {
		RockMsgHeader();
		uint8_t magic[2];
		//版本
		uint8_t version;
		uint8_t flag;
		int32_t length;
	};

	//数据解压缩类
	class RockMessageDecoder :public MessageDecoder {
	public:
		using ptr = std::shared_ptr<MessageDecoder>;

		//数据解压
		virtual Message::ptr parseFrom(Stream::ptr stream) override;

		//对数据进行压缩
		virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) override;
	};



}
#endif
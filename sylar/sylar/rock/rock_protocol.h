#pragma once
#ifndef __SYLAR_ROCK_ROCK_PROTOCOL_H__
#define __SYLAR_ROCK_ROCK_PROTOCOL_H__
//RockЭ��
#include "../protocol.h"
//#include <google/protobuf/message.h>

namespace sylar {

	//����
	class RockBody {
	public:
		using ptr = std::shared_ptr<RockBody>;

		virtual ~RockBody(){}

		//���������ַ���
		void setBody(const std::string& v) {
			m_body = v;
		}

		//��ȡ�����ַ���
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
		//��������
		std::string m_body;

	};


	class RockResponse;

	//������
	class RockRequest :public Request, public RockBody {
	public:
		using ptr = std::shared_ptr<RockRequest>;

		//������Ӧ��
		std::shared_ptr<RockResponse> createResponse();

		virtual std::string toString()const override;
		virtual const std::string& getName()const override;
		virtual int32_t getType()const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray)override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray)override;

	};

	//��Ӧ��
	class RockResponse :public Response, public RockBody {
	public:
		using ptr = std::shared_ptr<RockResponse>;

		virtual std::string toString()const override;
		virtual const std::string& getName()const override;
		virtual int32_t getType()const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray)override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray)override;
	};

	//֪ͨ��
	class RockNotify :public Notify, public RockBody {
	public:
		using ptr = std::shared_ptr<RockNotify>;

		virtual std::string toString() const override;
		virtual const std::string& getName() const override;
		virtual int32_t getType() const override;

		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	};

	//ͷ��
	struct RockMsgHeader {
		RockMsgHeader();
		uint8_t magic[2];
		//�汾
		uint8_t version;
		uint8_t flag;
		int32_t length;
	};

	//���ݽ�ѹ����
	class RockMessageDecoder :public MessageDecoder {
	public:
		using ptr = std::shared_ptr<MessageDecoder>;

		//���ݽ�ѹ
		virtual Message::ptr parseFrom(Stream::ptr stream) override;

		//�����ݽ���ѹ��
		virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) override;
	};



}
#endif
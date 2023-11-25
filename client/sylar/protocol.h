#pragma once
//�Զ���Э��
#ifndef __SYLAR_PROTOCOL_H__
#define __SYLAR_PROTOCOL_H__

#include "stream.h"
#include "bytearray.h"

namespace sylar {

	//��Ϣ�������
	class Message {
	public:
		using ptr = std::shared_ptr<Message>;

		enum MessageType {
			REQUEST = 1,
			RESPONSE = 2,
			NOTIFY = 3
		};

		virtual ~Message() {}

		virtual ByteArray::ptr toByteArray();

		//���л�
		virtual bool serializeToByteArray(ByteArray::ptr bytearray) = 0;
		//�����л�
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) = 0;

		virtual std::string toString() const = 0;
		virtual const std::string& getName() const = 0;
		virtual int32_t getType() const = 0;
	};

	//��Ϣѹ�����ѹ��
	class MessageDecoder {
	public:
		using ptr = std::shared_ptr<MessageDecoder>;

		virtual ~MessageDecoder() {}
		virtual Message::ptr parseFrom(Stream::ptr stream) = 0;
		virtual int32_t serializeTo(Stream::ptr stream, Message::ptr msg) = 0;

	};

	//������
	class Request :public Message {
	public:
		using ptr = std::shared_ptr<Request>;

		Request();

		 uint32_t getSn() const { return m_sn;}
		 uint32_t getCmd() const { return m_cmd; }

		 void setSn(uint32_t v) { m_sn = v; }
		 void setCmd(uint32_t v) { m_cmd = v; }

		 //���л�
		 virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		 //�����л�
		 virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;

		 uint64_t getTime() { return m_time; }
		 void setTime(uint64_t v) { m_time = v; }

	protected:
		//���к�
		uint32_t m_sn;
		//������
		uint32_t m_cmd;
		//ʱ���
		uint64_t m_time = 0; //us

	};

	//��Ӧ��
	class Response : public Message {
	public:
		using ptr= std::shared_ptr<Response> ;

		Response();

		uint32_t getSn() const { return m_sn; }
		uint32_t getCmd() const { return m_cmd; }
		uint32_t getResult() const { return m_result; }
		const std::string& getResultStr() const { return m_resultStr; }

		void setSn(uint32_t v) { m_sn = v; }
		void setCmd(uint32_t v) { m_cmd = v; }
		void setResult(uint32_t v) { m_result = v; }
		void setResultStr(const std::string& v) { m_resultStr = v; }

		//���л�
		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		//�����л�
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	protected:
		//���к�
		uint32_t m_sn;
		//������
		uint32_t m_cmd;

		uint32_t m_result;

		std::string m_resultStr;
	};

	//֪ͨ��
	class Notify : public Message {
	public:
		typedef std::shared_ptr<Notify> ptr;
		Notify();

		uint32_t getNotify() const { return m_notify; }
		void setNotify(uint32_t v) { m_notify = v; }

		//���л�
		virtual bool serializeToByteArray(ByteArray::ptr bytearray) override;
		//�����л�
		virtual bool parseFromByteArray(ByteArray::ptr bytearray) override;
	protected:
		uint32_t m_notify;
	};




}
#endif

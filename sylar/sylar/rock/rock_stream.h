#pragma once
#ifndef __SYLAR_ROCK_ROCK_STREAM_H__
#define __SYLAR_ROCK_ROCK_STREAM_H__

#include "../stream//async_socket_stream.h"
#include "rock_protocol.h"
#include "../singleton.h"
#include <boost/any.hpp>


namespace sylar {

	struct RockResult {
		using ptr = std::shared_ptr<RockResult>;


		RockResult(int32_t _result, const std::string& _resultStr, int32_t _used, RockResponse::ptr rsp, RockRequest::ptr req)
			:result(_result)
			, used(_used)
			, resultStr(_resultStr)
			, response(rsp)
			, request(req) {
		}


		int32_t result;
		int32_t used;
		std::string resultStr;
		RockResponse::ptr response;
		RockRequest::ptr request;

		std::string server;

		std::string toString() const;

	};

	//RockStream��
	class RockStream :public AsyncSocketStream {
	public:
		using ptr = std::shared_ptr<RockStream>;
		using request_handler = std::function<bool(RockRequest::ptr,RockResponse::ptr,RockStream::ptr)>;
		using notify_handler = std::function<bool(RockNotify::ptr,RockStream::ptr)>;

		RockStream(Socket::ptr sock);
		~RockStream();
		
		//������Ϣ
		int32_t sendMessage(Message::ptr msg);
		RockResult::ptr request(RockRequest::ptr req, uint32_t timeout_ms);

		//��ȡ����ص�
		request_handler getRequestHandler() const { return m_requestHandler; }
		//��ȡ֪ͨ�ص�
		notify_handler getNotifyHandler() const { return m_notifyHandler; }

		//��������ص�
		void setRequestHandler(request_handler v) { m_requestHandler = v; }
		//����֪ͨ�ص�
		void setNotifyHandler(notify_handler v) { m_notifyHandler = v; }

		template<class T>
		void setData(const T& v) {
			m_data = v;
		}

		template<class T>
		T getData() {
			try {
				return boost::any_cast<T>(m_data);
			}
			catch (...) {
			}
			return T();
		}

	protected:

		struct RockSendCtx :public SendCtx {
			using ptr= std::shared_ptr<RockSendCtx> ;
			Message::ptr msg;

			//��������
			virtual bool doSend(AsyncSocketStream::ptr stream) override;
		};

		struct RockCtx : public Ctx {
			using ptr=std::shared_ptr<RockCtx> ;
			RockRequest::ptr request;
			RockResponse::ptr response;

			//��������
			virtual bool doSend(AsyncSocketStream::ptr stream) override;
		};

		virtual Ctx::ptr doRecv() override;

		void handleRequest(sylar::RockRequest::ptr req);
		void handleNotify(sylar::RockNotify::ptr nty);

	private:
		//��Ϣѹ������
		RockMessageDecoder::ptr m_decoder;
		//��������
		request_handler m_requestHandler;
		//����֪ͨ
		notify_handler m_notifyHandler;
		//����
		boost::any m_data;
		//���к�
		std::atomic<uint32_t> m_sn = 0;

	};

	//RockSession��
	class RockSession :public RockStream {
	public:
		using ptr = std::shared_ptr<RockSession>;
		RockSession(Socket::ptr sock);
	};

	//RockConnect��
	class RockConnection :public RockStream {
	public:
		using ptr = std::shared_ptr<RockConnection>;
		RockConnection();
		bool connect(Address::ptr addr);

	};




}
#endif
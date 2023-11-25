#include "rock_stream.h"
#include "../config.h"

namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	static sylar::ConfigVar<std::unordered_map<std::string
		, std::unordered_map<std::string, std::string> > >::ptr g_rock_services =
		sylar::Config::Lookup("rock_services", std::unordered_map<std::string
			, std::unordered_map<std::string, std::string> >(), "rock_services");

	std::string RockResult::toString() const
	{
		std::stringstream ss;
		ss << "[RockResult result=" << result
			<< " result_str=" << resultStr
			<< " used=" << used
			<< " response=" << (response ? response->toString() : "null")
			<< " request=" << (request ? request->toString() : "null")
			<< " server=" << server
			<< "]";
		return ss.str();
	}



	RockStream::RockStream(Socket::ptr sock)
		:AsyncSocketStream(sock,true)
		,m_decoder(std::make_shared<RockMessageDecoder>())

	{
		SYLAR_LOG_DEBUG(g_logger) << "RockStream::RockStream "
			<< (sock ? sock->toString() : "");
	}
	

	RockStream::~RockStream()
	{
		SYLAR_LOG_DEBUG(g_logger) << "RockStream::~RockStream "
			<< (m_socket ? m_socket->toString() : "");
	}




	int32_t RockStream::sendMessage(Message::ptr msg)
	{
		//判断是否连接状态
		if (isConnected()) {
			//构造一个SendCtx
			RockSendCtx::ptr ctx = std::make_shared<RockSendCtx>();
			//赋值消息
			ctx->msg = msg;
			//插入队列中
			enqueue(ctx);
			return 1;
		}
		else
		{
			return -1;
		}
	}

	sylar::RockResult::ptr RockStream::request(RockRequest::ptr req, uint32_t timeout_ms)
	{
		if (isConnected()) {

			if (req->getSn() == 0) {
				req->setSn(m_sn.fetch_add(1));
			}

			RockCtx::ptr ctx = std::make_shared<RockCtx>();
			ctx->request = req;
			ctx->sn = req->getSn();
			ctx->timeout = timeout_ms;
			ctx->scheduler = Scheduler::GetThis();
			ctx->fiber = Fiber::GetThis();
			addCtx(ctx);
			uint64_t ts = GetCurrentMS();
			ctx->timer = IOManager::GetThis()->addTimer(timeout_ms, std::bind(&RockStream::onTimeOut,shared_from_this(),ctx));
			enqueue(ctx);
			Fiber::YieldToHold();
			auto rt = std::make_shared<RockResult>(ctx->result, ctx->resultStr, GetCurrentMS() - ts, ctx->response, req);
			rt->server = getRemoteAddressString();
			return rt;

		}
		else
		{
			auto rt = std::make_shared<RockResult>(AsyncSocketStream::NOT_CONNECT, "not_connect " + getRemoteAddressString(), 0, nullptr, req);
			rt->server = getRemoteAddressString();
			return rt;
		}
	}


	bool RockStream::RockSendCtx::doSend(AsyncSocketStream::ptr stream)
	{
		return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder->serializeTo(stream, msg) > 0;
	}

	bool RockStream::RockCtx::doSend(AsyncSocketStream::ptr stream)
	{
		return std::dynamic_pointer_cast<RockStream>(stream)->m_decoder->serializeTo(stream, request) > 0;
	}

	sylar::AsyncSocketStream::Ctx::ptr RockStream::doRecv()
	{
		auto msg = m_decoder->parseFrom(shared_from_this());

		if (!msg) {
			innerClose();
			return nullptr;
		}

		int type = msg->getType();
		if (type == Message::RESPONSE) {
			auto rsp = std::dynamic_pointer_cast<RockResponse>(msg);
			if (!rsp) {
				SYLAR_LOG_WARN(g_logger) << "RockStream doRecv response not RockResponse: "
					<< msg->toString();
				return nullptr;
			}
			RockCtx::ptr	ctx = getAndDelCtxAs<RockCtx>(rsp->getSn());
			if (!ctx) {
				SYLAR_LOG_WARN(g_logger) << "RockStream request timeout reponse="
					<< rsp->toString();
				return nullptr;
			}
			ctx->result = rsp->getResult();
			ctx->resultStr = rsp->getResultStr();
			ctx->response = rsp;
			return ctx;
		}
		else if (type == Message::REQUEST) {
			auto req = std::dynamic_pointer_cast<RockRequest>(msg);
			if (!req) {
				SYLAR_LOG_WARN(g_logger) << "RockStream doRecv request not RockRequest: "
					<< msg->toString();
				return nullptr;
			}
			//如果有请求回调
			if (m_requestHandler) {
				m_worker->schedule(std::bind(&RockStream::handleRequest,
					std::dynamic_pointer_cast<RockStream>(shared_from_this()),
					req));
			}
			else {
				SYLAR_LOG_WARN(g_logger) << "unhandle request " << req->toString();
			}
		}
		else if (type == Message::NOTIFY) {
			auto nty = std::dynamic_pointer_cast<RockNotify>(msg);
			if (!nty) {
				SYLAR_LOG_WARN(g_logger) << "RockStream doRecv notify not RockNotify: "
					<< msg->toString();
				return nullptr;
			}

			if (m_notifyHandler) {
				m_worker->schedule(std::bind(&RockStream::handleNotify,
					std::dynamic_pointer_cast<RockStream>(shared_from_this()),
					nty));
			}
			else {
				SYLAR_LOG_WARN(g_logger) << "unhandle notify " << nty->toString();
			}

		}
		else
		{
			SYLAR_LOG_WARN(g_logger) << "RockStream recv unknow type=" << type
				<< " msg: " << msg->toString();
		}

		return nullptr;
	}

	void RockStream::handleRequest(sylar::RockRequest::ptr req)
	{
		//创建请求
		RockResponse::ptr rsp = req->createResponse();
		//对请求进行处理并返回响应
		if (!m_requestHandler(req, rsp, std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
			sendMessage(rsp);
			close();
		}
		else
		{
			sendMessage(rsp);
		}
	}

	void RockStream::handleNotify(sylar::RockNotify::ptr nty)
	{
		if (!m_notifyHandler(nty
			, std::dynamic_pointer_cast<RockStream>(shared_from_this()))) {
			//innerClose();
			close();
		}
	}

	RockSession::RockSession(Socket::ptr sock):RockStream(sock)
	{
		m_autoConnect = false;
	}

	RockConnection::RockConnection():RockStream(nullptr)
	{
		m_autoConnect = true;
	}

	bool RockConnection::connect(Address::ptr addr)
	{
		m_socket = sylar::Socket::CreateTCP(addr);
		return m_socket->connect(addr);
	}

}
#include "rock_server.h"
#include "../log.h"
#include "../module.h"

namespace sylar {

	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

	sylar::RockServer::RockServer(const std::string& type /*= "rock" */, sylar::IOManager* worker /*= sylar::IOManager::GetThis() */, sylar::IOManager* io_worker /*= sylar::IOManager::GetThis() */, sylar::IOManager* accept_worker /*= sylar::IOManager::GetThis()*/)
		:TcpServer(worker, io_worker, accept_worker)
	{
		m_type = type;
	}

	void sylar::RockServer::handleClient(Socket::ptr client)
	{
		SYLAR_LOG_DEBUG(g_logger) << "handleClient " << *client;
		//根据client创建一个session类
		RockSession::ptr session= std::make_shared<sylar::RockSession>(client);
		//设置工作调度器
		session->setWorker(m_worker);

		session->setDisconnectCb([client](AsyncSocketStream::ptr stream) {
			SYLAR_LOG_INFO(g_logger) << "Client leave " << *client;
			});

		session->setRequestHandler([](sylar::RockRequest::ptr req, sylar::RockResponse::ptr rsp, sylar::RockStream::ptr conn)->bool {
			rsp->setResultStr(req->getBody());
			return true;
			});

		session->start();

	}





}

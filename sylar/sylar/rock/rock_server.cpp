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
		RockSession::ptr session= std::make_shared<sylar::RockSession>(client);
		session->setWorker(m_worker);

		ModuleMgr::GetInstance()->foreach(Module::ROCK, [session](Module::ptr m) {
			m->onConnect(session);
			});

		session->setDisconnectCb([](AsyncSocketStream::ptr stream) {
			ModuleMgr::GetInstance()->foreach(Module::ROCK,
				[stream](Module::ptr m) {
					m->onDisconnect(stream);
				});
			});

		session->setRequestHandler([](RockRequest::ptr req, RockResponse::ptr rsp, RockStream::ptr conn)->bool {
			bool rt = false;
			ModuleMgr::GetInstance()->foreach(Module::ROCK,
				[&rt, req, rsp, conn](Module::ptr m) {
					if (rt) {
						return;
					}
					rt = m->handleRequest(req, rsp, conn);
				});
			return rt;
			});

		session->setNotifyHandler([](RockNotify::ptr nty,RockStream::ptr conn)->bool {
			bool rt = false;
			ModuleMgr::GetInstance()->foreach(Module::ROCK,
				[&rt, nty, conn](Module::ptr m) {
					if (rt) {
						return;
					}
					rt = m->handleNotify(nty, conn);
				});
			return rt;
			});

		session->start();

	}





}

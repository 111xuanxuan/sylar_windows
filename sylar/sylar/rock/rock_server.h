#pragma once
#ifndef __SYLAR_ROCK_SERVER_H__
#define __SYLAR_ROCK_SERVER_H__


#include "rock_stream.h"
#include "../tcp_server.h"

namespace sylar {

	class RockServer :public TcpServer {
	public:
		using ptr = std::shared_ptr<RockServer>;

		RockServer(const std::string& type = "rock"
			, sylar::IOManager* worker = sylar::IOManager::GetThis()
			, sylar::IOManager* io_worker = sylar::IOManager::GetThis()
			, sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

	protected:

		virtual void handleClient(Socket::ptr client)override;

	};

}

#endif
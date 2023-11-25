#include "log.h"
#include "env.h"
#include "config.h"
#include "hook.h"
#include "rock/rock_server.h"
#


#pragma warning (disable: 4996)

using namespace sylar;

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");


void run() {
	auto addr = sylar::Address::LookupAny("192.168.56.1:1234");
	std::vector<sylar::Address::ptr> addrs;
	addrs.push_back(addr);

	RockServer::ptr tcp_server(new RockServer);
	std::vector<sylar::Address::ptr> fails;

	while (!tcp_server->bind(addrs, fails)) {
		s_sleep(2000);
	}

	tcp_server->start();

}

int main(int argc, char** argv) {

	//初始化环境变量
	auto env = EnvMgr::GetInstance();
	env->init(argc, argv);
	env->add("c", "config\\");
	Config::LoadFromConfDir("config\\", true);

	sylar::IOManager iom(2,false);

	iom.schedule(run);


	iom.stop();

	return 0;
}
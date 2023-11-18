#include "log.h"
#include "env.h"
#include "config.h"
#include "tcp_server.h"
#include "hook.h"
#pragma warning (disable: 4996)

using namespace sylar;

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run() {
	auto addr = sylar::Address::LookupAny("192.168.56.1:1234");
	//auto addr2 = sylar::UnixAddress::ptr(new sylar::UnixAddress("/tmp/unix_addr"));
	std::vector<sylar::Address::ptr> addrs;
	addrs.push_back(addr);
	//addrs.push_back(addr2);

	sylar::TcpServer::ptr tcp_server(new sylar::TcpServer);
	std::vector<sylar::Address::ptr> fails;
	while (!tcp_server->bind(addrs, fails)) {
		s_sleep(2);
	}
	tcp_server->start();

}

int main(int argc, char** argv) {

	//初始化环境变量
	auto env = EnvMgr::GetInstance();
	env->init(argc, argv);
	env->add("c", "config\\");
	Config::LoadFromConfDir("config\\", true);


	sylar::IOManager iom(1,false);
	iom.schedule(run);

	iom.stop();
	return 0;
}
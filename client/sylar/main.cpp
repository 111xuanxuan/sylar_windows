#include "endia.h"
#include "client_stream.h"
#pragma  warning(disable:4996)

using namespace sylar;

int main() {

	auto serverAddress = IPv4Address::Create("192.168.56.1", 1234);

	auto s = Socket::CreateTCPSocket();

	u_long mode = 0;
	ioctlsocket(s->getSocket(), FIONBIO, &mode);

	if (!s->connect(serverAddress)) {
		std::cout << "Á¬½ÓÊ§°Ü";
		return 0;
	}

	ClientStream::ptr cs= std::make_shared<ClientStream>(s);

	char tmp[4096]{ 0 };

	while (true)
	{
		scanf("%s", tmp);
		if (strcmp(tmp, "exit") == 0)
			break;

		ByteArray::ptr ba = std::make_shared<ByteArray>();
		ba->write(tmp, strlen(tmp));
		ba->setPosition(0);
		auto req=ClientStream::CreateMessage(Message::REQUEST, ba);
		cs->doSend(req);
		auto res= cs->doRecv();
		if (res->getType() == Message::RESPONSE) {
			std::cout << ClientStream::GetMessageResult(res);
		}


	}



	return 0;
}
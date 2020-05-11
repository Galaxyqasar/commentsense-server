#include <thread>

#include "address.hpp"
#include "tcpsocket.hpp"

using namespace std;
using ipaddress = ipv6address;

int main()
{
	
	tcpserver<ipv4address> server;
	server.bind(ipv4address(ipv4address::Any, 80));
	server.listen(32);
	while(1) {
		ipv4address address;
		tcpclient<ipv4address> client = server.accept(address);

		if(client) {
			std::thread([](tcpclient<ipv4address> client){
				std::string str(1024, '\0');
				client.recv(str);
				client.send(std::string("HTTP/1.1 200 OK\n\n"));
			}, std::move(client)).detach();
		}
	}
	
	tcpclient<ipv4address> client;
	client.connect(ipv4address("commentsense.de", 80));
	client.send(std::string("GET / HTTP/1.1\n\n"));
	std::cout<<client.recvline();
	return 0;
}

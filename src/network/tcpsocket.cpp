#include "tcpsocket.hpp"

namespace network{
	utils::logfile *tcplog;
	void __attribute__((constructor)) initTCP(){
		tcplog = new utils::logfile("./log_network.txt");
		#if defined(WINDOWS)
			WSADATA wsa;
			long r =  WSAStartup(MAKEWORD(2,0),&wsa);
			if(r != 0){
				log(*tcplog, "WSAStartup failed(", r, ")");
				exit(-1);
			}
		#endif
	}
	utils::logfile* getNetworkLog(){
		return tcplog;
	}

	address::address(unsigned v) : val(v){}

#if defined(WINDOWS)
	tcpsocket::tcpsocket(int af, int type, int protocol){
		log(*tcplog, "tcpsocket(", af, ", ", type, ", ", protocol, ")");
		handle = socket(af, type, protocol);
		if(handle == INVALID_SOCKET)
			log(*tcplog, "    creating socket failed: ", WSAGetLastError());
		else
			log(*tcplog, "    socket created: ", WSAGetLastError());
	}
	tcpsocket::tcpsocket(SOCKET s, SOCKADDR_IN addr) : handle{s}, client{addr}{
		;
	}

	void tcpsocket::bind(address addr, unsigned short port){
		log(*tcplog, "tcpsocket::bind(", addr.val, ", ", port, ")");
		SOCKADDR_IN server = {AF_INET, htons(port)};
		server.sin_addr.s_addr = htonl(addr.val);
		memset(&server.sin_zero, 0, 8);
		if(::bind(handle, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
			log(*tcplog, "    couldn't bind socket: ", WSAGetLastError());
	}
	bool tcpsocket::listen(){
		if(::listen(handle, 64) == SOCKET_ERROR){
			log(*tcplog, "    couldn't listen socket: ", WSAGetLastError());
			return false;
		}
		return true;
	}
	tcpsocket* tcpsocket::accept(struct timeval timeout){
		int len = sizeof(client);
		SOCKET s = ::accept(handle, (SOCKADDR*)&client, &len);
		if(s == INVALID_SOCKET)
			log(*tcplog, "error: couldn't accept socket: ", WSAGetLastError());
		else{
			std::string ipaddress = inet_ntoa(client.sin_addr);
			log(*tcplog, "status: client connected from ", getClientIP(), WSAGetLastError());
			setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
		}
		return new tcpsocket(s, client);
	}
	std::string tcpsocket::getClientIP(){
		return inet_ntoa(client.sin_addr);
	}
	void tcpsocket::connect(std::string host, unsigned short port){
		unsigned long addr;

		if((addr = inet_addr(host.c_str())) != INADDR_NONE);
		else{
			hostent *host_info = gethostbyname(host.c_str());
			if(host_info)
				memcpy(&addr, host_info->h_addr, host_info->h_length);
			else{
				log(*tcplog, "error: unknown server: ", WSAGetLastError());
				return;
			}
		}
		SOCKADDR_IN server = {AF_INET, htons(port)};
		server.sin_addr.s_addr = htonl(addr);
		if(::connect(handle, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR)
			log(*tcplog, "error: couldn't connect to server: ", WSAGetLastError());
		else
			log(*tcplog, "connected to server: ", host);
	}
	void tcpsocket::disconnect(){
		if(shutdown(handle, SD_SEND) < 0){
			log(*tcplog, "error: shutdown socket failed", WSAGetLastError());
		}
		if(close(handle) < 0){
			log(*tcplog, "error: close socket failed", WSAGetLastError());
		}
	}

	bool tcpsocket::isConnected(){
		int error = 0;
		int errorlen = sizeof(error);
		if(getsockopt(handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &errorlen) != 0){
			log(*tcplog, "error: socket not connected: ", WSAGetLastError());
			return false;
		}
		return true;
	}

#else

	tcpsocket::tcpsocket(int af, int type, int protocol){
		log(*tcplog, "tcpsocket(", af, ", ", type, ", ", protocol, ")");
		handle = socket(af, type, protocol);
		if(handle < 0)
			log(*tcplog, "    creating socket failed: ", strerror(errno));
		else
			log(*tcplog, "    socket created: ", strerror(errno));
		setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, nullptr, 0);
	}
	tcpsocket::tcpsocket(int handle, struct sockaddr_in addr) : handle(handle), client{addr}{}

	void tcpsocket::bind(address addr, unsigned short port){
		log(*tcplog, "tcpsocket::bind(", addr.val, ", ", port, ")");
		struct sockaddr_in server = {AF_INET, htons(port), {htonl(addr.val)}};
		memset(&server.sin_zero, 0, 8);
		if(::bind(handle, (struct sockaddr*)&server, sizeof(server)) < 0)
			log(*tcplog, "    couldn't bind socket: ", strerror(errno));
	}
	bool tcpsocket::listen(){
		if(::listen(handle, 64) == -1) {
			log(*tcplog, "    couldn't listen socket: ", strerror(errno));
			return false;
		}
		return true;
	}
	tcpsocket* tcpsocket::accept(struct timeval timeout){
		unsigned int len = sizeof(client);
		int new_socket = ::accept(handle, (struct sockaddr *)&client, &len);
		if(new_socket  == -1)
			log(*tcplog, "error: couldn't accept socket: ", strerror(errno));
		else{
			log(*tcplog, "status: client connected from ", getClientIP());
			setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		}
		return new tcpsocket(new_socket, client);
	}
	std::string tcpsocket::getClientIP(){
		std::string ipaddress(16, ' ');
		struct in_addr ipAddr = client.sin_addr;
		inet_ntop(AF_INET, &ipAddr, &ipaddress[0], INET_ADDRSTRLEN);
		return ipaddress.c_str();
	}
	void tcpsocket::connect(std::string host, unsigned short port){
		struct hostent *host_info;
		unsigned long addr;

		if((addr = inet_addr(host.c_str())) != INADDR_NONE);
		else{
			host_info = gethostbyname(host.c_str());
			if(host_info)
				memcpy(&addr, host_info->h_addr, host_info->h_length);
			else{
				log(*tcplog, "error: unknown server: ", strerror(errno));
				return;
			}
		}
		struct sockaddr_in server = {AF_INET, htons(port), {unsigned(addr)}};
		if(::connect(handle, (struct sockaddr*)&server, sizeof(server)) < 0)
			log(*tcplog, "error: couldn't connect to server: ", strerror(errno));
		else
			log(*tcplog, "connected to server: ", host);
	}
	void tcpsocket::disconnect(){
		if(shutdown(handle, SHUT_RDWR) < 0){
			if (errno != ENOTCONN && errno != EINVAL)
				log(*tcplog, "error: shutdown socket failed", strerror(errno));
		}
		if(close(handle) < 0){
			log(*tcplog, "error: close socket failed", strerror(errno));
		}
	}

	bool tcpsocket::isConnected(){
		int error = 0;
		socklen_t errorlen = sizeof(error);
		if(getsockopt(handle, SOL_SOCKET, SO_ERROR, &error, &errorlen) != 0){
			return false;
		}
		return true;
	}

#endif
	tcpsocket::~tcpsocket(){
		if(isConnected())
			disconnect();
	}

	bool tcpsocket::send(std::string data){
		if(::send(handle, data.c_str(), (size_t)data.size(), 0) == -1){
			log(*tcplog, "error: tcp send(): ", strerror(errno));
			return false;
		}
		return true;
	}
	std::string tcpsocket::recv(int len){
		if(isConnected()){
			std::string data(len+1, '\0');
			int size = ::recv(handle, &data[0], len, 0);
			data[size] = '\0';
			return data;
		}
		return "";
	}
	std::string tcpsocket::recvLine(){;
		std::string line = "";
		if(isConnected()){
			char c;
			while(::recv(handle, &c, 1, 0) > 0 && c != '\n'){
				if(c != '\r')
					line.push_back(c);
			}
		}
		return line;
	}
}
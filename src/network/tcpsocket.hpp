#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WINDOWS)
	#include <winsock2.h>
#else
	#include <errno.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
#endif

#include <iostream>
#include <string>

#include "../utils/log.hpp"

#define CHUNK_SIZE 512

namespace network{
	//include unistd.h insde namespace network to avoid conflict between namespace crypt and char* crypt(char*, char*);
	#include <unistd.h>

	utils::logfile* getNetworkLog();

	struct address{
		address(unsigned v = Any);
		enum types : unsigned{
			Any = INADDR_ANY,
			Broadcast = INADDR_BROADCAST,
			LoopBack = INADDR_LOOPBACK,
		#if defined(LINUX)
			UnspecGroup = INADDR_UNSPEC_GROUP,
			AllHostGroup = INADDR_ALLHOSTS_GROUP,
			AllRtrsGroup = INADDR_ALLRTRS_GROUP,
			MaxLocalGroup = INADDR_MAX_LOCAL_GROUP,
		#endif
			None
		};
		unsigned val;
	};

	class tcpsocket{
	public:
		tcpsocket(int af, int type, int protocol);

		#if defined(WINDOWS)
			tcpsocket(SOCKET s, SOCKADDR_IN addr);
		#else
			tcpsocket(int handle, struct sockaddr_in addr);
		#endif

		virtual ~tcpsocket();

		void bind(address addr = address::Any, unsigned short port = 80);
		virtual bool listen();
		virtual tcpsocket* accept(struct timeval timeout);
		virtual void connect(std::string host, unsigned short port = 80);
		virtual void disconnect();

		bool isConnected();
		std::string getError();
		std::string getClientIP();

		virtual bool send(std::string data);
		virtual std::string recv(int len);
		virtual std::string recvLine();
	private:
		#if defined(WINDOWS)
			SOCKET handle;
			SOCKADDR_IN client;
		#else
			int handle;
			struct sockaddr_in client;
		#endif
	};
}
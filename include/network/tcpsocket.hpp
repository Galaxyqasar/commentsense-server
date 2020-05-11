#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>

#if defined(__USE_MISC)
	#undef __USE_MISC
	#include <unistd.h>
	#define __USE_MISC
#else
	#include <unistd.h>
#endif

#include <algorithm>
#include <iostream>
#include <limits>
#include <string>
#include <chrono>

namespace inet {
	template<typename address_t>
	class tcpsocket {
	public:
		tcpsocket(int type, int protocol) {
			handle = socket(address_t::family(), type, protocol);
		}
		tcpsocket(tcpsocket &&other) : handle(other.handle) {
			other.handle = -1;
		}
		~tcpsocket() {
			shutdown();
			close();
		}
		operator bool() {
			return  handle >= 0;
		}
		tcpsocket& operator=(tcpsocket &&other) {
			std::swap(other.handle, handle);
			return *this;
		}
		bool connected() {
			int error = 0;
			socklen_t errorlen = sizeof(error);
			return getopt(SOL_SOCKET, SO_ERROR, &error, &errorlen) == 0;
		}
		bool shutdown(int method = SHUT_RDWR) {
			if(handle >= 0) {
				return ::shutdown(handle, method) >= 0;
			}
			return false;
		}
		bool close() {
			if(handle >= 0) {
				return ::close(handle) >= 0;
			}
			return false;
		}
		int setopt(int level, int option, const void *value, socklen_t size) {
			return setsockopt(handle, level, option, value, size);
		}
		int getopt(int level, int option, const void *value, socklen_t *size) {
			return getsockopt(handle, level, option, value, size);
		}

	protected:
		tcpsocket(int handle) : handle(handle) {}
		int handle;
	private:
		tcpsocket(const tcpsocket<address_t> &other) = delete;	
		tcpsocket<address_t>& operator=(const tcpsocket<address_t> &other) = delete;	
	};

	template<typename address_t>
	class tcpclient : public tcpsocket<address_t> {
		using base = tcpsocket<address_t>;
	public:
		tcpclient() : base(SOCK_STREAM, 0) {}
		tcpclient(int handle) : base(handle) {}

		void setRecvTimeout(std::chrono::duration<int> timeout) {
			std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
			struct timeval t{seconds.count(), microseconds.count()};
			base::setopt(SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
		}

		void setSendTimeout(std::chrono::duration<int> timeout) {
			std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
			struct timeval t{seconds.count(), microseconds.count()};
			base::setopt(SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));
		}

		bool connect(address_t server) {
			return ::connect(base::handle, server.addrptr(), server.addrsize()) >= 0;
		}
		void disconnect() {
			base::shutdown();
			base::close();
		}
		size_t recv(char *buffer, size_t size) {
			return ::recv(base::handle, buffer, size, MSG_NOSIGNAL);
		}
		inline void recv(std::string &buffer) {
			size_t size = recv(&buffer[0], buffer.size());
			if(size < buffer.size()) {
				buffer.resize(size);
			}
		}
		inline size_t recvchar(char &c) {
			return recv(&c, 1);
		}
		inline char recvchar() {
			char c;
			recvchar(c);
			return c;
		}
		inline std::string recv(size_t size) {
			std::string buffer(size, '\0');
			recv(buffer);
			return buffer;
		}
		std::string recvline(char delim = '\n', size_t max = std::numeric_limits<size_t>::max()) {
			std::string buffer;
			buffer.reserve(std::min(max, size_t(1024)));
			char c;
			while(recvchar(c) == 1 && c != delim && buffer.size() + 1 < max) {
				if(c != '\r')
					buffer.push_back(c);
			}
			return buffer;
		}
		size_t send(const char *buffer, size_t size) {
			return ::send(base::handle, buffer, size, MSG_NOSIGNAL);
		}
		inline size_t send(const std::string &buffer) {
			return send(buffer.c_str(), buffer.size());
		}
		inline size_t sendchar(char c) {
			return send(&c, 1);
		}
	};

	template<typename address_t>
	class tcpserver : public tcpsocket<address_t> {
		using base = tcpsocket<address_t>;
	public:
		tcpserver() : base(SOCK_STREAM, 0) {}
		bool bind(address_t addr) {
			return ::bind(base::handle, addr.addrptr(), addr.addrsize()) >= 0;
		}
		bool listen(size_t queuesize = 4) {
			if(::listen(base::handle, queuesize) >= 0) {
				FD_ZERO(&master);
				FD_ZERO(&readfds);
				FD_SET(base::handle, &master);
				return true;
			}
			return false;
		}
		tcpclient<address_t> accept(std::chrono::duration<int> timeout, address_t *addr = nullptr) {
			std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
			struct timeval t{seconds.count(), microseconds.count()};
			memcpy(&readfds, &master, sizeof(master));
			int nready = select(base::handle + 1, &readfds, nullptr, nullptr, &t);
			for (int i = 0; i <= base::handle && nready > 0; i++) {
				if (FD_ISSET(i, &readfds)) {
					nready--;
					if(i == base::handle) {
						if(addr) {
							unsigned size = addr->addrsize();
							return tcpclient<address_t>(::accept(base::handle, addr->addrptr(), &size));
						}
						else {
							return tcpclient<address_t>(::accept(base::handle, nullptr, nullptr));
						}
					}
				}
			}
			return tcpclient<address_t>(-1);
		}
	private:
		fd_set master, readfds;
	};
}
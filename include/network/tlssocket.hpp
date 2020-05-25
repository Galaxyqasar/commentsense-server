#pragma once

#include "tcpsocket.hpp"
#include "tlse.h"

namespace inet {
	template<typename address_t>
	class tlsclient : public tcpclient<address_t> {
		using base = tcpsocket<address_t>;
	public:
		tlsclient() : tcpclient<address_t>(), ctx(nullptr) {}
		tlsclient(int handle, SSL *server) : tcpclient<address_t>(handle) {
			ctx = SSL_new(server);
			SSL_set_fd(ctx, handle);
			if(!SSL_accept(ctx)) {
				SSL_free(ctx);	// handshake fails
				ctx = nullptr;
			}
		}
		tlsclient(tlsclient<address_t> &&other) : tcpclient<address_t>(other), ctx(other.ctx) {
			other.ctx = nullptr;
		}
		~tlsclient() {
			if(ctx) {
				SSL_free(ctx);
			}
		}
		bool connect(address_t server) {
			return ::connect(base::handle, server.addrptr(), server.addrsize()) >= 0;
		}
		void disconnect() {
			if(ctx) {
				SSL_shutdown(ctx);
			}
			base::shutdown();
			base::close();
		}
		size_t recv(char *buffer, size_t size) {
			return SSL_read(ctx, buffer, size);
		}
		size_t send(const char *buffer, size_t size) {
			return SSL_write(ctx, buffer, size);
		}
	private:
		SSL *ctx;
	};

	template<typename address_t>
	class tlsserver : public tcpserver<address_t> {
		using base = tcpsocket<address_t>;
		using server = tcpserver<address_t>;
	public:
		tlsserver(std::string certpath, std::string privkeypath) : tcpserver<address_t>() {
			server_ctx = SSL_CTX_new(SSLv3_server_method());
			SSL_CTX_use_certificate_file(server_ctx, certpath.c_str(), SSL_SERVER_RSA_CERT);
			SSL_CTX_use_PrivateKey_file(server_ctx, privkeypath.c_str(), SSL_SERVER_RSA_KEY);
			if (!SSL_CTX_check_private_key(server_ctx)) {
				fprintf(stderr, "Private key not loaded\n");
				SSL_CTX_free(server_ctx);
				server_ctx = nullptr;
			}
		}
		~tlsserver() {
			if(server_ctx) {
				SSL_CTX_free(server_ctx);
			}
		}
		tlsclient<address_t>* accept(std::chrono::duration<int> timeout, address_t *addr = nullptr) {
			std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
			std::chrono::microseconds microseconds = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
			struct timeval t{seconds.count(), microseconds.count()};
			memcpy(&(server::readfds), &(server::master), sizeof(server::master));
			int nready = select(base::handle + 1, &(server::readfds), nullptr, nullptr, &t);
			for (int i = 0; i <= base::handle && nready > 0; i++) {
				if (FD_ISSET(i, &(server::readfds))) {
					nready--;
					if(i == base::handle) {
						if(addr) {
							unsigned size = addr->addrsize();
							return new tlsclient<address_t>(::accept(base::handle, addr->addrptr(), &size), server_ctx);
						}
						else {
							return new tlsclient<address_t>(::accept(base::handle, nullptr, nullptr), server_ctx);
						}
					}
				}
			}
			return nullptr;
		}
	private:
		SSL *server_ctx = nullptr;
	};
}
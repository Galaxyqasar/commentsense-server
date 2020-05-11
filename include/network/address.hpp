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

#if defined(__USE_MISC)
	#undef __USE_MISC
	#include <unistd.h>
	#define __USE_MISC
#else
	#include <unistd.h>
#endif

#include <string>

namespace inet {
	class address {
	public:
		virtual std::string to_string() = 0;
		virtual sockaddr* addrptr() = 0;
		virtual size_t addrsize() = 0;
		static sa_family_t family();
	};

	class ipv4address : public address {
	public:
		enum inaddr {
			Any = INADDR_ANY,
			Loopback = INADDR_LOOPBACK,
			Broadcast = INADDR_BROADCAST,
			None = INADDR_NONE
		};
		inline ipv4address() : addr{AF_INET} {}
		inline ipv4address(uint32_t addr, uint16_t port) : addr{family(), htons(port), {htonl(addr)}} {}
		ipv4address(std::string host, uint16_t port);

		std::string to_string();

		inline static constexpr sa_family_t family() {
			return AF_INET;
		}
		inline sockaddr* addrptr() {
			return reinterpret_cast<sockaddr*>(&addr);
		}
		inline size_t addrsize() {
			return sizeof(addr);
		}
	private:
		sockaddr_in addr;
	};

	class ipv6address : public address {
	public:
		static constexpr in6_addr Any = IN6ADDR_ANY_INIT;
		static constexpr in6_addr Loopback = IN6ADDR_LOOPBACK_INIT;

		inline ipv6address() : addr{AF_INET6} {}
		inline ipv6address(in6_addr addr, uint16_t port) : addr{family(), htons(port), 0, addr, 0} {}
		ipv6address(std::string host, uint16_t port);

		std::string to_string();

		inline static constexpr sa_family_t family() {
			return AF_INET6;
		}
		inline sockaddr* addrptr() {
			return reinterpret_cast<sockaddr*>(&addr);
		}
		inline size_t addrsize() {
			return sizeof(addr);
		}
	private:
		sockaddr_in6 addr;
	};
}

#include <network/address.hpp>

namespace inet {
	ipv4address::ipv4address(std::string host, uint16_t port) {
		static constexpr addrinfo hints{0, family(), SOCK_STREAM, 0};
		addrinfo *result = nullptr;
		if(getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) == 0) {
			for(addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
				if(rp->ai_family == family()) {
					int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
					if (sfd == -1) {
						continue;
					}
					if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
						memcpy(&addr, rp->ai_addr, rp->ai_addrlen);
						close(sfd);
						break;
					}
					close(sfd);
				}
			}
			freeaddrinfo(result);
		}
	}

	std::string ipv4address::to_string() {
		std::string ipaddress(16, ' ');
		inet_ntop(family(), &addr.sin_addr, &ipaddress[0], INET_ADDRSTRLEN);
		return ipaddress;
	} 
	ipv6address::ipv6address(std::string host, uint16_t port) {
		static constexpr addrinfo hints{0, family(), SOCK_STREAM, 0};
		addrinfo *result = nullptr;
		if(getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) == 0) {
			for(addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
				if(rp->ai_family == family()) {
					int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
					if (sfd == -1) {
						continue;
					}
					if(connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
						memcpy(&addr, rp->ai_addr, rp->ai_addrlen);
						close(sfd);
						break;
					}
					close(sfd);
				}
			}
			freeaddrinfo(result);
		}
	}
	std::string ipv6address::to_string() {
		std::string ipaddress(45, ' ');
		inet_ntop(family(), &addr.sin6_addr, &ipaddress[0], INET_ADDRSTRLEN);
		return ipaddress;
	}
}
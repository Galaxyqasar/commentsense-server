#include "tlssocket.hpp"

namespace network{
	utils::logfile *tlslog;
	void __attribute__((constructor)) initTLS(){
		tlslog = getNetworkLog();
	}
}
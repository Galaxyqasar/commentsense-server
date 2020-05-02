#pragma once

#include <string>

namespace crypt {
	std::string rijndael(std::string source, std::string key, int mode);
}
#pragma once

#include <string>
#include <vector>
#include <math.h>

#include "../math/math.hpp"

namespace crypt{
	enum mode{
		encrypt = 1,
		decrypt = -1
	};
	std::string ceasar(std::string source, int offset, int mode);
	std::string vigenere(std::string source, std::string password, int mode);
}
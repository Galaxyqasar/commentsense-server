#pragma once

#include <stdio.h>
#include <time.h>

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

#include "../math/math.hpp"

typedef unsigned char byte;

inline bool isascii(char c){
	return math::clamp(int(c), 0x00, 0x7F) == c;
}

namespace utils {	
	std::vector<std::string> split(const std::string& s, char delimiter);
	std::vector<std::string> split(const std::string& str, std::string delimiter);
	std::string join(const std::vector<std::string> parts, const char delimiter);
	std::string removeAll(std::string str, std::string chars);
	std::string stringToHex(std::string str, bool space = false);
	std::string stringFromHex(std::string hex);

	std::string replace(std::string &str, const std::string& from, const std::string& to);

	std::string inflate(std::string str);
	std::string deflate(std::string str);

	template<typename T>
	T min(T a, T b){
		return a < b ? a : b;
	}
	template<typename T>
	T max(T a, T b){
		return a > b ? a : b;
	}

	inline std::string toLower(std::string str){
		std::transform(str.begin(), str.end(), str.begin(), [](char c) -> char{{ return (c <= 'Z' && c >= 'A') ? c - ('Z' - 'z') : c;}});
		return str;
	}

	inline std::string toUpper(std::string str){
		std::transform(str.begin(), str.end(), str.begin(), [](char c) -> char{{ return (c <= 'z' && c >= 'a') ? c + ('Z' - 'z') : c;}});
		return str;
	}
}

#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include "sys.hpp"
#include "utils.hpp"

namespace utils{
	class logfile{
	public:
		logfile(std::string fileName);
		~logfile();
		void operator<<(std::string str);
	private:
		std::ofstream f;
	};

	template<typename T>
	T adder(T v){
		return v;
	}

	template<typename T, typename... Args>
	std::string adder(T first, Args... args){
		std::stringstream ss;
		ss<<first<<adder(args...);
		return ss.str();
	}

	template<typename... Args>
	void log(logfile& log, Args... args){
		//["<<name<<"]\t\""<<str<<"\" || \""<<stringToHex(str, true)<<
		log<<adder("[", sys::getTimeStr(), "] ", args...);
	}
}
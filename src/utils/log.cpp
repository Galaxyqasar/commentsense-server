#include "log.hpp"


namespace utils{
//#if !defined(NO_LOG)
	logfile::logfile(std::string fileName) : f(fileName){
	}
	logfile::~logfile(){
		f.close();
	}
	void logfile::operator<<(std::string str){
		f<<str<<"\n";
		f.flush();
	}/*
#else
	logfile::logfile(std::string fileName){}
	logfile::~logfile(){}
	void logfile::operator<<(std::string str){}
#endif*/
}
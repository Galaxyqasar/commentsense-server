#pragma once

#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>

namespace utils{
	class file{
	public:
		file(std::string fileName = "");
		~file();
	    void setFileName(std::string file);
	    bool exists();
	    static bool exists(std::string filename);
		bool open(std::string mode, std::string fileName = "");
		std::string readAll();
		static std::string readAll(std::string fileName);
		std::string read(unsigned long len);
		void write(std::string data);
		static void write(std::string filename, std::string data);
		void close();
	    
	    unsigned long size();
	    
	    bool isDir();
	    bool isOpen();
	private:
		std::string fileName;
		FILE *m_file;
		bool m_isOpen = false;
	};
}
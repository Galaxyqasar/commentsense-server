#pragma once

#include <dlfcn.h>
#include <string>
#include <iostream>

namespace utils{
	class dll{
	public:
		dll(std::string name = "");
		dll(const dll& other);
		~dll();
		dll& operator=(const dll& other);
		void open(std::string name);
		template <class T>
		T get(std::string symbolname){
			dlerror();
			if(handle){
				T func = reinterpret_cast<T>(dlsym(handle, symbolname.c_str()));
				if(!func)
					std::cout<<"error: couldn't get function \""<<dlerror()<<"\" from dll "<<m_name<<std::endl;
				return func;
			}
			std::cout<<"error: dll not loaded "<<std::endl;
			return nullptr;
		}
		std::string name();
		std::string error();
		bool isOpen();
		void close();
	private:
		void *handle = nullptr;
		std::string m_name;
	};
}
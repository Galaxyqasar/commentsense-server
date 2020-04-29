#pragma once

#include <dlfcn.h>
#include <string>
#include <iostream>

#include <spdlog/spdlog.h>

namespace utils{
	class dll{
	public:
		dll(std::string name = "");
		~dll();
		void open(std::string name);
		template <class T>
		T get(std::string symbolname){
			dlerror();
			if(handle){
				T func = reinterpret_cast<T>(dlsym(handle, symbolname.c_str()));
				if(!func)
					spdlog::error("couldn't load symbol: {}", dlerror());
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
		dll(const dll& other);
		dll& operator=(const dll& other);
		void *handle = nullptr;
		std::string m_name;
	};
}
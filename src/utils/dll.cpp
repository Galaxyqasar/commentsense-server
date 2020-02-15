#include "dll.hpp"

namespace utils{
	dll::dll(std::string name){
		m_name = name;
		if(name != ""){
			open(name);
		}
	}
	dll::dll(const dll& other){
		handle = other.handle;
		m_name = other.m_name;
	}
	dll::~dll(){
	}
	dll& dll::operator=(const dll& other){
		handle = other.handle;
		m_name = other.m_name;
		return *this;
	}
	void dll::open(std::string name){
		handle = dlopen(name.c_str(), RTLD_NOW | RTLD_GLOBAL);
		if(!handle){
			std::cout<<"error: "<<dlerror()<<"\n";
		}
	}
	std::string dll::name(){
		return m_name;
	}
	std::string dll::error(){
		char *e = dlerror();
		return (e == NULL ? "" : e);
	}
	bool dll::isOpen(){
		return (handle != nullptr);
	}
	void dll::close(){
		if(handle){
			dlclose(handle);
			handle = nullptr;
		}
	}
}
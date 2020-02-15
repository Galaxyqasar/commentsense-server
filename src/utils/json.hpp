#pragma once

#include <string.h>
#include <climits>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include "utils.hpp"

namespace utils{
	class json{
	public:
		enum types{
			null_t,
			object_t,
			array_t,
			string_t,
			number_t,
			boolean_t,
			minified = INT_MIN
		};

		json(std::string name, int type, double num, std::string str, std::vector<json> elements);
		json(const json &other);
		json();
		json& operator=(const json& other);

		json& operator[](std::string key);
		json& operator[](int index);
		json& operator+=(json element);

		std::string print(int offset = 0);

		static json parse(std::string source);

		static json null(std::string name = "");
		static json object(std::string name, std::vector<json> elements);
		static json array(std::string name, std::vector<json> elements);
		static json string(std::string name, std::string value, bool deflate = true);
		static json number(std::string name, double value);
		static json boolean(std::string name, bool value);

		std::string& valueStr();
		double& valueNum();
		std::vector<json> valueList();

		int toInt();
		std::string toStr();
		std::string& name();

		std::vector<json>::iterator begin();
		std::vector<json>::iterator end();
		inline void remove(unsigned long i){
			if(isContainer() && i < elements.size()){
				elements.erase(elements.begin() + 1);
			}
		}
		inline int childs(){
			return elements.size();
		}

		inline bool isStr(){
			return type == string_t;
		}
		inline bool isNum(){
			return type == number_t;
		}
		inline bool isBool(){
			return type == boolean_t;
		}
		inline bool isObject(){
			return type == object_t;
		}
		inline bool isArray(){
			return type == array_t;
		}
		inline bool isContainer(){
			return isObject() || isArray();
		}
		inline bool isNull(){
			return type == null_t;
		}
	private:
		std::string m_name;
		int type = null_t;

		double num;					//I'm a number
		std::string str;			//I'm a string
		std::vector<json> elements;	//I'm an array/object
	};
}
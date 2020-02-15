#include "json.hpp"

namespace utils{
	json json_err = json::null();

	inline void indent(std::stringstream &result, int amnt){
		for(int i = 0; i < amnt; i++)
			result<<"    ";
	}

	json::json(std::string name, int type, double num, std::string str, std::vector<json> elements){
		this->m_name = name;
		this->type = type;
		this->num = num;
		this->str = str;
		this->elements = elements;
	}
	json::json(const json &other){
		m_name = other.m_name;
		type = other.type;
		num = other.num;
		str = other.str;
		elements = other.elements;
	}
	json::json(){
		this->operator=(json::null());
	}

	json& json::operator=(const json& other){
		m_name = other.m_name;
		type = other.type;
		num = other.num;
		str = other.str;
		elements = other.elements;
		return *this;
	}

	json& json::operator[](std::string key){
		if(type == object_t){
			for(json &e : elements){
				if(e.m_name == key){
					return e;
				}
			}
			return json_err;
		}
		else{
			return json_err;
		}
	}
	json& json::operator[](int index){
		if(type == array_t){
			return elements[index];
		}
		else{
			return json_err;
		}
	}
	json& json::operator+=(json element){
		if(type == array_t || type == object_t){
			elements.push_back(element);
		}
		return *this;
	}

	json json::null(std::string name){
		return json(name, null_t, 0, "", {});
	}
	json json::object(std::string name, std::vector<json> elements){
		return json(name, object_t, 0, "", elements);
	}
	json json::array(std::string name, std::vector<json> elements){
		return json(name, array_t, 0, "", elements);
	}
	json json::string(std::string name, std::string value, bool deflate){
		return json(name, string_t, 0, deflate ? utils::deflate(value) : value, {});
	}
	json json::number(std::string name, double value){
		return json(name, number_t, value, "", {});
	}
	json json::boolean(std::string name, bool value){
		return json(name, boolean_t, (int)value, "", {});
	}

	std::string& json::valueStr(){
		return str;
	}
	double& json::valueNum(){
		return num;
	}
	std::vector<json> json::valueList(){
		return elements;
	}

	int json::toInt(){
		if(type == number_t)
			return num;
		else if(type == string_t)
			return std::atoi(str.c_str());
		else if(type == boolean_t)
			return int(num);
		return -1;
	}

	std::string json::toStr(){
		if(type == string_t)
			return str;
		else if(type == number_t)
			return std::to_string(num);
		return print(minified);
	}

	std::string& json::name(){
		return m_name;
	}

	std::vector<json>::iterator json::begin(){
		return elements.begin();
	}
	std::vector<json>::iterator json::end(){
		return elements.end();
	}

	std::string json::print(int offset){
		std::stringstream result;
		std::string nl = (offset >= 0 ? "\n" : "");
		indent(result, offset);
		if(m_name != "")
			result<<"\""<<m_name.c_str()<<"\":";
		if(type == object_t){
			char separator = '{';
			for(json &e : elements){
				result<<separator<<nl<<e.print(offset + 1);
				separator = ',';
			}
			if(elements.size() == 0)
				result<<"{";
			result<<nl;
			indent(result, offset);
			result<<"}";
		}
		else if(type == array_t){
			char separator = '[';
			for(json &e : elements){
				result<<separator<<nl<<e.print(offset + 1);
				separator = ',';
			}
			if(elements.size() == 0)
				result<<"[";
			result<<nl;
			indent(result, offset);
			result<<"]";
		}
		else if(type == string_t)
			result<<"\""<<utils::inflate(str)<<"\"";
		else if(type == number_t)
			result<<num;
		else if(type == boolean_t)
			result<<(num == 1 ? "true" : "false");
		else if(type == null_t)
			result<<"null";
		return result.str();
	}

	json json::parse(std::string source){
		//get name of current object
		while(::isspace(source[0])){
			source.erase(0, 1);
		}
		while(::isspace(source.back()) || source.back() == '\0'){
			source.pop_back();
		}
		std::string m_name = "";
		size_t begin, end;
		begin = source.find('\"');
		end = source.find('\"', begin + 1);
		if(begin == 0 && end > begin && end != std::string::npos){
			m_name = std::string(source.begin() + begin + 1, source.begin() + end);
			if(source.length() >= 2 && m_name == std::string(source.begin() + 1, source.end() - 1)){
				return json::string("", m_name);
			}
			source.erase(begin, end + 2);
			while(::isspace(source[0])){
				source.erase(0, 1);
			}
		}
		//get value
		if((source.front() == '{' && source.back() == '}') || (source.front() == '[' && source.back() == ']')){	//I'm an object or an array
			std::vector<json> elements;
			char c = 0;
			size_t begin = 1;
			for(unsigned i = 1; i < source.length()-1; i++){
				c = source[i];
				//skip sub elements
				if(c == '{'){
					int depth = 1;
					while(depth > 0){
						if(source[++i] == '}')
							depth--;
						else if(source[i] == '{')
							depth++;
					}
				}
				else if(c == '['){
					int depth = 1;
					while(depth > 0){
						if(source[++i] == ']')
							depth--;
						else if(source[i] == '[')
							depth++;
					}
				}
				else if(c == '\"'){
					while(source[++i] != '\"'){
						if(source[i] == '\\')
							i++;
					}
				}

				if(c == ','){	//end of current element
					std::string childsource(source.begin() + begin, source.begin() + i);
					elements.push_back(json::parse(childsource));
					begin = i + 1;
				}
			}
			std::string childsource(source.begin() + begin, source.end() - 1);
			if(childsource != "")
				elements.push_back(json::parse(childsource));
			if(source.front() == '{')
				return object(m_name, elements);
			else
				return array(m_name, elements);
		}
		else if(source != ""){//I'm a string or number
			if(source.front() == '\"' && source.back() == '\"'){	//I'm a string
				return json::string(m_name, std::string(source.begin() + 1, source.end() - 1));
			}
			else{ //I'm a number or bool
				if(source == "true")
					return boolean(m_name, true);
				if(source == "false")
					return boolean(m_name, false);
				return number(m_name, atof(source.c_str()));
			}
		}
		else{
			return null();
		}
	}
}
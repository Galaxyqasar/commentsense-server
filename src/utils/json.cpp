#include <utils/json.hpp>

// parse a string
//	+ really fast
//	- entire source needs to be in memory at the same time
json::json(const char *str, size_t length) {
	while(::isspace(*str)) str++, length--;		//"remove" whitespaces from begin
	while(::isspace(str[length-1])) length--;	// and end
	this->val = std::monostate();
	if(!length)
		return;
	switch(str[0]) {
		case '{': {	//start of an object
			if(str[length-1] == '}') {
				str++, length--;
				val = json::object();
				for(size_t i = 0; i < length; i++) {
					switch(str[i]) {
						case '{': {
							int depth = 1;
							while(depth > 0){
								if(str[++i] == '}'){
									depth--;
								}
								else if(str[i] == '{'){
									depth++;
								}
							}
						} break;
						case '[': {
							int depth = 1;
							while(depth > 0){
								if(str[++i] == ']'){
									depth--;
								}
								else if(str[i] == '['){
									depth++;
								}
							}
						} break;
						case '"': {
							while(str[++i] != '\"'){
								if(str[i] == '\\'){
									i++;
								}
							}
						} break;
						case '}':
						case ',': {
							if(i) {
								const char *key = str;
								size_t keylen = 0;
								while(key[keylen] != ':') keylen++;
								const char *val = key + keylen + 1;
								size_t vallen = i - (val - key);

								while(::isspace(*key)) key++, keylen--;
								while(::isspace(key[keylen-1])) keylen--;

								std::get<json::object>(this->val)[std::string(key+1, keylen-2)] = json(val, vallen);
								key = nullptr;
								keylen = 0;
								length -= i + 1;
								str += i + 1;
								i = -1;
							}
						} break;
					}
				}
			}
		} break;
		case '[': {	//start of an array
			if(str[length-1] == ']') {
				str++, length--;
				this->val = json::array();
				for(size_t i = 0; i < length; i++) {
					switch(str[i]) {
						case '{': {
							int depth = 1;
							while(depth > 0){
								if(str[++i] == '}'){
									depth--;
								}
								else if(str[i] == '{'){
									depth++;
								}
							}
						} break;
						case '[': {
							int depth = 1;
							while(depth > 0){
								if(str[++i] == ']'){
									depth--;
								}
								else if(str[i] == '['){
									depth++;
								}
							}
						} break;
						case '"': {
							while(str[++i] != '\"'){
								if(str[i] == '\\'){
									i++;
								}
							}
						} break;
						case ']':
						case ',': {
							if(i) {
								const char *val = str;
								size_t vallen = i;

								while(::isspace(*val)) val++, vallen--;
								while(::isspace(val[vallen-1])) vallen--;

								std::get<json::array>(this->val).push_back(json(val, vallen));
								length -= i + 1;
								str += i + 1;
								i = -1;
							}
						} break;
					}
				}
			}
		} break;
		case '"': {	//start of an string
			if(length >= 2) {
				this->val = deflate(std::string(str+1, length-2));
			}
		} break;
		default: {
			if(strncmp(str, "true", 4) == 0) {
				this->val = true;
			}
			else if(strncmp(str, "false", 5) == 0) {
				this->val = true;
			}
			else if(strncmp(str, "null", 4) == 0) {
				this->val = std::monostate();
			}
			else {
				this->val = strtod(str, NULL);
			}
		}
	}
}

json& json::operator[](std::string key) {
	static json error;
	error = json();
	if(isObject()) {
		return std::get<object>(val)[key];
	}
	return error;
}
json& json::operator[](size_t index) {
	static json error;
	error = json();
	if(isArray() && std::get<array>(val).size() > index) {
		return std::get<array>(val)[index];
	}
	return error;
}

std::string json::print(size_t offset) {
	std::stringstream str;
	print(str, offset);
	return str.str();
}
void json::print(std::ostream &output, int offset) {
	switch(val.index()) {
		case 0: {
			output<<"null";
		} break;
		case 1: {
			output<<std::string("{")<<(offset >= 0 ? "\n" : "");
			std::string sep, space;
			for(int i = 0; i < offset; i++) {
				space += '\t';
			}
			for(std::pair<const std::string, json> &c : std::get<json::object>(val)) {
				output<<sep<<space<<'\"'<<c.first<<"\":";
				c.second.print(output, offset + (offset > 0));
				sep = std::string(",") + (offset >= 0 ? "\n" : "");
			}
			output<<(offset >= 0 ? "\n" : "")<<space.substr(0,space.length()-1)<<"}";
		} break;
		case 2: {
			output<<'['<<(offset >= 0 ? "\n" : "");
			std::string sep, space;
			for(int i = 0; i < offset; i++) {
				space += '\t';
			}
			for(json &j : std::get<std::vector<json>>(val)) {
				output<<sep<<space;
				j.print(output, offset + (offset > 0));
				sep = std::string(",") + (offset >= 0 ? "\n" : "");
			}
			output<<(offset >= 0 ? "\n" : "")<<space.substr(0,space.length()-1)<<']';
		} break;
		case 3: {
			output<<'\"'<<inflate(std::get<std::string>(val))<<'\"';
		} break;
		case 4: {
			output<<(std::get<bool>(val) ? "true" : "false");
		} break;
		case 5: {
			output<<std::get<double>(val);
		} break;
		default: {
			output<<"error";
		}
	}
}

std::ostream& operator<<(std::ostream &stream, json& data) {
	data.print(stream, json::minified);
	return stream;
}
#include <crypt/crypt.hpp>

namespace crypt{
	std::string ceasar(std::string source, int offset, int mode){
		std::string result(source.size(), '\0');
		for(unsigned i = 0; i < source.size(); i++){
			result[i] = int(source[i]) - (offset * mode);
		}
		return result;
	}
	std::string vigenere(std::string source, std::string password, int mode){
		if(password.length()) {
			std::string result(source.size(), '\0');
			for(unsigned i = 0; i < source.size(); i++){
				result[i] = int(source[i]) - (int(password[i%password.size()]) * mode);
			}
			return result;
		}
		return source;
	}
}

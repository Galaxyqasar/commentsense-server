#include <utils/utils.hpp>

namespace utils{
	std::string stringToHex(std::string str, bool space){
		if(str.length() > 0){
			std::string hex(str.size()*(2+space)-space, ' ');
			for(unsigned int i = 0; i < str.size(); i++){
				unsigned char c = static_cast<unsigned char>(str[i]);
				unsigned char higher = c>>4;
				unsigned char lower = static_cast<unsigned char>(c<<4);
				lower >>= 4;
				higher += higher > 9 ? 7 : 0;
				lower += lower > 9 ? 7 : 0;
				hex[i*(2+space)] = char(higher+48);
				hex[i*(2+space)+1] = char(lower+48);
			}
			return hex;
		}
		return "00";
	}

	std::string stringFromHex(std::string hex){
		std::string str(hex.size()/2, '\0');
		for(unsigned int i = 0; i < str.size(); i++){
			int higher = hex[i*2] -48;
			int lower = hex[i*2+1] - 48;
			higher -= higher > 9 ? 7 : 0;
			lower -= lower > 9 ? 7 : 0;
			
			int c = (higher<<4) + lower;
			str[i] = char(c);
		}
		return str;
	}
}

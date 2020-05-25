#include <crypt/rijndael.hpp>
#include <crypt/crypt.hpp>
#include <crypt/sha.hpp>
#include <utils/string.hpp>	// stringFromHex
#include <tomcrypt/tomcrypt.h>

namespace crypt {
	std::string rijndael(std::string source, std::string key, int mode) {
		const size_t blocksize = 16, blocks = source.length() / blocksize + (source.length() % blocksize ? 1 : 0);
		source.resize(blocks * blocksize, '\0');
		unsigned char *src = reinterpret_cast<unsigned char*>(&source[0]);
		
		symmetric_key k;
		if(rijndael_setup(reinterpret_cast<const unsigned char*>(key.c_str()), key.length(), 0, &k) == CRYPT_OK) {
			for(unsigned i = 0; i < blocks; i++) {
				if(mode == encrypt) {
					rijndael_ecb_encrypt(src, src, &k);
				}
				else if(mode == decrypt) {
					rijndael_ecb_decrypt(src, src, &k);
				}
				src += blocksize;
			}
		}
		return source;
	}
	std::string rijndael(std::string source, std::string key, int mode, std::string iv) {
		if(find_cipher("rijndael") < 0) {
			register_cipher(&rijndael_desc);
		}
		const size_t blocksize = 16, blocks = source.length() / blocksize + (source.length() % blocksize ? 1 : 0);
		source.resize(blocks * blocksize, '\0');
		unsigned char *src = reinterpret_cast<unsigned char*>(&source[0]);
		const unsigned char *keyptr = reinterpret_cast<const unsigned char*>(key.c_str());
		const unsigned char *ivptr = reinterpret_cast<const unsigned char*>(iv.c_str());
		std::cout<<key.length()<<" "<<iv.length()<<" "<<source.length()<<"\n"<<find_cipher("rijndael")<<"\n";
		symmetric_CBC k;
		if(cbc_start(find_cipher("rijndael"), ivptr, keyptr, key.length(), 1, &k) == CRYPT_OK) {
			if(mode == encrypt) {
				cbc_encrypt(src, src, source.length(), &k);
			}
			else if(mode == decrypt) {
				cbc_decrypt(src, src, source.length(), &k);
			}
			cbc_done(&k);
		}
		return source;
	}
}

#include "file.hpp"

namespace utils{
	file::file(std::string fileName){
	    this->fileName = fileName;
	}

	file::~file(){
		if(m_isOpen)
			close();
	}

	void file::setFileName(std::string name)
	{
	    fileName = name;
	}

    bool file::exists(){
    	bool r = false;
    	FILE *f = fopen(this->fileName.c_str(), "rb");
    	if(f){
    		r = true;
    		fclose(f);
    	}
    	return r;
    }
    
    bool file::exists(std::string filename){
    	return file(filename).exists();
    }

	bool file::open(std::string mode, std::string fileName){
		m_isOpen = false;
		if(fileName != "")
			this->fileName = fileName;
		if(this->fileName != ""){
			m_file = fopen(this->fileName.c_str(), mode.c_str());
			if(!m_file)
				return false;
			m_isOpen = true;
			return true;
		}
		return false;
	}

	std::string file::readAll(){
		if(!m_isOpen){
			return "";
		}
		unsigned long size = this->size();
		std::string content(size, '\0');
	    if(fread(&content[0], size, 1, m_file) > size){
	        std::cout<<"[ERROR] reading file:\""<<fileName<<"\"\n";
	    }
		return content;
	}

	std::string file::readAll(std::string fileName){
		file f(fileName);
		f.open("rb");
		std::string data = f.readAll();
		f.close();
		return data;
	}

	std::string file::read(unsigned long len){
	    if(m_isOpen){
	        std::string content(len, '\0');
	        if(fread(&content[0], len, 1, m_file) > len){
	            std::cout<<"[ERROR] reading file:\""<<fileName<<"\"\n";
	        }
	        return content;
	    }
	    return "";  
	}

	void file::write(std::string data){
	    if(m_isOpen){
	        fwrite(data.data(), data.size(), 1, m_file);
	    }
	}
	void file::write(std::string filename, std::string data){
		file f(filename);
		f.open("wb");
		f.write(data);
		f.close();
	}

	void file::close(){
		if(m_isOpen){
	        fclose(m_file);
	        m_isOpen = false;
	    }
	}

	unsigned long file::size()
	{
	    unsigned long size = 0;
	    if(m_isOpen){
	        fseek(m_file, 0, SEEK_END);
	        size = static_cast<unsigned long>(ftell(m_file));
	        fseek(m_file, 0, SEEK_SET);
	    }
	    return size;
	}

	bool file::isDir()
	{
	    struct stat pathStat;
	    stat(fileName.c_str(), &pathStat);
	    return S_ISDIR(pathStat.st_mode);
	}

	bool file::isOpen(){
	    return m_isOpen;
	}
}
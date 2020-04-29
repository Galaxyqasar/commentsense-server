#include <utils/image.hpp>

namespace utils{
	color::color(){
		r = g = b = a = 0.0f;
	}
	color::color(math::vec3 rgb){
		r = rgb.x;
		g = rgb.y;
		b = rgb.z;
	}
	color::color(math::vec4 rgba){
		r = rgba.x;
		g = rgba.y;
		b = rgba.z;
		a = rgba.w;
	}
	color::color(pixelRef pixel){
		r = float(pixel.data[0]) / 255.0f;
		g = float(pixel.data[1]) / 255.0f;
		b = float(pixel.data[2]) / 255.0f;
		a = float(pixel.data[3]) / 255.0f;
	}
	color::color(float r, float g, float b, float a){
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	color& color::operator=(pixelRef pixel){
		r = float(pixel.data[0]) / 255.0f;
		g = float(pixel.data[1]) / 255.0f;
		b = float(pixel.data[2]) / 255.0f;
		a = float(pixel.data[3]) / 255.0f;
		return *this;
	}
/**
 **
 **
 **
 **/
	pixelRef::pixelRef(unsigned char *data){
		this->data = data;
	}
	pixelRef& pixelRef::operator=(color c){
		data[0] = int(c.r*255);
		data[1] = int(c.g*255);
		data[2] = int(c.b*255);
		data[3] = int(c.a*255);
		return *this;
	}
/**
 **
 **
 **
 **/
	image::image(std::string file, bool flip){
		load(file, flip);
	}
	image::image(std::string file){
		load(file);
	}
	image::image(int width, int height, const unsigned char *data){
		w = width;
		h = height;
		initData(w, h);
		if(data){
			memcpy(d, data, 4*w*h);
		}
	}
	image::image(){
		d = new unsigned char[0];
	}
	image::~image(){
		stbi_image_free(d);
	}
	void image::initData(size_t width, size_t height){
		try{
			d = new unsigned char[4*width*height];
		}
		catch(std::bad_alloc &e){
			std::cerr<<"not enough memory\n";
			w = 0, h = 0;
			d = nullptr;
		}
	}
	image& image::operator=(const image &img){
		w = img.w;
		h = img.h;
		initData(w, h);
		memcpy(d, img.d, 4*w*h);
		return *this;
	}
	void image::load(std::string file, bool flip){
		stbi_set_flip_vertically_on_load(flip);
		d = stbi_load(file.c_str(), reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), nullptr, STBI_rgb_alpha);
	}
	void image::save(std::string file, int quality){
		std::string ending(file.begin() + file.rfind(".")+1, file.end());
		if(ending == "bmp")
			stbi_write_bmp(file.c_str(), w, h, 4, d);
		else if(ending == "jpg")
			stbi_write_jpg(file.c_str(), w, h, 4, d, quality);
		else if(ending == "png")
			stbi_write_png(file.c_str(), w, h, 4, d, w*4);
		else if(ending == "tga")
			stbi_write_tga(file.c_str(), w, h, 4, d);
		else
			std::cerr<<"format not supported\n";
	}

	void image::fill(color c){
		memset(d, 0, w*h*4);
	}
	size_t image::width(){
		return w;
	}
	size_t image::height(){
		return h;
	}
	unsigned char* image::data(){
		return d;
	}
	pixelRef image::operator()(int x, int y){
		return pixelRef(&d[(y*w+x)*4]);
	}
	pixelRef image::operator[](math::ivec2 pos){
		return this->operator()(pos.x, pos.y);
	}
	pixelRef image::at(int x, int y){
		return pixelRef(&d[(y*w+x)*4]);
	}
	pixelRef image::at(math::ivec2 pos){
		return this->operator()(pos.x, pos.y);
	}
}

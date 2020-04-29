#pragma once

#include <string.h>
#include <string>
#include <vector>
#include <iostream>

#include <math/vector.hpp>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

namespace utils{
	class color;
	class pixelRef;
	class image;

	class color{
	public:
		color();
		color(math::vec3 rgb);
		color(math::vec4 rgba);
		color(pixelRef pixel);
		color(float r, float g, float b, float a = 0.0f);
		color& operator=(pixelRef pixel);

		float r, g, b, a;
	};

	class pixelRef{ //a single pixel of an image
	public:
		pixelRef(unsigned char *data);
		pixelRef& operator=(color c);
	private:
		unsigned char *data;//4 bytes of data of image(rgba)
	friend color;
	};
	
	class image{
	public:
		enum Format{
			bmp,
			jpg,
			png
		};
		image(std::string file, bool flip);
		image(std::string file);
		image(int width, int height, const unsigned char *data = nullptr);
		image();
		~image();
		void initData(size_t width, size_t height);
		image& operator=(const image &img);
		void load(std::string file, bool flip = false);
		void save(std::string file, int quality = 0);


		void fill(color c = color());
		pixelRef operator()(int x, int y);
		pixelRef operator[](math::ivec2 pos);
		pixelRef at(int x, int y);
		pixelRef at(math::ivec2 pos);

		size_t width();
		size_t height();
		unsigned char* data();
	private:
		size_t w = 0, h = 0;
		unsigned char *d = nullptr;
	};
}

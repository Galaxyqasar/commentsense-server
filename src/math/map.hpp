#pragma once

#include <stdio.h>
#include "vector.hpp"

namespace math{
	template<typename T>
	class map{
	public:
		map(int width, int height, T val = T(0)){
			w = width, h = height;
			data = new T*[height];
			for(int y = 0; y < h; y++){
				data[y] = new T[w];
				for(int x = 0; x < w; x++){
					data[y][x] = val;
				}
			}
		}
		map(int width, int height, const T *values){
			w = width, h = height;
			data = new T*[height];
			for(int y = 0; y < h; y++){
				data[y] = new T[w];
				for(int x = 0; x < w; x++){
					data[y][x] = values[y*width+x];
				}
			}
		}
		map(const map &other){
			w = other.w;
			h = other.h;
			data = new T*[h];
			for(int y = 0; y < h; y++){
				data[y] = new T[w];
				for(int x = 0; x < w; x++){
					data[y][x] = other.data[y][x];
				}
			}
		}
		~map(){
			for(int y = 0; y < h; y++){
				delete [] data[y];
			}
			delete [] data;
			w = h = 0;
		}
		int width(){return w;}
		int height(){return h;}
		T& operator()(int x, int y){return (x < w && y < h) ? data[y][x] : fallback;}
		T& operator[](ivec2 pos){return (pos.x < w && pos.y < h) ? data[pos.y][pos.x] : fallback;}
		T at(ivec2 pos){return (pos.x < w && pos.y < h) ? data[pos.y][pos.x] : fallback;}
		T at(int x, int y){return (x < w && y < h) ? data[y][x] : fallback;}
		map& operator=(const map& other){
			this->~map();
			w = other.w;
			h = other.h;
			data = new T*[h];
			for(int y = 0; y < h; y++){
				data[y] = new T[w];
				for(int x = 0; x < w; x++){
					data[y][x] = other.data[y][x];
				}
			}
		}
		void fill(T c){
			for(int y = 0; y < h; y++){
				for(int x = 0; x < w; x++){
					data[y][x] = c;
				}
			}
		}
		void print(){
			for(int x = 0; x < w; x++){
				for(int y = 0; y < h; y++){
					std::cout<<at(x,y)<<"|";
				}
				std::cout<<"\n";
			}
		}
	private:
		int w, h;
		T **data;
		T fallback;
	};
}

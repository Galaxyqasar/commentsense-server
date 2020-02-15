#pragma once

#include <iostream>

#include "math.hpp"

namespace math{
	template<typename type, unsigned n>
	class tvecn{
	public:
		tvecn(type v = 0.0f){fill(v);}
		tvecn(type x, type y){fill(type(0.0)); this->x = x, this->y = y;}
		tvecn(type x, type y, type z){fill(type(0.0)); this->x = x, this->y = y, this->z = z;}
		tvecn(type x, type y, type z, type w){fill(type(0.0)); this->x = x, this->y = y, this->z = z, this->w = w;}
		tvecn(const tvecn<type, n+1> &other){for(unsigned i = 0; i < n; i++) data[i] = other.data[i];}
		tvecn(const tvecn<type, n-1> &other, type v = type(0.0)){for(unsigned i = 0; i < n-1; i++) data[i] = other.data[i]; data[n-1] = v;}
		//tvecn(const tvecn<type, n> &other){for(unsigned i = 0; i < n; i++) data[i] = other.data[i];}
		template<typename t2>
		tvecn(tvecn<t2, n> v){for(unsigned i = 0; i < n; i++) data[i] = type(v.data[i]);}
		void fill(type v){for(unsigned i = 0; i < n; i++) data[i] = v;}
		void print(){std::cout<<"(";for(unsigned i = 0; i < n-1; i++)std::cout<<data[i]<<"|";std::cout<<data[n-1]<<")\n";}
		tvecn<type, n> operator-(){tvecn<type, n> r;for(unsigned i = 0; i < n; i++) r.data[i] = -data[i]; return r;}
		tvecn<type, n> operator+(tvecn<type, n> a){tvecn<type, n> r;for(unsigned i = 0; i < n; i++) r.data[i] = data[i] + a.data[i]; return r;}
		tvecn<type, n> operator-(tvecn<type, n> a){tvecn<type, n> r;for(unsigned i = 0; i < n; i++) r.data[i] = data[i] - a.data[i]; return r;}
		tvecn<type, n> operator*(tvecn<type, n> a){tvecn<type, n> r;for(unsigned i = 0; i < n; i++) r.data[i] = data[i] * a.data[i]; return r;}
		tvecn<type, n> operator/(tvecn<type, n> a){tvecn<type, n> r;for(unsigned i = 0; i < n; i++) r.data[i] = data[i] / a.data[i]; return r;}
		bool operator==(tvecn<type, n> v){for(unsigned i = 0; i < n; i++) if(data[i] != v.data[i]) return false; return true;}
		tvecn<type, n>& operator+=(tvecn<type, n> a){for(unsigned i = 0; i < n; i++) data[i] += a.data[i]; return *this;}
		tvecn<type, n>& operator-=(tvecn<type, n> a){for(unsigned i = 0; i < n; i++) data[i] -= a.data[i]; return *this;}
		tvecn<type, n>& operator*=(tvecn<type, n> a){for(unsigned i = 0; i < n; i++) data[i] *= a.data[i]; return *this;}
		tvecn<type, n>& operator/=(tvecn<type, n> a){for(unsigned i = 0; i < n; i++) data[i] /= a.data[i]; return *this;}
		tvecn<type, n>& operator=(tvecn<type, n> v){for(unsigned i = 0; i < n; i++) data[i] = v.data[i]; return *this;}
		template<typename t2, unsigned size>
		tvecn<type, n>& operator=(tvecn<t2, n> v){for(unsigned i = 0; i < n; i++) data[i] = type(v.data[i]); return *this;}
		type& operator[](unsigned i){return data[i<n?i:0];};

		union{
			struct{
				type x, y, z, w;
			};
			struct{
				type r, g, b, a;
			};
			type data[n];
		};
	};

	template<typename t, unsigned n>
	std::ostream& operator<<(std::ostream &os, const tvecn<t, n> &vec){
		os<<'(';
		for(unsigned i = 0; i < n-1; i++)
			os<<vec.data[i]<<'|';
		os<<vec.data[n-1]<<')';
		return os;
	}

	template<typename t, unsigned n>
	t length(tvecn<t, n> v){
		t d = 0;
		for(int i = 0; i < n; i++)
			d += v[i] * v[i];
		return sqrt(d);
	}
	template<typename t, unsigned n>
	t dist(tvecn<t, n> a, tvecn<t, n> b){
		return length(a-b);
	}

	template<typename t, unsigned n>
	tvecn<t, n> normalize(tvecn<t, n> v){
		return v / length(v);
	}

	template<typename t, unsigned n>
	t dot(tvecn<t, n> a, tvecn<t, n> b){
		t d = 0;
		for(int i = 0; i < n; i++)
			d += a[i] * b[i];
		return d;
	}

	template<typename t, unsigned n>
	tvecn<t, n> floor(tvecn<t, n> v){
		tvecn<t, n> r = 0;
		for(int i = 0; i < n; i++)
			r[i] = floor(v[i]);
		return r;
	}
	template<typename t, unsigned n>
	tvecn<t, n> fract(tvecn<t, n> v){
		tvecn<t, n> r = 0;
		for(int i = 0; i < n; i++)
			r[i] = fract(v[i]);
		return r;
	}
	template<typename t, unsigned n>
	tvecn<t, n> smoothstep(tvecn<t, n> v){
		tvecn<t, n> r = 0;
		for(int i = 0; i < n; i++)
			r[i] = smoothstep(v[i]);
		return r;
	}
	template<typename t, unsigned n>
	tvecn<t, n> sigmoid(tvecn<t, n> v){
		tvecn<t, n> r = 0;
		for(int i = 0; i < n; i++)
			r[i] = sigmoid(v[i]);
		return r;
	}
	template<typename t, unsigned n>
	tvecn<t, n> clamp(tvecn<t, n> v, t min, t max){
		tvecn<t, n> r = 0;
		for(int i = 0; i < n; i++)
			r[i] = clamp(v[i], min, max);
		return r;
	}
	
	typedef tvecn<int, 2> ivec2;
	typedef tvecn<int, 3> ivec3;
	typedef tvecn<int, 4> ivec4;
	typedef tvecn<unsigned, 2> uvec2;
	typedef tvecn<unsigned, 3> uvec3;
	typedef tvecn<unsigned, 4> uvec4;
	
	typedef tvecn<float, 2> vec2;
	typedef tvecn<float, 3> vec3;
	typedef tvecn<float, 4> vec4;
	typedef tvecn<double, 2> dvec2;
	typedef tvecn<double, 3> dvec3;
	typedef tvecn<double, 4> dvec4;
	
	inline vec3 cross(vec3 v, vec3 a){ 
		return vec3(v.y*a.z - v.z*a.y, v.z*a.x - v.x*a.z, v.x*a.y - v.y*a.x); 
	}
	inline vec4 cross(vec4 v, vec4 a){ 
		return vec4(v.y*a.z - v.z*a.y, v.z*a.x - v.x*a.z, v.x*a.y - v.y*a.x, v.w); 
	}
}

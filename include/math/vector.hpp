#pragma once

#include <iostream>
#include <cmath>

#include "math.hpp"

namespace math {
	template<typename type>	class tvec2;
	template<typename type>	class tvec3;
	template<typename type>	class tvec4;


	template<typename type>
	class tvec2{
	public:
		tvec2(type v = 0.0f) : x(v), y(v) {}
		tvec2(type x, type y) : x(x), y(y) {}

		tvec2(const tvec2<type> &other);
		tvec2(const tvec3<type> &other);
		tvec2(const tvec4<type> &other);

		tvec2<type> operator-(){return tvec2<type>(-x, -y);}
		tvec2<type> operator+(tvec2<type> other){return tvec2<type>(x + other.x, y + other.y);}
		tvec2<type> operator-(tvec2<type> other){return tvec2<type>(x - other.x, y - other.y);}
		tvec2<type> operator*(tvec2<type> other){return tvec2<type>(x * other.x, y * other.y);}
		tvec2<type> operator/(tvec2<type> other){return tvec2<type>(x / other.x, y / other.y);}

		tvec2<type>& operator+=(tvec2<type> other){x += other.x, y += other.y; return *this;}
		tvec2<type>& operator-=(tvec2<type> other){x += other.x, y += other.y; return *this;}
		tvec2<type>& operator*=(tvec2<type> other){x += other.x, y += other.y; return *this;}
		tvec2<type>& operator/=(tvec2<type> other){x += other.x, y += other.y; return *this;}

		tvec2<type>& operator=(tvec2<type> other){x = other.x, y = other.y; return *this;}
		template <typename t2>
		tvec2<type>& operator=(tvec2<t2> other){x = other.x, y = other.y; return *this;}

		bool operator==(tvec2<type> other){return x == other.x && y == other.y;}

		union {
			struct {
				type r, g;
			};
			struct {
				type x, y;
			};
		};
	};

	template<typename type>
	class tvec3{
	public:
		tvec3(type v = 0.0f) : x(v), y(v), z(v) {}
		tvec3(type x, type y, type z) : x(x), y(y), z(z) {}

		tvec3(const tvec2<type> &other, type z = 0);
		tvec3(const tvec3<type> &other);
		tvec3(const tvec4<type> &other);

		tvec3<type> operator-(){return tvec3<type>(-x, -y, -z);}
		tvec3<type> operator+(tvec3<type> other){return tvec3<type>(x + other.x, y + other.y, z + other.z);}
		tvec3<type> operator-(tvec3<type> other){return tvec3<type>(x - other.x, y - other.y, z - other.z);}
		tvec3<type> operator*(tvec3<type> other){return tvec3<type>(x * other.x, y * other.y, z * other.z);}
		tvec3<type> operator/(tvec3<type> other){return tvec3<type>(x / other.x, y / other.y, z / other.z);}

		tvec3<type>& operator+=(tvec3<type> other){x += other.x, y += other.y, y += other.y; return *this;}
		tvec3<type>& operator-=(tvec3<type> other){x -= other.x, y -= other.y, z -= other.z; return *this;}
		tvec3<type>& operator*=(tvec3<type> other){x *= other.x, y *= other.y, z *= other.z; return *this;}
		tvec3<type>& operator/=(tvec3<type> other){x /= other.x, y /= other.y, z /= other.z; return *this;}

		tvec3<type>& operator=(tvec3<type> other){x = other.x, y = other.y, z = other.z; return *this;}
		template <typename t2>
		tvec3<type>& operator=(tvec3<t2> other){x = other.x, y = other.y, z = other.z; return *this;}

		bool operator==(tvec3<type> other){return x == other.x && y == other.y && z == other.z;}

		union {
			struct {
				type r, g, b;
			};
			struct {
				type x, y, z;
			};
			tvec2<type> xy;
		};
	};

	template<typename type>
	class tvec4{
	public:
		tvec4(type v = 0) : x(v), y(v) , z(v), w(v) {}
		tvec4(type x, type y, type z = 0, type w = 0) : x(x), y(y), z(z), w(w) {}

		tvec4(const tvec2<type> &other, tvec2<type> rest = 0);
		tvec4(const tvec2<type> &other, type z = 0, type w = 0);
		tvec4(const tvec3<type> &other, type w = 0);
		tvec4(const tvec4<type> &other);

		tvec4<type> operator-(){return tvec4<type>(-x, -y, -z, -w);}
		tvec4<type> operator+(tvec4<type> other){return tvec4<type>(x + other.x, y + other.y, z + other.z, w + other.w);}
		tvec4<type> operator-(tvec4<type> other){return tvec4<type>(x - other.x, y - other.y, z - other.z, w - other.w);}
		tvec4<type> operator*(tvec4<type> other){return tvec4<type>(x * other.x, y * other.y, z * other.z, w * other.w);}
		tvec4<type> operator/(tvec4<type> other){return tvec4<type>(x / other.x, y / other.y, z / other.z, w / other.w);}

		tvec4<type>& operator+=(tvec4<type> other){x += other.x, y += other.y, z += other.z, w += other.w; return *this;}
		tvec4<type>& operator-=(tvec4<type> other){x -= other.x, y -= other.y, z -= other.z, w -= other.w; return *this;}
		tvec4<type>& operator*=(tvec4<type> other){x *= other.x, y *= other.y, z *= other.z, w *= other.w; return *this;}
		tvec4<type>& operator/=(tvec4<type> other){x /= other.x, y /= other.y, z /= other.z, w /= other.w; return *this;}

		tvec4<type>& operator=(tvec4<type> other){x = other.x, y = other.y, z = other.z, w = other.w; return *this;}
		template <typename t2>
		tvec4<type>& operator=(tvec4<t2> other){x = other.x, y = other.y, z = other.z, w = other.w; return *this;}

		bool operator==(tvec4<type> other){return x == other.x && y == other.y && z == other.z && w == other.w;}

		union {
			struct {
				type r, g, b, a;
			};
			struct {
				type x, y, z, w;
			};
			struct {
				tvec2<type> xy, zw;
			};
			struct {
				tvec2<type> rg, ba;
			};
			tvec3<type> xyz;
			tvec3<type> rgb;
		};
	};

	template<typename type>	tvec2<type>::tvec2(const tvec2<type> &other) : x(other.x), y(other.y) {}
	template<typename type>	tvec2<type>::tvec2(const tvec3<type> &other) : x(other.x), y(other.y) {}
	template<typename type>	tvec2<type>::tvec2(const tvec4<type> &other) : x(other.x), y(other.y) {}

	template<typename type>	tvec3<type>::tvec3(const tvec2<type> &other, type z) : x(other.x), y(other.y), z(z) {}
	template<typename type>	tvec3<type>::tvec3(const tvec3<type> &other) : x(other.x), y(other.y), z(other.z) {}
	template<typename type>	tvec3<type>::tvec3(const tvec4<type> &other) : x(other.x), y(other.y), z(other.z) {}

	template<typename type>	tvec4<type>::tvec4(const tvec2<type> &other, tvec2<type> rest) : x(other.x), y(other.y), z(rest.x), w(rest.y) {}
	template<typename type>	tvec4<type>::tvec4(const tvec2<type> &other, type z, type w) : x(other.x), y(other.y), z(z), w(w) {}
	template<typename type>	tvec4<type>::tvec4(const tvec3<type> &other, type w) : x(other.x), y(other.y), z(other.z), w(w) {}
	template<typename type>	tvec4<type>::tvec4(const tvec4<type> &other) : x(other.x), y(other.y), z(other.z), w(other.w) {}

	template<typename type>
	std::ostream& operator<<(std::ostream &os, const tvec2<type> &vec){
		return os<<"("<<vec.x<<" | "<<vec.y<<")";
	}
	template<typename type>
	std::ostream& operator<<(std::ostream &os, const tvec3<type> &vec){
		return os<<"("<<vec.x<<" | "<<vec.y<<" | "<<vec.z<<")";
	}
	template<typename type>
	std::ostream& operator<<(std::ostream &os, const tvec4<type> &vec){
		return os<<"("<<vec.x<<" | "<<vec.y<<" | "<<vec.z<<" | "<<vec.w<<")";
	}

	template<typename type>
	type length(tvec2<type> v){
		return sqrt(v.x*v.x + v.y*v.y);
	}
	template<typename type>
	type length(tvec3<type> v){
		return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	}
	template<typename type>
	type length(tvec4<type> v){
		return sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
	}

	template<typename type>
	type dist(tvec2<type> a, tvec2<type> b){
		return length(a-b);
	}
	template<typename type>
	type dist(tvec3<type> a, tvec3<type> b){
		return length(a-b);
	}
	template<typename type>
	type dist(tvec4<type> a, tvec4<type> b){
		return length(a-b);
	}

	template<typename type>
	tvec2<type> normalize(tvec2<type> v){
		return v / length(v);
	}
	template<typename type>
	tvec3<type> normalize(tvec3<type> v){
		return v / length(v);
	}
	template<typename type>
	tvec4<type> normalize(tvec4<type> v){
		return v / length(v);
	}

	template<typename type>
	type dot(tvec2<type> a, tvec2<type> b){
		return a.x*b.x + a.y*b.y;
	}
	template<typename type>
	type dot(tvec3<type> a, tvec3<type> b){
		return a.x*b.x + a.y*b.y + a.z*b.z;
	}
	template<typename type>
	type dot(tvec4<type> a, tvec4<type> b){
		return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
	}

	template<typename type>
	tvec3<type> cross(tvec3<type> v, tvec3<type> a){ 
		return tvec3<type>(v.y*a.z - v.z*a.y, v.z*a.x - v.x*a.z, v.x*a.y - v.y*a.x); 
	}

	template<typename type>
	tvec2<type> floor(tvec2<type> v) {
		return tvec2<type>(floor(v.x), floor(v.y));
	}
	template<typename type>
	tvec3<type> floor(tvec3<type> v) {
		return tvec3<type>(floor(v.x), floor(v.y), floor(v.z));
	}
	template<typename type>
	tvec4<type> floor(tvec4<type> v) {
		return tvec4<type>(floor(v.x), floor(v.y), floor(v.z), floor(v.w));
	}

	template<typename type>
	tvec2<type> fract(tvec2<type> v) {
		return tvec2<type>(fract(v.x), fract(v.y));
	}
	template<typename type>
	tvec3<type> fract(tvec3<type> v) {
		return tvec3<type>(fract(v.x), fract(v.y), fract(v.z));
	}
	template<typename type>
	tvec4<type> fract(tvec4<type> v) {
		return tvec4<type>(fract(v.x), fract(v.y), fract(v.z), fract(v.w));
	}

	template<typename type>
	tvec2<type> mix(tvec2<type> a, tvec2<type> b, float amnt) {
		return tvec2<type>(mix(a.x, b.x, amnt), mix(a.y, b.y, amnt));
	}
	template<typename type>
	tvec3<type> mix(tvec3<type> a, tvec3<type> b, float amnt) {
		return tvec3<type>(mix(a.x, b.x, amnt), mix(a.y, b.y, amnt), mix(a.z, b.z, amnt));
	}
	template<typename type>
	tvec4<type> mix(tvec4<type> a, tvec4<type> b, float amnt) {
		return tvec4<type>(mix(a.x, b.x, amnt), mix(a.y, b.y, amnt), mix(a.z, b.z, amnt), mix(a.w, b.w, amnt));
	}

	template<typename type>
	tvec2<type> mix(tvec2<type> a, tvec2<type> b, tvec2<float> amnt) {
		return tvec2<type>(mix(a.x, b.x, amnt.x), mix(a.y, b.y, amnt.y));
	}
	template<typename type>
	tvec3<type> mix(tvec3<type> a, tvec3<type> b, tvec3<float> amnt) {
		return tvec3<type>(mix(a.x, b.x, amnt.x), mix(a.y, b.y, amnt.y), mix(a.z, b.z, amnt.z));
	}
	template<typename type>
	tvec4<type> mix(tvec4<type> a, tvec4<type> b, tvec4<float> amnt) {
		return tvec4<type>(mix(a.x, b.x, amnt.x), mix(a.y, b.y, amnt.y), mix(a.z, b.z, amnt.z), mix(a.w, b.w, amnt.w));
	}

	template<typename type>
	tvec2<type> smoothstep(tvec2<type> v) {
		return tvec2<type>(smoothstep(v.x), smoothstep(v.y));
	}
	template<typename type>
	tvec3<type> smoothstep(tvec3<type> v) {
		return tvec3<type>(smoothstep(v.x), smoothstep(v.y), smoothstep(v.z));
	}
	template<typename type>
	tvec4<type> smoothstep(tvec4<type> v) {
		return tvec4<type>(smoothstep(v.x), smoothstep(v.y), smoothstep(v.z), smoothstep(v.w));
	}
	
	typedef tvec2<int> ivec2;
	typedef tvec3<int> ivec3;
	typedef tvec4<int> ivec4;
	typedef tvec2<unsigned> uvec2;
	typedef tvec3<unsigned> uvec3;
	typedef tvec4<unsigned> uvec4;
	
	typedef tvec2<float> vec2;
	typedef tvec3<float> vec3;
	typedef tvec4<float> vec4;
	typedef tvec2<double> dvec2;
	typedef tvec3<double> dvec3;
	typedef tvec4<double> dvec4;
}

#pragma once

#include <math.h>
#include <stdlib.h>

namespace math{
	const float pi = 3.14159;
	const float e = 2.71828;

	inline float d2r(float x){
		return pi/180.0f*x;
	}
	inline float r2d(float x){
		return x/pi*180.0f;
	}

	inline bool equal(float a, float b){
		return abs(a - b) < 0.00001;
	}

	inline float floor(float x){
		return float(int(x));
	}

	inline float fract(float x){
		return x + floor(x)*(x > 0 ? -1 : 1);
	}

	inline float mix(float a, float b, float c){
		return a*(1.0f-c) + b*(c);
	}

	template<typename type>
	type max(type a, type b){
		return  a > b ? a : b;
	}

	template<typename type>
	type min(type a, type b){
		return  a < b ? a : b;
	}

	template<typename type>
	type clamp(type x, type l, type u) {
		return max(min(x, u), l);
	}

	inline float smoothstep(float e0, float e1, float x){
		x = clamp((x - e0) / (e1 - e0), 0.0f, 1.0f); 
		return x * x * (3 - 2 * x);
	}

	inline float sigmoid(float x){
		return 1.0f / (1 + exp(-x));
	}

	unsigned int gcd(unsigned int u, unsigned int v);
}

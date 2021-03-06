#include <math/math.hpp>

namespace math{
	unsigned int gcd(unsigned int u, unsigned int v){
	    int shift;
	    if (u == 0) return v;
	    if (v == 0) return u;
	    shift = __builtin_ctz(u | v);
	    u >>= __builtin_ctz(u);
	    do {
	        v >>= __builtin_ctz(v);
	        if (u > v) {
	            unsigned int t = v;
	            v = u;
	            u = t;
	        }  
	        v = v - u;
	    } while (v != 0);
	    return u << shift;
	}
}

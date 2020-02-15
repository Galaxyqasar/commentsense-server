#include "matrix.hpp"

#pragma push_macro("minor")
#undef minor

namespace math{
	mat4  projection(float fov, float aspect, float near, float far, bool leftHanded){
		mat4  result;
		if (fov <= 0 || aspect == 0)
			return result;
		float frustumDepth = far - near;
		float oneOverDepth = 1 / frustumDepth;

		result(1,1) = 1 / tan(0.5f * fov);
		result(0,0) = (leftHanded ? 1 : -1 ) * result(1,1) / aspect;
		result(2,2) = far * oneOverDepth;
		result(3,2) = (-far * near) * oneOverDepth;
		result(2,3) = 1;
		result(3,3) = 0;
		return result;
	}
}

#pragma pop_macro("minor")
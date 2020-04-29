#pragma once

#include "math.hpp"
#include "vector.hpp"

#include <sstream>

#pragma push_macro("minor")
#undef minor

namespace math{
	template<typename type, unsigned n>
	class tmatn{
	public:
		tmatn(){
			fill(type(0));
			for(unsigned i = 0; i < n; i++)
				data[i][i] = type(1);
		}
		tmatn(tvecn<tvecn<type, n>, n> v){
			for(unsigned x = 0; x < n; x++)
				for(unsigned y = 0; y < n; y++)
					data[y][x] = v[y][x];
		}
		tmatn(type v){
			fill(v);
		}
		void fill(type v){
			for(unsigned x = 0; x < n; x++)
				for(unsigned y = 0; y < n; y++)
					data[y][x] = v;
		}
		type& operator()(unsigned row, unsigned col){
			if(col < n && row < n)
				return data[row][col];
			return data[0][0];
		}
		std::string print(){
			std::stringstream result;
			for(unsigned y = 0; y < n; y++){
				for(unsigned x = 0; x < n; x++){
					result<<'|'<<get(y, x);
				}
				result<<"|\n";
			}
			return result.str();
		}

		tmatn<type, n> operator*(tmatn<type, n> mat){
			tmatn<type, n> r;
			for(unsigned row = 0; row < n; row++){
				for(unsigned col = 0; col < n; col++){
					r(row, col) = 0;
					for(unsigned i = 0; i < n; i++){
						r(row, col) += get(row, i) * mat.get(i, col);
					}
				}
			}
			return r;
		}
		tvecn<type, n> operator*(tvecn<type, n> vec){
			tvecn<type, n> r;
			for(unsigned row = 0; row < n; row++){
				r[row] = 0;
				for(unsigned i = 0; i < n; i++){
					r[row] += get(i, row) * vec[i];
				}
			}
			return r;
		}
		inline type& get(unsigned x, unsigned y){
			return this->operator()(x, y);
		}

		tmatn<type, n> transpose(){
			tmatn<type, n> m;
			for(int y = 0; y < 4; y++){
				for(int x = 0; x < 4; x++)
					m(x,y) = get(y,x);
			}
			return m;
		}

		type data[n][n];
	};

	template<typename type>
	class tmat2 : public tmatn<type, 2>{
	public:
		tmat2(tmatn<type, 2> other = tmatn<type, 2>()) : tmatn<type, 2>(other){}
		tmat2<type> rotate(type angle){
			tmatn<type, 2> z;
			z(0,0) = cos(angle);
			z(0,1) =-sin(angle);
			z(1,0) = sin(angle);
			z(1,1) = cos(angle);
			return this->operator*(z);
		}
		type determinant(){
			return this->get(0,0) * this->get(1,1) - this->get(0,1) * this->get(1,0);
		}
	};

	template<typename type>
	class tmat3 : public tmatn<type, 3>{
	public:
		tmat3(tmatn<type, 3> other = tmatn<type, 3>()) : tmatn<type, 3>(other){}
		template<typename... Args>
		tmat3<type> rotate(Args... args){
			tvecn<type, 3> a(args...);
			tmatn<type, 3> x, y, z;
			x(1,1) = cos(a.x);
			x(1,2) =-sin(a.x);
			x(2,1) = sin(a.x);
			x(2,2) = cos(a.x);

			y(0,0) = cos(a.y);
			y(2,0) = sin(a.y);
			y(0,2) =-sin(a.y);
			y(2,2) = cos(a.y);

			z(0,0) = cos(a.z);
			z(0,1) =-sin(a.z);
			z(1,0) = sin(a.z);
			z(1,1) = cos(a.z);
			return this->operator*(x*y*z);
		}
		type determinant(){
			type det = type(0);
			for(int x = 0; x < 3; x++){
				det += this->get(0,x)*cofactor(0,x);
			}
			return det;
		}
		tmat2<type> submatrix(int row, int col){
			tmat2<type> m;
			for(int y = 0, ty = 0; y < 3; y++){
				if(y == row){
					continue;
				}
				for(int x = 0, tx = 0; x < 3; x++){
					if(x == col){
						continue;
					}
					m(ty,tx++) = this->get(y,x);
				}
				ty++;
			}
			return m;
		}
		type minor(int row, int col){
			return submatrix(row,col).determinant();
		}
		type cofactor(int row, int col){
			return minor(row,col) * ((row+col)%2 == 1 ? 1 : -1);
		}
	};

	template<typename type>
	class tmat4 : public tmatn<type, 4>{
	public:
		template<typename... Args>
		tmat4(Args... args) : tmatn<type, 4>(args...){}

		template<typename... Args>
		tmat4<type> translate(Args... args){
			tvecn<type, 3> p(args...);
			tmat4<type> r;
			for(unsigned i = 0; i < 3; i++)
				r(i,3) = p[i];
			return this->operator*(r);
		}

		template<typename... Args>
		tmat4<type> scale(Args... args){
			tvecn<type, 3> s(args...);
			tmat4<type> r;
			for(unsigned i = 0; i < 3; i++)
				r(i,i) = s[i];
			return this->operator*(r);
		}

		template<typename... Args>
		tmat4<type> rotate(Args... args){
			tvecn<type, 3> a(args...);
			tmatn<type, 4> x, y, z;
			x(1,1) = cos(a.x);
			x(1,2) =-sin(a.x);
			x(2,1) = sin(a.x);
			x(2,2) = cos(a.x);

			y(0,0) = cos(a.y);
			y(2,0) = sin(a.y);
			y(0,2) =-sin(a.y);
			y(2,2) = cos(a.y);

			z(0,0) = cos(a.z);
			z(0,1) =-sin(a.z);
			z(1,0) = sin(a.z);
			z(1,1) = cos(a.z);
			return this->operator*(z*y*x);
		}
		type determinant(){
			type det = 0.0f;
			for(int x = 0; x < 4; x++){
				det += this->get(0,x)*cofactor(0,x);
			}
			return det;
		}
		tmat3<type> submatrix(int row, int col){
			tmat3<type> m;
			for(int y = 0, ty = 0; y < 4; y++){
				if(y == row){
					continue;
				}
				for(int x = 0, tx = 0; x < 4; x++){
					if(x == col){
						continue;
					}
					m(ty,tx++) = this->get(y,x);
				}
				ty++;
			}
			return m;
		}
		type minor(int row, int col){
			return submatrix(row,col).determinant();
		}
		type cofactor(int row, int col){
			return minor(row,col) * ((row+col)%2 == 1 ? 1 : -1);
		}
		bool isInvertible(){
			return determinant() != type(0);
		}
		tmat4<type> inverse(){
			tmat4<type> m(0);
			type det = determinant();
			if(det != 0.0){
				for(int y = 0; y < 4; y++){
					for(int x = 0; x < 4; x++){
						m(y,x) = cofactor(x,y)/det;
					}
				}
			}
			return m;
		}
		template<typename T>
		tmat4<type> operator/(T arg){
			return inverse() * arg;
		}
	};

	template<typename t, unsigned n>
	std::ostream& operator<<(std::ostream &os, tmatn<t, n> &mat){
		os<<"hi"<<mat.print();
		return os;
	}

	typedef tmat2<float> mat2;
	typedef tmat3<float> mat3;
	typedef tmat4<float> mat4;

	inline mat4 projection(float fov, float aspect, float near, float far, bool leftHanded = true){
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

	template<typename... Args>
	inline mat4 scaling(Args... args){ return mat4().scale(args...); }
	template<typename... Args>
	inline mat4 translation(Args... args){ return mat4().translate(args...); }
	template<typename... Args>
	inline mat4 rotation(Args... args){ return mat4().rotate(args...); }
}

#pragma pop_macro("minor")

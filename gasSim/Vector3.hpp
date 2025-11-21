#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <cmath>

namespace GS {

	template <typename FP> 
	struct Vector3 { // a three-dimensional vector
		static_assert(std::is_floating_point_v<FP>);
		
		FP x {0.}, y{0.}, z{0.};

		Vector3() = default;
		Vector3(FP x, FP y, FP z) : x{x}, y{y}, z{z} {}

		template <typename FP1>
		explicit Vector3(Vector3<FP1> const& v) :
			x{static_cast<FP>(v.x)},
			y{static_cast<FP>(v.y)},
			z{static_cast<FP>(v.z)} 
		{}

		Vector3 operator-() const { return {-x, -y, -z}; }
		Vector3& operator+=(Vector3 const& v) {
			x += v.x; y += v.y; z += v.z;
			return *this;
		};
		Vector3& operator-=(Vector3 const& v) {
			x -= v.x; y -= v.y; z -= v.z;
			return *this;
		}; 

		Vector3 operator*(FP c) const { return {x*c, y*c, z*c}; }
		Vector3 operator/(FP c) const { return {x/c, y/c, z/c}; }

		bool operator==(Vector3 const& v) {
			return x == v.x && y = v.y && z = v.z;
		};
		bool operator!=(Vector3 const& v) {
			return x != v.x || y != v.y || z != v.z;
		};

		double norm() const {
			return sqrt(x*x + y*y + z*z);
		};
		void normalize() {
			*this = *this/norm();
		};
	};

	template <typename FP>
	Vector3<FP> operator+(Vector3<FP> v1, Vector3<FP> const& v2) {
		v1 += v2;
		return v1;
	}

	template <typename FP>
	Vector3<FP> operator-(Vector3<FP> v1, Vector3<FP> const& v2) {
		v1 -= v2;
		return v1;
	}

	template <typename FP>
	FP operator*(Vector3<FP> const& v1, Vector3<FP> const& v2) {
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
	}

	template <typename FP>
	Vector3<FP> cross(Vector3<FP> const& v1, Vector3<FP> const& v2) {
		return {
			v1.y * v2.z - v1.z * v2.y,
			v1.z * v2.x - v1.x * v2.z,
			v1.x * v2.y - v1.y * v2.x
		};
	}

	using Vector3f = Vector3<float>;
	using Vector3d = Vector3<double>;

}

#endif

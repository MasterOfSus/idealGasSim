#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include <cmath>

namespace GS {

template <typename FP>
struct GSVector {  // a three-dimensional vector
  static_assert(std::is_floating_point_v<FP>);

  FP x{0.}, y{0.}, z{0.};

  GSVector() = default;
  GSVector(FP x, FP y, FP z) : x{x}, y{y}, z{z} {}

  template <typename FP1>
  explicit GSVector(GSVector<FP1> const& v)
      : x{static_cast<FP>(v.x)},
        y{static_cast<FP>(v.y)},
        z{static_cast<FP>(v.z)} {}

  GSVector operator-() const { return {-x, -y, -z}; }
  GSVector& operator+=(GSVector const& v) {
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
  }
  GSVector& operator-=(GSVector const& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
  }

  GSVector operator*(FP c) const { return {x * c, y * c, z * c}; }
  GSVector operator/(FP c) const { return {x / c, y / c, z / c}; }


  double norm() const { return sqrt(x * x + y * y + z * z); }
  void normalize() { *this = *this / norm(); }
};

template<typename FP>
bool operator==(GSVector<FP> const& v1, GSVector<FP> const& v2) {
	return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

template <typename FP>
bool operator!=(GSVector<FP> const& v1, GSVector<FP> const& v2) { return v1.x != v2.x || v1.y != v2.y || v1.z != v2.z; }

template <typename FP>
GSVector<FP> operator+(GSVector<FP> v1, GSVector<FP> const& v2) {
  v1 += v2;
  return v1;
}

template <typename FP>
GSVector<FP> operator-(GSVector<FP> v1, GSVector<FP> const& v2) {
  v1 -= v2;
  return v1;
}

template <typename FP>
FP operator*(GSVector<FP> const& v1, GSVector<FP> const& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename FP>
GSVector<FP> operator*(FP x, GSVector<FP> v) {
	return v*x;
}

template <typename FP>
GSVector<FP> cross(GSVector<FP> const& v1, GSVector<FP> const& v2) {
  return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
          v1.x * v2.y - v1.y * v2.x};
}

using GSVectorF = GSVector<float>;
using GSVectorD = GSVector<double>;

}  // namespace GS

#endif

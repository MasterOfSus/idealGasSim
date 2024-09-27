#include "physics_engine.hpp"

#include <cmath>
#include <random>

namespace gasSim {
namespace physics {

// Definition of vector functions
vector::vector() : x(0), y(0), z(0) {}
vector::vector(double x, double y, double z) : x(x), y(y), z(z) {}
vector::vector(double maxValue) {
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, maxValue);
  x = dist(eng);
  y = dist(eng);
  z = dist(eng);
}
vector vector::operator+(const vector& v) {
  return {x + v.x, y + v.y, z + v.z};
}
vector vector::operator-(const vector& v) {
  return {x - v.x, y - v.y, z - v.z};
}
vector vector::operator*(double scalar) {
  return {x * scalar, y * scalar, z * scalar};
}
vector vector::operator/(double scalar) {
  return {x / scalar, y / scalar, z / scalar};
}
double vector::operator*(const vector& v) {
  return x * v.x + y * v.y + z * v.z;
}
bool vector::operator==(const vector& v) const {
  return (x == v.x && y == v.y && z == v.z);
}
bool vector::operator!=(const vector& v) const { return !(*this == v); }
double vector::norm() { return std::sqrt(x * x + y * y + z * z); }

}  // namespace physics
}  // namespace gasSim

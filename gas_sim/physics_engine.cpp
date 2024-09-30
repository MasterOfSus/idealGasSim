#include "physics_engine.hpp"

#include <cmath>
#include <random>

namespace gasSim {
namespace physics {

// Definition of vector functions

vector vector::operator+(const vector& v) {
  return {x + v.x, y + v.y, z + v.z};
}

vector vector::operator-(const vector& v) {
  return {x - v.x, y - v.y, z - v.z};
}

vector vector::operator*(const double scalar) {
  return {x * scalar, y * scalar, z * scalar};
}

vector vector::operator/(const double scalar) {
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

vector randomVector(const double maxNorm) {
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, pow(maxNorm / 3, 1. / 2.));
  return {dist(eng), dist(eng), dist(eng)};
}

}  // namespace physics
}  // namespace gasSim

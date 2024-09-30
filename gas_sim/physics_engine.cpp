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
// End of vector functions

// Definition of particle functions
bool particle::operator==(const particle& p) const {
  return (position == p.position && speed == p.speed);
}
// End of particle functions

// Definition of square_box functions
bool square_box::operator==(const square_box& b) const {
  return (side == b.side);
}
// End of square_box functions

// Definition of gas functions
gas::gas(std::vector<particle> particles, square_box box)
    : particles_{particles}, box_{box} {}
gas::gas(const gas& gas)
    : particles_(gas.particles_.begin(), gas.particles_.end()),
      box_{gas.box_} {}

const std::vector<particle>& gas::get_particles() const { return particles_; }
const square_box& gas::get_box() const { return box_; }

}  // namespace physics
}  // namespace gasSim

#include "physics_engine.hpp"
#include <cmath>

namespace thermo {
namespace physics {

vector vector::operator+(const vector& v)
{
  return {x + v.x, y + v.y, z + v.z};
}
vector vector::operator-(const vector& v)
{
  return {x - v.x, y - v.y, z - v.z};
}

vector vector::operator*(double scalar)
{
  return {x * scalar, y * scalar, z * scalar};
}
vector vector::operator/(double scalar)
{
  return {x / scalar, y / scalar, z / scalar};
}

double vector::operator*(const vector& v)
{
  return x * v.x + y * v.y + z * v.z;
}

bool vector::operator==(const vector& v) const
{
  return (x == v.x && y == v.y && z == v.z);
}
bool vector::operator!=(const vector& v) const
{
  return !(*this == v);
}

double vector::norm()
{
  return std::sqrt(x * x + y * y + z * z);
}

} // namespace physics
} // namespace thermo

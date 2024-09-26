#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

namespace gasSim {
namespace physics {
struct vector
{
  double x, y, z;

  vector operator+(const vector& v);
  vector operator-(const vector& v);

  vector operator*(double scalar);
  vector operator/(double scalar);

  double operator*(const vector& v);

  bool operator==(const vector& v) const;
  bool operator!=(const vector& v) const;

  double norm();
};

struct particle {
  static double mass;
  static double radius;

  vector position;
  vector speed;
};

} // namespace physics
} // namespace gasSim

#endif

#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <vector>

namespace gasSim {
namespace physics {
struct vector {
  double x, y, z;

  vector operator+(const vector& v);
  vector operator-(const vector& v);

  vector operator*(const double scalar);
  vector operator/(const double scalar);

  double operator*(const vector& v);

  bool operator==(const vector& v) const;
  bool operator!=(const vector& v) const;

  double norm();
};

vector randomVector(double maxNorm);

struct particle {
  static double mass;
  static double radius;

  vector position;
  vector speed;

  bool operator==(const particle& p) const;
};

struct square_box {
  double side;

  bool operator==(const square_box& b) const;
};

class gas {
 public:
  gas(std::vector<particle> particles, square_box box);
  gas(const gas& gas);

  const std::vector<particle>& get_particles() const;
  const square_box& get_box() const;

  void update_gas_state();  // called in each iteration of the game loop

 private:
  std::vector<particle> particles_;
  square_box box_;  // side of the cubical container
  void update_positions(double time);
};

}  // namespace physics
}  // namespace gasSim

#endif

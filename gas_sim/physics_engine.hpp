#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <vector>

namespace gasSim {
namespace physics {
struct vector {
  double x, y, z;

  vector();
  vector(double x, double y, double z);
  vector(double maxValue);

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

struct square_box {
  double side;
};

class gas {
 public:
  void update_gas_state();  // called in each iteration of the game loop
  gas(const gas&);

 private:
  std::vector<particle> particles_;
  square_box box_;  // side of the cubical container
  void update_positions(double time);
};

}  // namespace physics
}  // namespace gasSim

#endif

#include <cmath>
#include <ctime>
#include <vector>

namespace thermo {

struct PhysVector {  // representation of a simple three-dimensional vector
  double x_, y_, z_;

  void operator+=(const PhysVector& v);
};

struct Particle {
  static double mass_;
  static double radius_;

  PhysVector position_;
  PhysVector speed_;
};

struct Wall {
  char wall_type_;
};

struct Collision {
  double time_;
  std::vector<std::vector<Particle>::iterator> particles_;
  std::vector<Wall> walls_;
};

class Gas {
  std::vector<Particle> particles_;
  double side_;  // side of the cubical container

 public:
  void update_positions(double time);
  void update_gas_state(
      Collision collision);  // called in each iteration of the game loop
  Gas(const Gas&);
  Gas(int, double, double);

  Collision find_iteration();

  // double get_pressure();     // returns the value of the pressure of the gas
  // double get_volume();       // returns the volume of the gas
  // double get_temperature();  // returns the temperature of the gas
};

}  // namespace thermo

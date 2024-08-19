#include <cmath>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

namespace thermo {
struct PhysVector {
  double x, y, z;
  double get_module() { return std::sqrt(x * x + y * y + z * z); }
  PhysVector(double Ix = 0.0, double Iy = 0.0, double Iz = 0.0)
      : x{Ix}, y{Iy}, z{Iz} {}
};

class Particle {
  static double mass_;
  short int last_collision_;
  PhysVector position_;
  PhysVector speed_;
  PhysVector momentum_;

 public:
  Particle(PhysVector Ipos, PhysVector Ispeed)
      : position_{Ipos}, speed_{Ispeed} {}
};
class Gas {
  std::vector<Particle> particles_{};
  void update_gas_state();  // called in each iteration of the game loop
 public:
  Gas(int n) {
    std::default_random_engine eng(std::time(nullptr));
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i{0}; i != n; ++i) {
      particles_.push_back({{dist(eng), dist(eng), dist(eng)},
                            {dist(eng), dist(eng), dist(eng)}});
    }
  }
  double get_pressure();     // returns the value of the pressure of the gas
  int get_volume();          // returns the volume of the gas
  double get_temperature();  // returns the temperature of the gas
};
}  // namespace thermo
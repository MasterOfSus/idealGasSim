#include <cmath>
#include <ctime>
#include <vector>

namespace thermo {
struct PhysVector {
  double x, y, z;
  double get_module();
  PhysVector(double Ix = 0.0, double Iy = 0.0, double Iz = 0.0);
  PhysVector operator*(double factor) const;
};

class Particle {
  //static double mass_;
  
  //int last_collision_;
  PhysVector position_;
  PhysVector speed_;
  //PhysVector momentum_;

 public:
  double radius=5.;
  PhysVector get_speed();
  PhysVector get_position();
  //PhysVector get_momentum();

  Particle(PhysVector Ipos, PhysVector Ispeed);
};
class Gas {
  std::vector<Particle> particles_{};
  void update_gas_state();  // called in each iteration of the game loop
  double time_impact(std::vector<Particle>::iterator a,
                     std::vector<Particle>::iterator b);

 public:
  Gas(int n);
  double find_iteration();
  //double get_pressure();     // returns the value of the pressure of the gas
  //double get_volume();       // returns the volume of the gas
  //double get_temperature();  // returns the temperature of the gas
};
}  // namespace thermo
#include <cmath>
#include <ctime>
#include <vector>

namespace thermo {
struct PhysVector {
  double x, y, z;
  double get_module();
  PhysVector(double Ix = 0.0, double Iy = 0.0, double Iz = 0.0);
  PhysVector operator*(double factor) const;
	double operator*(const PhysVector& vector) const;
};

struct Particle {
  // static double mass_;

  // int last_collision_;
  PhysVector position;
  PhysVector speed;
  // PhysVector momentum_;
  double radius = 1.;
  // PhysVector get_momentum();
  Particle(PhysVector Ipos, PhysVector Ispeed);
};

struct Box {
  double side;
  Box(double Iside);
};

struct Collision {
  char flag;
  double time;
  std::vector<Particle>::iterator firstPaticle;

  char wallCollision;  //-1 no collision, 0 down ...
  std::vector<Particle>::iterator secondPaticle;

  Collision(double Itime, std::vector<Particle>::iterator IfirstPaticle,
            char IwallCollision);
  Collision(double Itime, std::vector<Particle>::iterator IfirstPaticle,
            std::vector<Particle>::iterator IsecondPaticle);
};

class Gas {
  std::vector<Particle> particles_{};
  Box box_;
  void update_gas_state();  // called in each iteration of the game loop
  double time_impact(std::vector<Particle>::iterator a,
                     std::vector<Particle>::iterator b);
  double time_impact(std::vector<Particle>::iterator P1, char side);

 public:
  Gas(int n, double l);
  Collision find_iteration();
  // double get_pressure();     // returns the value of the pressure of the gas
  // double get_volume();       // returns the volume of the gas
  // double get_temperature();  // returns the temperature of the gas
};
}  // namespace thermo

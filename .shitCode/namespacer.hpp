#include <cmath>
#include <ctime>
#include <vector>

namespace thermo {

struct PhysVector {  // representation of a simple three-dimensional vector
  double x_, y_, z_;
};

struct Particle {
  static double mass_;
  static double radius_;

  PhysVector position_;
  PhysVector speed_;

	void move(double);
};

struct Wall {
  char wall_type_;
};

struct Cluster {
  std::vector<std::vector<Particle>::iterator> particles_iterators_;
  std::vector<Wall> walls_;
};

class Collision {
	std::vector<Cluster> clusters_;
	double time_;

	public:
	void set_time(const double);
	void set_time(const double, const double);
	void wipe();
	void insert_Cluster(const Cluster&, double);
	double get_time() const;
	void solve_collisions();
};

class Gas {
  std::vector<Particle> particles_;
  double side_;  // side of the cubical container

 public:
	Gas(const Gas&);
  Gas(int, double, double);
	
  Collision get_next_collision();
	
	void update_gas_state();  // called in each iteration of the game loop
	
	void output_particles_positions();

  // double get_pressure();     // returns the value of the pressure of the gas
  // double get_volume();       // returns the volume of the gas
  // double get_temperature();  // returns the temperature of the gas
};

}  // namespace thermo

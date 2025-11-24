#ifndef GAS_HPP
#define GAS_HPP

#include <vector>

#include "Collision.hpp"
#include "Particle.hpp"

namespace GS {
class SimDataPipeline;

class Gas {
 public:
  bool contains(const Particle& p);

  Gas() : particles{}, boxSide{1.}, time{0.} {}
  Gas(std::vector<Particle>&& particles, double boxSide, double time = 0.);
  // parametric constructor with particles distributed
  // in a cubical lattice and uniform distribution for
  // speed norm and direction
  Gas(size_t particlesN, double temperature, double boxSide,
      double time = 0.);

  const std::vector<Particle>& getParticles() const { return particles; }
  double getBoxSide() const { return boxSide; }
  double getTime() const { return time; }

	void simulate(size_t iterationsN);
  void simulate(size_t iterationsN, SimDataPipeline& SimOutput);
  // int getPIndex(const Particle* p); // what?
  // void operator=(const Gas& gas); // WHAT??

 private:
  std::vector<Particle> particles{};
  double boxSide;
  double time;

  PWCollision firstPWColl();
  PPCollision firstPPColl();
  void move(double dt);
  void solveNextEvt();
};
}  // namespace GS

#endif

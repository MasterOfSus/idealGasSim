#ifndef GASDATA_HPP
#define GASDATA_HPP

#include <stddef.h>

#include <vector>

#include "PhysicsEngine/Collision.hpp"
#include "PhysicsEngine/Particle.hpp"

namespace GS {

class Gas;

class GasData {
 public:
  // expects a solved collision
  GasData(Gas const& gas, Collision const* collision);

  char getCollType() const;

  std::vector<Particle> const& getParticles() const { return particles; }
  Particle const& getP1() const { return particles[p1Index]; }
  size_t getP1Index() const { return p1Index; }
  Particle const& getP2() const;
  size_t getP2Index() const;
  double getT0() const { return t0; }
  double getTime() const { return time; }
  double getBoxSide() const { return boxSide; }
  Wall getWall() const;

  bool operator==(GasData const& data) const;

 private:
  std::vector<Particle> particles;
  double t0;
  double time;
  double boxSide;
  size_t p1Index;
  size_t p2Index;
  Wall wall{Wall::VOID};
};
}  // namespace GS

#endif

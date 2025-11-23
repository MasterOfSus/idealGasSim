#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <atomic>

#include "Vector3.hpp"

namespace GS {

struct Particle {
  Vector3d position;
  Vector3d speed;

  static double getMass() { return mass.load(); }
  static void setMass(double m) { mass.store(m); }

  static double getRadius() { return radius.load(); }
  static void setRadius(double r) { radius.store(r); }

 private:
  static std::atomic<double> radius;
  static std::atomic<double> mass;
};

bool overlap(Particle const& p1, Particle const& p2);

}  // namespace GS

#endif

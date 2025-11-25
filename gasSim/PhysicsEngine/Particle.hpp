#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <atomic>

#include "GSVector.hpp"

namespace GS {

struct Particle {
  GSVectorD position;
  GSVectorD speed;

  static double getMass() { return mass.load(); }
  static void setMass(double m) { mass.store(m); }

  static double getRadius() { return radius.load(); }
  static void setRadius(double r) { radius.store(r); }

 private:
  static std::atomic<double> radius;
  static std::atomic<double> mass;
};

bool operator==(Particle const& p1, Particle const& p2);
bool overlap(Particle const& p1, Particle const& p2);
double energy (Particle const& p);

}  // namespace GS

#endif

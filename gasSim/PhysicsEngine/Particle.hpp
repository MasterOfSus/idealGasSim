#ifndef PARTICLE_HPP
#define PARTICLE_HPP

#include <atomic>

#include "GSVector.hpp"

namespace GS {

struct Particle {
  GSVectorD position;
  GSVectorD speed;

  static double getMass() { return mass.load(); }
  static void setMass(double m);
  static double getRadius() { return radius.load(); }
  static void setRadius(double r);

 private:
  inline static std::atomic<double> radius{1.};
  inline static std::atomic<double> mass{1.};
};

bool overlap(Particle const& p1, Particle const& p2);
double energy(Particle const& p);

bool operator==(Particle const& p1, Particle const& p2);

}  // namespace GS

#endif

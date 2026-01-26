#include "Particle.hpp"

#include <stdexcept>

#include "GSVector.hpp"
#include "Gas.hpp"

namespace GS {

void Particle::setMass(double m) {
  if (m <= 0.) {
    throw std::invalid_argument(
        "Particle setMass error: non-positive mass provided");
  } else {
    mass.store(m);
  }
}

void Particle::setRadius(double r) {
  if (r <= 0.) {
    throw std::invalid_argument(
        "Particle setRadius error: non-positive mass provided");
  } else if (static_cast<bool>(Gas::gasInstances())) {
    throw std::runtime_error(
        "Particle setRadius error: tried to change radius with " +
        std::to_string(Gas::gasInstances()) + " alive Gas instances");
  } else {
    radius.store(r);
  }
}

bool overlap(Particle const& p1, Particle const& p2) {
  return (p1.position - p2.position).norm() < 2. * Particle::getRadius();
}

double energy(Particle const& p) {
  return p.getMass() * p.speed * p.speed / 2.;
}

bool operator==(Particle const& p1, Particle const& p2) {
  return p1.position == p2.position && p1.speed == p2.speed;
}

}  // namespace GS

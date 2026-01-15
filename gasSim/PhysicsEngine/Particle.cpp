#include "Particle.hpp"

#include "GSVector.hpp"

namespace GS {

bool overlap(Particle const& p1, Particle const& p2) {
  return (p1.position - p2.position).norm() < 2. * Particle::getRadius();
}

double energy(Particle const& p) {
  return p.speed * p.speed * Particle::getMass() / 2.;
}

bool operator==(Particle const& p1, Particle const& p2) {
  return p1.position == p2.position && p1.speed == p2.speed;
}

}  // namespace GS

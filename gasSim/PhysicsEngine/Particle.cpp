#include "Particle.hpp"

bool GS::overlap(GS::Particle const& p1, GS::Particle const& p2) {
  return (p1.position - p2.position).norm() < Particle::getRadius();
}

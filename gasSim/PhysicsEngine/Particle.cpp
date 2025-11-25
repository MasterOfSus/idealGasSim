#include "Particle.hpp"

namespace GS {

bool operator==(Particle const& p1, Particle const& p2) {
	return p1.position == p2.position && p1.speed == p2.speed;
}

bool overlap(Particle const& p1, Particle const& p2) {
  return (p1.position - p2.position).norm() < Particle::getRadius();
}

double energy(Particle const& p) {
	return p.speed * p.speed * Particle::getMass() / 2.;
}

}

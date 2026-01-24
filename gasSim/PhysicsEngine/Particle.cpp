#include "Particle.hpp"

#include <stdexcept>

#include "GSVector.hpp"

namespace GS {

void Particle::setMass(double m) {
	if (m > 0.) {
		mass.store(m);
	} else {
		throw std::invalid_argument("Particle setMass error: non-positive mass provided");
	}
}

void Particle::setRadius(double r) {
	if (r > 0.) {
		radius.store(r);
	} else {
		throw std::invalid_argument("Particle setMass error: non-positive mass provided");
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

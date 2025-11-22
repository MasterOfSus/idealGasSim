#include <stdexcept>
#include <cassert>
#include "GasData.hpp"

// auxiliary getPIndex function

inline int getPIndex(const GS::Particle* p, const GS::Gas& gas) {
  return p - gas.getParticles().data();
}

// expects a solved collision as argument
GS::GasData::GasData(const Gas& gas, const Collision* collision) {
  if (!(gas.getParticles().data() <= collision->getP1() &&
        collision->getP1() <=
            gas.getParticles().data() + gas.getParticles().size()))
    throw std::invalid_argument(
      "Collision first particle does not belong to gas."
		);
  else {
    if (collision->getType() == 'p') {
      const PPCollision* coll{static_cast<const PPCollision*>(collision)};
      if (!(gas.getParticles().data() <= coll->getP2() &&
            coll->getP2() <=
                gas.getParticles().data() + gas.getParticles().size()))
        throw std::invalid_argument(
            "Collision second particle does not belong to gas.");
      else {
        particles = gas.getParticles();
        t0 = gas.getTime() - collision->getTime();
        time = gas.getTime();  // yes
        boxSide = gas.getBoxSide();
        p1Index = getPIndex(coll->getP1(), gas);
        p2Index = getPIndex(coll->getP2(), gas);
      }
    } else {
      particles = gas.getParticles();
      t0 = gas.getTime() - collision->getTime();
      time = gas.getTime();  // yes
      boxSide = gas.getBoxSide();
      // std::cout << "Adding p1_ at index " <<
      // getPIndex(collision->getFirstParticle(), gas) << '\n';
      p1Index = getPIndex(collision->getP1(), gas);
      p2Index = NAN;
      const PWCollision* coll{static_cast<const PWCollision*>(collision)};
      wall = coll->getWall();
    }
  }
}

char GS::GasData::getCollType() const {
  assert(p1Index >= 0 && p1Index <= static_cast<size_t>(particles.size()));
  if (p2Index == NAN) {
    return 'w';
  } else {
    return 'p';
	}
}

const GS::Particle& GS::GasData::getP2() const {
  if (getCollType() == 'w')
    throw std::logic_error("Asked for p2 in wall collision");
  else
    return particles[p2Index];
}

size_t GS::GasData::getP2Index() const {
  if (getCollType() == 'w')
    throw std::logic_error("Asked for p2 index in wall collision");
  else
    return p2Index;
}

GS::Wall GS::GasData::getWall() const {
  if (getCollType() == 'p')
    throw std::logic_error("Asked for wall from a particle collision.");
  else
    return wall;
}

bool GS::GasData::operator==(const GasData& data) const {
  return particles == data.particles && t0 == data.t0 &&
         time == data.time && boxSide == data.boxSide &&
         p1Index == data.p1Index && p2Index == data.p2Index &&
         wall == data.wall;
}


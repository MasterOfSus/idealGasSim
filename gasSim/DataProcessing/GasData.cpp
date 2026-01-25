#include "GasData.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>

#include "PhysicsEngine/Gas.hpp"

// auxiliary getPIndex function
inline long getPIndex(GS::Particle const* p, GS::Gas const& gas) {
  return static_cast<long>(p - gas.getParticles().data());
}

GS::GasData::GasData(Gas const& gas, Collision const* collision) {
	if (!collision->getP1()) {
		throw std::invalid_argument("GasData constructor error: provided nullptr as first particle pointer.");
	}
  if (!(gas.getParticles().data() <= collision->getP1() &&
        collision->getP1() <=
            gas.getParticles().data() + gas.getParticles().size()))
    throw std::invalid_argument(
        "GasData constructor error: collision first particle does not belong "
        "to gas.");
  else {
    if (collision->getType() == 'p') {
      PPCollision const* coll{static_cast<PPCollision const*>(collision)};
			if (!coll->getP2()) {
				throw std::invalid_argument("GasData constructor error: provided nullptr as second particle pointer.");
			}
      if (!(gas.getParticles().data() <= coll->getP2() &&
            coll->getP2() <=
                gas.getParticles().data() + gas.getParticles().size())) {
        throw std::invalid_argument(
            "GasData constructor error: collision second particle does not "
            "belong to gas.");
      } else {
        particles = gas.getParticles();
        t0 = gas.getTime() - collision->getTime();
        time = gas.getTime();
        boxSide = gas.getBoxSide();
        p1Index = static_cast<size_t>(getPIndex(coll->getP1(), gas));
        p2Index = static_cast<size_t>(getPIndex(coll->getP2(), gas));
        wall = Wall::VOID;
      }
    } else {
      particles = gas.getParticles();
      t0 = gas.getTime() - collision->getTime();
      time = gas.getTime();
      boxSide = gas.getBoxSide();
      p1Index = static_cast<size_t>(getPIndex(collision->getP1(), gas));
      p2Index = SIZE_MAX;
      PWCollision const* coll{static_cast<PWCollision const*>(collision)};
      wall = coll->getWall();
    }
  }
}

char GS::GasData::getCollType() const {
  assert(p1Index <= static_cast<size_t>(particles.size()));
  if (wall == Wall::VOID) {
    return 'p';
  } else {
    return 'w';
  }
}

GS::Particle const& GS::GasData::getP2() const {
  if (getCollType() == 'w')
    throw std::logic_error(
        "GasData::getP2 error: asked for p2 in wall collision");
  else
    return particles[p2Index];
}

size_t GS::GasData::getP2Index() const {
  if (getCollType() == 'w')
    throw std::logic_error(
        "GasData::getP2Index error: asked for p2 index in wall collision");
  else
    return p2Index;
}

GS::Wall GS::GasData::getWall() const {
  if (getCollType() == 'p')
    throw std::logic_error(
        "GasData::getWall error: asked for wall from a particle collision.");
  else
    return wall;
}

bool GS::GasData::operator==(GasData const& data) const {
  return particles == data.particles && t0 == data.t0 && time == data.time &&
         boxSide == data.boxSide && p1Index == data.p1Index &&
         p2Index == data.p2Index && wall == data.wall;
}

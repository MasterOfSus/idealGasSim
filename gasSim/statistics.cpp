#include "statistics.hpp"
#include "physicsEngine.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>

namespace gasSim {

// TdStats class
// Constructors
TdStats::TdStats(const Gas& firstState):
	wallPulses_ {},
	freePaths_ {},
	t0_(firstState.getTime()),
	time_(firstState.getTime()),
	boxSide_(firstState.getBoxSide())
	{
		std::transform(firstState.getParticles().begin(), firstState.getParticles().end(), std::back_inserter(speeds_), [](const Particle& p) { return p.speed; });
		lastPositions_ = std::vector<PhysVectorD>(getNParticles(), {0., 0., 0.});
}

TdStats::TdStats(const Gas& firstState, const TdStats& prevStats):
	wallPulses_ {},
	freePaths_ {},
	t0_(firstState.getTime()),
	time_(firstState.getTime()),
	boxSide_(firstState.getBoxSide())
	{
		if (firstState.getParticles().size() != prevStats.getNParticles())
			throw std::invalid_argument("Non-matching particle numbers in arguments.");
		else if (firstState.getBoxSide() != prevStats.getBoxSide())
			throw std::invalid_argument("Non-matching box sides.");
		else if (firstState.getTime() > prevStats.getTime())
			throw std::invalid_argument("Stats time is less than gas time.");
		else {
			lastPositions_ = prevStats.lastPositions_;
			std::transform(firstState.getParticles().begin(), firstState.getParticles().end(), std::back_inserter(speeds_), [](const Particle& p) { return p.speed; });
		}
}

void TdStats::addData(const Gas& gas, const Collision* collision) {
	if (gas.getParticles().size() != getNParticles()) {
		throw std::invalid_argument("Non-matching gas particles number.");
	} else if (gas.getTime() <= time_) {
		throw std::invalid_argument("Gas time is less than or equal to internal time.");
	} else if (collision->getTime() <= time_) {
		throw std::invalid_argument("Collision time is less than or equal to internal time.");
	} else if (collision->getTime() != gas.getTime()) {
		throw std::invalid_argument("Gas time less than collision time.");
	} else {
		time_ = gas.getTime();
		if (collision->getType() == "Wall") {
			const WallCollision* wallColl = static_cast<const WallCollision*>(collision);
			switch (wallColl->getWall()) {
				case Wall::Front:
					wallPulses_[0] -= 2. * wallColl->getFirstParticle()->speed.y;
					break;
				case Wall::Back:
					wallPulses_[1] -= 2. * wallColl->getFirstParticle()->speed.y;
					break;
				case Wall::Left:
					wallPulses_[2] -= 2. * wallColl->getFirstParticle()->speed.x;
					break;
				case Wall::Right:
					wallPulses_[3] -= 2. * wallColl->getFirstParticle()->speed.x;
					break;
				case Wall::Top:
					wallPulses_[4] -= 2. * wallColl->getFirstParticle()->speed.z;
					break;
				case Wall::Bottom:
					wallPulses_[5] -= 2. * wallColl->getFirstParticle()->speed.z;
					break;
			}

			if (lastPositions_[gas.getPIndex(wallColl->getFirstParticle())] != PhysVectorD({0., 0., 0.})) {
				PhysVectorD lastPos = lastPositions_[gas.getPIndex(wallColl->getFirstParticle())];
				freePaths_.emplace_back((wallColl->getFirstParticle()->position - lastPos).norm());
			}
			lastPositions_[gas.getPIndex(wallColl->getFirstParticle())] = wallColl->getFirstParticle()->position;
			speeds_[gas.getPIndex(wallColl->getFirstParticle())] = wallColl->getFirstParticle()->position;
		} else {
			const PartCollision* partColl = static_cast<const PartCollision*>(collision);
			if (lastPositions_[gas.getPIndex(partColl->getFirstParticle())] != PhysVectorD({0., 0., 0.})) {
				PhysVectorD lastPos {lastPositions_[gas.getPIndex(partColl->getFirstParticle())]};
				freePaths_.emplace_back(
						(partColl->getFirstParticle()->position - lastPos).norm()
						);
			}
			if (lastPositions_[gas.getPIndex(partColl->getSecondParticle())] != PhysVectorD({0., 0., 0.})) {
				PhysVectorD lastPos = lastPositions_[gas.getPIndex(partColl->getSecondParticle())];
				freePaths_.emplace_back(
						(partColl->getSecondParticle()->position - lastPos).norm()
						);
			}
			lastPositions_[
				gas.getPIndex(partColl->getFirstParticle())
			] = partColl->getFirstParticle()->position;
			lastPositions_[
				gas.getPIndex(partColl->getSecondParticle())
			] = partColl->getSecondParticle()->position;
			speeds_[
				gas.getPIndex(partColl->getFirstParticle())
			] = partColl->getFirstParticle()->speed;
			speeds_[
				gas.getPIndex(partColl->getSecondParticle())
			] = partColl->getSecondParticle()->speed;
		}
	}
}

double TdStats::getPressure(Wall wall) const {
	return wallPulses_[int(wall)] / getBoxSide()*getBoxSide();
}

double TdStats::getPressure() const {
	double totPulses {};
	int i {0};
	for (; i < 6; ++i) {
		totPulses += wallPulses_[i];
	}
	return totPulses / (getBoxSide() * getBoxSide() * 6);
}

/*double TdStats::getTemp() const {
	return [] ()
}*/

} // namespace gasSim

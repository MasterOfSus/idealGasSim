#include "statistics.hpp"
#include "physicsEngine.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>

namespace gasSim {

// TdStats class
// Constructors
TdStats::TdStats(const Gas* gas):
	gas_ (gas),
	wallPulses_ {},
	nParticles_(gas_->getParticles().size()),
	freePaths_ {},
	t0_(gas_->getTime()),
	time_(firstState.getTime()),
	boxSide_(firstState.getBoxSide())
	{
		lastCollTimes_ = std::vector<double>(nParticles_, -INFINITY);
		std::transform(firstState.getParticles().begin(), firstState.getParticles().end(), std::back_inserter(speeds_), [](const Particle& p) { return p.speed; });
}

TdStats::TdStats(const Gas& firstState, const TdStats& prevStats):
	wallPulses_ {},
	nParticles_(firstState.getParticles().size()),
	freePaths_(prevStats.freePaths_),
	t0_(firstState.getTime()),
	time_(firstState.getTime()),
	boxSide_(firstState.getBoxSide())
	{
		lastCollTimes_ = prevStats.lastCollTimes_;
		std::transform(firstState.getParticles().begin(), firstState.getParticles().end(), std::back_inserter(speeds_), [](const Particle& p) { return p.speed; });
}

void TdStats::addData(const Gas& gas, const Collision* collision) {
	if (gas.getParticles().size() != nParticles_) {
		throw std::invalid_argument("Wrong gas particles number.");
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

			if (lastCollTimes_[gas.getPIndex(wallColl->getFirstParticle())] != -INFINITY) {
				double t = lastCollTimes_[gas.getPIndex(wallColl->getFirstParticle())];
				freePaths_.emplace_back(wallColl->getFirstParticle()->speed.norm() * (time_ - t));
			}
			lastCollTimes_[gas.getPIndex(wallColl->getFirstParticle())] = time_;
		} else {
			const PartCollision* partColl = static_cast<const PartCollision*>(collision);
			if (lastCollTimes_[gas.getPIndex(partColl->getFirstParticle())] != -INFINITY) {
				double lastCollTime {lastCollTimes_[gas.getPIndex(partColl->getFirstParticle())]};
				freePaths_.emplace_back(
						partColl->getFirstParticle()->speed.norm() * (time_ - lastCollTime)
						);
			}
			if (lastCollTimes_[gas.getPIndex(partColl->getSecondParticle())] != -INFINITY) {
				double lastCollTime = lastCollTimes_[gas.getPIndex(partColl->getSecondParticle())];
				freePaths_.emplace_back(
						partColl->getSecondParticle()->speed.norm() * (time_ - lastCollTime)
						);
			}
			lastCollTimes_[
				gas.getPIndex(partColl->getFirstParticle())
			] = time_;
			lastCollTimes_[
				gas.getPIndex(partColl->getSecondParticle())
			] = time_;
		}
		std::transform(gas.getParticles().begin(), gas.getParticles().end(), std::back_inserter(speeds_),
[](const Particle& p) { return p.speed; });
	}
}

} // namespace gasSim

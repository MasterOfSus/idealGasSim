#include "statistics.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <stdexcept>

#include "physicsEngine.hpp"

namespace gasSim {

// TdStats class
// Constructors
TdStats::TdStats(const Gas& firstState)
    : wallPulses_{},
      freePaths_{},
      t0_(firstState.getTime()),
      time_(firstState.getTime()),
      boxSide_(firstState.getBoxSide()) {
  std::transform(firstState.getParticles().begin(),
                 firstState.getParticles().end(), std::back_inserter(speeds_),
                 [](const Particle& p) { return p.speed; });
  lastPositions_ = std::vector<PhysVectorD>(getNParticles(), {0., 0., 0.});
}

TdStats::TdStats(const Gas& firstState, const TdStats& prevStats)
    : wallPulses_{},
      freePaths_{},
      t0_(firstState.getTime()),
      time_(firstState.getTime()),
      boxSide_(firstState.getBoxSide()) {
  if (firstState.getParticles().size() != prevStats.getNParticles())
    throw std::invalid_argument("Non-matching particle numbers in arguments.");
  else if (firstState.getBoxSide() != prevStats.getBoxSide())
    throw std::invalid_argument("Non-matching box sides.");
  else if (firstState.getTime() > prevStats.getTime())
    throw std::invalid_argument("Stats time is less than gas time.");
  else {
    lastPositions_ = prevStats.lastPositions_;
    std::transform(firstState.getParticles().begin(),
                   firstState.getParticles().end(), std::back_inserter(speeds_),
                   [](const Particle& p) { return p.speed; });
  }
}

void TdStats::addData(const Gas& gas, const Collision* collision) {
  if (gas.getParticles().size() != getNParticles()) {
    throw std::invalid_argument("Non-matching gas particles number.");
  } else if (gas.getTime() < time_) {
    throw std::invalid_argument(
        "Gas time is less than or equal to internal time.");
  } else if (gas.getTime() + collision->getTime() < time_) {
    throw std::invalid_argument(
        "Collision time is less than or equal to internal time.");
  } else if (std::abs(gas.getTime() - collision->getTime() - time_) > 0.001)
    throw std::invalid_argument(
        "Non matching gas (" + std::to_string(gas.getTime()) + ") collision (" +
        std::to_string(collision->getTime()) + ") and stats (" +
        std::to_string(time_) + ") times.");
  else {
    time_ += collision->getTime();
    if (collision->getType() == "Wall") {
      const WallCollision* wallColl =
          static_cast<const WallCollision*>(collision);
      switch (wallColl->getWall()) {
        case Wall::Front:
          wallPulses_[0] +=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.y;
          break;
        case Wall::Back:
          wallPulses_[1] -=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.y;
          break;
        case Wall::Left:
          wallPulses_[2] +=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.x;
          break;
        case Wall::Right:
          wallPulses_[3] -=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.x;
          break;
        case Wall::Top:
          wallPulses_[4] +=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.z;
          break;
        case Wall::Bottom:  // ??????????
          wallPulses_[5] -=
              Particle::mass * 2. * wallColl->getFirstParticle()->speed.z;
          break;
      }

      if (lastPositions_[gas.getPIndex(wallColl->getFirstParticle())] !=
          PhysVectorD({0., 0., 0.})) {
        PhysVectorD lastPos =
            lastPositions_[gas.getPIndex(wallColl->getFirstParticle())];
        freePaths_.emplace_back(
            (wallColl->getFirstParticle()->position - lastPos).norm());
      }
      lastPositions_[gas.getPIndex(wallColl->getFirstParticle())] =
          wallColl->getFirstParticle()->position;
      speeds_[gas.getPIndex(wallColl->getFirstParticle())] =
          wallColl->getFirstParticle()->speed;
    } else {
      const PartCollision* partColl =
          static_cast<const PartCollision*>(collision);
      if (lastPositions_[gas.getPIndex(partColl->getFirstParticle())] !=
          PhysVectorD({0., 0., 0.})) {
        PhysVectorD lastPos{
            lastPositions_[gas.getPIndex(partColl->getFirstParticle())]};
        freePaths_.emplace_back(
            (partColl->getFirstParticle()->position - lastPos).norm());
      }
      if (lastPositions_[gas.getPIndex(partColl->getSecondParticle())] !=
          PhysVectorD({0., 0., 0.})) {
        PhysVectorD lastPos =
            lastPositions_[gas.getPIndex(partColl->getSecondParticle())];
        freePaths_.emplace_back(
            (partColl->getSecondParticle()->position - lastPos).norm());
      }
      lastPositions_[gas.getPIndex(partColl->getFirstParticle())] =
          partColl->getFirstParticle()->position;
      lastPositions_[gas.getPIndex(partColl->getSecondParticle())] =
          partColl->getSecondParticle()->position;
      speeds_[gas.getPIndex(partColl->getFirstParticle())] =
          partColl->getFirstParticle()->speed;
      speeds_[gas.getPIndex(partColl->getSecondParticle())] =
          partColl->getSecondParticle()->speed;
    }
  }
}

double TdStats::getPressure(Wall wall) const {
  return wallPulses_[int(wall)] / (getBoxSide() * getBoxSide() * getDeltaT());
}

double TdStats::getPressure() const {
  double totPulses{0};
  totPulses = std::accumulate(wallPulses_.begin(), wallPulses_.end(), 0.);
  return totPulses / (getBoxSide() * getBoxSide() * 6 * getDeltaT());
}

double TdStats::getTemp() const {
  double sqrSpeeds = std::accumulate(
      speeds_.begin(), speeds_.end(), 0.0,
      [](double acc, const PhysVectorD& v) { return acc + v * v; });
  return gasSim::Particle::mass * sqrSpeeds / speeds_.size();
}

double TdStats::getMeanFreePath() const {
  if (freePaths_.size() == 0) {
    throw std::logic_error(
        "Tried to get mean free path from blank free path data.");
  } else {
    double mfp{0};
    mfp = std::accumulate(freePaths_.begin(), freePaths_.end(), 0.);
    return mfp / freePaths_.size();
  }
}

}  // namespace gasSim

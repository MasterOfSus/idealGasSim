#include "TdStats.hpp"

#include <algorithm>
#include <cassert>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

#include "DataProcessing/GasData.hpp"
#include "PhysicsEngine/Collision.hpp"
#include "PhysicsEngine/Particle.hpp"

namespace GS {

// Constructors
TdStats::TdStats(GasData const& firstState, TH1D const& speedsHTemplate)
    : wallPulses{},
      lastCollPositions(std::vector<GSVectorD>(firstState.getParticles().size(),
                                               {0., 0., 0.})),
      T{std::accumulate(
            firstState.getParticles().begin(), firstState.getParticles().end(),
            0., [](double x, Particle const& p) { return x + energy(p); }) *
        2. / static_cast<double>(getNParticles()) / 3.},
      freePaths{},
      t0(firstState.getT0()),
      time(firstState.getTime()),
      boxSide(firstState.getBoxSide()) {
  if (speedsHTemplate.GetEntries() != 0.) {
    throw std::invalid_argument(
        "TdStats constructor error: non-empty speedsH template provided");
  }
  speedsH = speedsHTemplate;
  speedsH.SetDirectory(nullptr);
  if (firstState.getCollType() == 'w') {
    addPulse(firstState);
  } else if (firstState.getCollType() == 'p') {
    lastCollPositions[firstState.getP2Index()] = firstState.getP2().position;
  }
  lastCollPositions[firstState.getP1Index()] = firstState.getP1().position;
  std::for_each(firstState.getParticles().begin(),
                firstState.getParticles().end(),
                [this](Particle const& p) { speedsH.Fill(p.speed.norm()); });
}

bool isNegligible(double epsilon, double x) {
  return std::fabs(epsilon / x) < 1E-6;
}

TdStats::TdStats(GasData const& data, TdStats&& prevStats)
    : wallPulses{},
      T{std::accumulate(
            data.getParticles().begin(), data.getParticles().end(), 0.,
            [](double x, Particle const& p) { return x + energy(p); }) *
        2. / static_cast<double>(data.getParticles().size()) / 3.},
      freePaths{},
      t0{data.getT0()},
      time{data.getTime()},
      boxSide{data.getBoxSide()} {
  if (data.getParticles().size() != prevStats.getNParticles()) {
    throw std::invalid_argument(
        "TdStats constructor error: provided data with non-matching particle "
        "number");
  } else if (data.getBoxSide() != prevStats.boxSide) {
    throw std::invalid_argument(
        "TdStats constructor error: provided data with non-matching box side");
  } else if (
			!isNegligible(t0 - prevStats.time, t0 + prevStats.time) ||
			time <= prevStats.time) {
    throw std::invalid_argument(
        "TdStats constructor error: provided non time-contiguous data and TdStats");
  } else if (!isNegligible(T - prevStats.T, prevStats.T)) {
    throw std::invalid_argument(
        "TdStats constructor error: provided gas and stats with non-matching "
        "temperatures");
  } else {
    prevStats.speedsH.Reset("ICES");
    speedsH = prevStats.speedsH;
    speedsH.SetDirectory(nullptr);
    lastCollPositions = std::move(prevStats.lastCollPositions);

    if (data.getCollType() == 'w') {
      addPulse(data);
      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }
    } else if (data.getCollType() == 'p') {
      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }
      if (lastCollPositions[data.getP2Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP2().position - lastCollPositions[data.getP2Index()])
                .norm());
      }
      lastCollPositions[data.getP2Index()] = data.getP2().position;
    }
    lastCollPositions[data.getP1Index()] = data.getP1().position;

    std::for_each(data.getParticles().begin(), data.getParticles().end(),
                  [this](Particle const& p) { speedsH.Fill(p.speed.norm()); });
  }
}

TdStats::TdStats(GasData const& data, TdStats&& prevStats,
                 TH1D const& speedsHTemplate)
    : wallPulses{},
      T{std::accumulate(
            data.getParticles().begin(), data.getParticles().end(), 0.,
            [](double x, Particle const& p) { return x + energy(p); }) *
        2. / static_cast<double>(data.getParticles().size()) / 3.},
      freePaths{},
      t0(data.getT0()),
      time(data.getTime()),
      boxSide(data.getBoxSide()) {
  if (data.getParticles().size() != prevStats.getNParticles()) {
    throw std::invalid_argument(
        "TdStats constructor error: provided data with different particle "
        "number");
  } else if (data.getBoxSide() != prevStats.boxSide) {
    throw std::invalid_argument(
        "TdStats constructor error: provided data with different box side");
  } else if (
			!isNegligible(t0 - prevStats.time, t0 + prevStats.time) ||
			time <= prevStats.time) {
    throw std::invalid_argument(
        "TdStats constructor error: provided non time-contiguous data and TdStats");
  } else if (!isNegligible(T - prevStats.T, prevStats.T + T)) {
    throw std::invalid_argument(
        "TdStats constructor error: provided gas and stats with non-matching "
        "temperatures");
  } else {
    {
      TH1D* defH = new TH1D();
			defH->SetDirectory(nullptr);
      if (speedsHTemplate.IsEqual(defH)) {
				prevStats.speedsH.Reset("ICES");
        speedsH = prevStats.speedsH;
        speedsH.SetDirectory(nullptr);
      } else {
        if (speedsHTemplate.GetEntries() != 0.) {
          delete defH;
          throw std::runtime_error(
              "TdStats constructor error: provided non-empty speedsH template");
        } else {
          speedsH = speedsHTemplate;
					speedsH.SetDirectory(nullptr);
        }
      }
      delete defH;
    }

    lastCollPositions = std::move(prevStats.lastCollPositions);
    T = prevStats.T;

    if (data.getCollType() == 'w') {
      addPulse(data);
      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }
    } else if (data.getCollType() == 'p') {
      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }
      if (lastCollPositions[data.getP2Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP2().position - lastCollPositions[data.getP2Index()])
                .norm());
      }
      lastCollPositions[data.getP2Index()] = data.getP2().position;
    }
    lastCollPositions[data.getP1Index()] = data.getP1().position;

    std::for_each(data.getParticles().begin(), data.getParticles().end(),
                  [this](Particle const& p) { speedsH.Fill(p.speed.norm()); });
  }
}

TdStats::TdStats(TdStats const& s)
    : wallPulses(s.wallPulses),
      lastCollPositions(s.lastCollPositions),
      T(s.T),
      freePaths(s.freePaths),
      speedsH(s.speedsH),
      t0(s.t0),
      time(s.time),
      boxSide(s.boxSide) {
  speedsH.SetDirectory(nullptr);
}

TdStats::TdStats(TdStats&& s) noexcept
    : wallPulses(std::move(s.wallPulses)),
      lastCollPositions(std::move(s.lastCollPositions)),
      T(s.T),
      freePaths(std::move(s.freePaths)),
      speedsH(s.speedsH),
      t0(s.t0),
      time(s.time),
      boxSide(s.boxSide) {
  speedsH.SetDirectory(nullptr);
}

TdStats& TdStats::operator=(TdStats const& s) {
  if (this == &s) {
    return *this;
  }
  wallPulses = s.wallPulses;
  lastCollPositions = s.lastCollPositions;
  T = s.T;
  freePaths = s.freePaths;
  speedsH = s.speedsH;
  speedsH.SetDirectory(nullptr);
  t0 = s.t0;
  time = s.time;
  boxSide = s.boxSide;
  return *this;
}

TdStats& TdStats::operator=(TdStats&& s) noexcept {
  wallPulses = std::move(s.wallPulses);
  lastCollPositions = std::move(s.lastCollPositions);
  T = s.T;
  freePaths = std::move(s.freePaths);
  speedsH = s.speedsH;
  speedsH.SetDirectory(nullptr);
  t0 = s.t0;
  time = s.time;
  boxSide = s.boxSide;
  return *this;
}

void TdStats::addData(GasData const& data) {
  if (data.getParticles().size() != getNParticles()) {
    throw std::invalid_argument(
        "TdStats addData error: non-matching gas particles number");
  } else if (
			!isNegligible(data.getT0() - time, time + data.getT0()) ||
			data.getTime() <= time) {
    throw std::invalid_argument(
        "TdStats addData error: provided non-time contiguous GasData instance");
  } else if (data.getBoxSide() != boxSide) {
		throw std::invalid_argument(
				"TdStats addData error: provided non-matching data box side"
		);
	} else if (!isNegligible(std::accumulate(
            data.getParticles().begin(), data.getParticles().end(), 0.,
            [](double x, Particle const& p) { return x + energy(p); }) *
        2. / static_cast<double>(data.getParticles().size()) / 3. - T, T)) {
		throw std::invalid_argument(
				"TdStats addData error: provided non-matching data temperature"
		);
	} else {
    time = data.getTime();
    if (data.getCollType() == 'w') {
      addPulse(data);

      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }

      lastCollPositions[data.getP1Index()] = data.getP1().position;

    } else {
      if (lastCollPositions[data.getP1Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP1().position - lastCollPositions[data.getP1Index()])
                .norm());
      }
      if (lastCollPositions[data.getP2Index()] != GSVectorD({0., 0., 0.})) {
        freePaths.emplace_back(
            (data.getP2().position - lastCollPositions[data.getP2Index()])
                .norm());
      }

      lastCollPositions[data.getP1Index()] = data.getP1().position;
      lastCollPositions[data.getP2Index()] = data.getP2().position;
    }
    std::for_each(data.getParticles().begin(), data.getParticles().end(),
                  [this](Particle const& p) { speedsH.Fill(p.speed.norm()); });
  }
}

// Auxiliary addPulse private function, assumes solved collision for input
void TdStats::addPulse(GasData const& data) {
  assert(data.getCollType() == 'w');
  switch (data.getWall()) {
    case Wall::Front:
      wallPulses[0] += Particle::getMass() * 2. * data.getP1().speed.y;
      break;
    case Wall::Back:
      wallPulses[1] -= Particle::getMass() * 2. * data.getP1().speed.y;
      break;
    case Wall::Left:
      wallPulses[2] += Particle::getMass() * 2. * data.getP1().speed.x;
      break;
    case Wall::Right:
      wallPulses[3] -= Particle::getMass() * 2. * data.getP1().speed.x;
      break;
    case Wall::Top:
      wallPulses[4] -= Particle::getMass() * 2. * data.getP1().speed.z;
      break;
    case Wall::Bottom:
      wallPulses[5] += Particle::getMass() * 2. * data.getP1().speed.z;
      break;
    default:
      throw std::invalid_argument("addPulse error: VOID wall type provided");
  }
}

double TdStats::getPressure(Wall wall) const {
  if (wall == Wall::VOID) {
    throw std::invalid_argument("getPressure error: VOID wall provided");
  }
  return wallPulses[static_cast<size_t>(wall)] /
         (getBoxSide() * getBoxSide() * getDeltaT());
}

double TdStats::getPressure() const {
  double totPulses{std::accumulate(wallPulses.begin(), wallPulses.end(), 0.)};
  return totPulses / (getBoxSide() * getBoxSide() * 6 * getDeltaT());
}

double TdStats::getMeanFreePath() const {
  if (freePaths.size() == 0) {
    return -1.;
  } else {
    return std::accumulate(freePaths.begin(), freePaths.end(), 0.) /
           static_cast<double>(freePaths.size());
  }
}

bool TdStats::operator==(TdStats const& stats) const {
  return wallPulses == stats.wallPulses &&
         lastCollPositions == stats.lastCollPositions &&
         freePaths == stats.freePaths && t0 == stats.t0 && time == stats.time &&
         T == stats.T;
}

}  // namespace GS

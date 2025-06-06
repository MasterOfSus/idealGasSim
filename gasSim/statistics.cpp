#include "statistics.hpp"

#include <SFML/Graphics/RenderTexture.hpp>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <ratio>
#include <stdexcept>
#include <thread>
#include <utility>

#include "graphics.hpp"
#include "physicsEngine.hpp"

namespace gasSim {

// TdStats class

// Auxiliary addPulse private function, assumes solved collision for input

void TdStats::addPulse(const GasData& data) {
  assert(data.getCollType() == 'w');
  switch (data.getWall()) {
    case Wall::Front:
      wallPulses_[0] += Particle::mass * 2. * data.getP1().speed.y;
      break;
    case Wall::Back:
      wallPulses_[1] -= Particle::mass * 2. * data.getP1().speed.y;
      break;
    case Wall::Left:
      wallPulses_[2] += Particle::mass * 2. * data.getP1().speed.x;
      break;
    case Wall::Right:
      wallPulses_[3] -= Particle::mass * 2. * data.getP1().speed.x;
      break;
    case Wall::Top:
      wallPulses_[4] -= Particle::mass * 2. * data.getP1().speed.z;
      break;
    case Wall::Bottom:  // Yes
      wallPulses_[5] += Particle::mass * 2. * data.getP1().speed.z;
      break;
  }
}

// Constructors

// note to self: to correctly add pulses and pressures, one cannot take all
// collisions: those collisions which come from a state of non-collision cannot
// be counted in the time score, therefore one must initialize with t0_ as the
// time of first collision, not the starting time for the first collision. This
// behaviour is avoided completely though if the stats instance is not
// initialized with a first simulation step collision-gas couple. The drawback
// is minimal anyways, but if the user wishes to have perfectly coherent data he
// must acknowledge this concept and avoid this behaviour.

TdStats::TdStats(const GasData& firstState)
    : wallPulses_{},
      lastCollPositions_(std::vector<PhysVectorD>(
          firstState.getParticles().size(), {0., 0., 0.})),
      freePaths_{},
      t0_(firstState.getT0()),
      time_(firstState.getTime()),
      boxSide_(firstState.getBoxSide()) {
  T_ = std::accumulate(
           firstState.getParticles().begin(), firstState.getParticles().end(),
           0.,
           [](double x, const Particle& p) { return x + p.speed * p.speed; }) *
       Particle::mass / getNParticles();
  if (firstState.getCollType() == 'w') {
    addPulse(firstState);
  } else if (firstState.getCollType() == 'p') {
    lastCollPositions_[firstState.getP2Index()] = firstState.getP2().position;
  }
  lastCollPositions_[firstState.getP1Index()] = firstState.getP1().position;
}

TdStats::TdStats(const GasData& data, const TdStats& prevStats)
    : wallPulses_{},
      freePaths_{},
      t0_(data.getT0()),
      time_(data.getTime()),
      boxSide_(data.getBoxSide()) {
  if (data.getParticles().size() != prevStats.getNParticles()) {
    throw std::invalid_argument("Non-matching particle numbers in arguments.");
  } else if (data.getBoxSide() != prevStats.getBoxSide()) {
    throw std::invalid_argument("Non-matching box sides.");
  } else if (data.getTime() < prevStats.getTime()) {
    throw std::invalid_argument("Stats time is less than gas time.");
  } else if (std::accumulate(data.getParticles().begin(),
                             data.getParticles().end(), 0.,
                             [](double x, const Particle& p) {
                               return x + p.speed * p.speed;
                             }) != prevStats.getTemp()) {
    throw std::invalid_argument("Non-matching temperatures.");
  } else {
    lastCollPositions_ = prevStats.lastCollPositions_;
    T_ = prevStats.T_;
    if (data.getCollType() == 'w') {
      addPulse(data);
    } else if (data.getCollType() == 'p') {
      lastCollPositions_[data.getP2Index()] = data.getP2().position;
    }
    lastCollPositions_[data.getP1Index()] = data.getP1().position;
  }
}

void TdStats::addData(const GasData& data) {
  if (data.getParticles().size() != getNParticles())
    throw std::invalid_argument("Non-matching gas particles number.");
  else if (data.getTime() < time_)
    throw std::invalid_argument("Data time less than internal time.");
  else {
    time_ = data.getTime();
    if (data.getCollType() == 'w') {
      addPulse(data);

      PhysVectorD lastPos{lastCollPositions_[data.getP1Index()]};

      if (lastCollPositions_[data.getP1Index()] != PhysVectorD({0., 0., 0.})) {
        freePaths_.emplace_back(
            (data.getP1().position - lastCollPositions_[data.getP1Index()])
                .norm());
      }

      lastCollPositions_[data.getP1Index()] = data.getP1().position;

    } else {
      if (lastCollPositions_[data.getP1Index()] != PhysVectorD({0., 0., 0.})) {
        freePaths_.emplace_back(
            (data.getP1().position - lastCollPositions_[data.getP1Index()])
                .norm());
      }
      if (lastCollPositions_[data.getP2Index()] != PhysVectorD({0., 0., 0.})) {
        freePaths_.emplace_back(
            (data.getP2().position - lastCollPositions_[data.getP2Index()])
                .norm());
      }

      lastCollPositions_[data.getP1Index()] = data.getP1().position;
      lastCollPositions_[data.getP2Index()] = data.getP2().position;
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

double TdStats::getMeanFreePath() const {
  if (freePaths_.size() == 0) {
    throw std::logic_error(
        "Tried to get mean free path from blank free path data.");
  } else {
    return std::accumulate(freePaths_.begin(), freePaths_.end(), 0.) /
           freePaths_.size();
  }
}

bool TdStats::operator==(const TdStats& stats) const {
  return wallPulses_ == stats.wallPulses_ &&
         lastCollPositions_ == stats.lastCollPositions_ &&
         freePaths_ == stats.freePaths_ && t0_ == stats.t0_ &&
         time_ == stats.time_ && T_ == stats.T_;
}

// GasData methods

// auxiliary getPIndex function

inline int getPIndex(const Particle* p, const Gas& gas) {
  return p - gas.getParticles().data();
}

// expects a solved collision as argument
GasData::GasData(const Gas& gas, const Collision* collision) {
  if (!(gas.getParticles().data() <= collision->getFirstParticle() &&
        collision->getFirstParticle() <=
            gas.getParticles().data() + gas.getParticles().size()))
    throw std::invalid_argument(
        "Collision first particle does not belong to gas.");
  else {
    if (collision->getType() == "Particle Collision") {
      const PartCollision* coll{static_cast<const PartCollision*>(collision)};
      if (!(gas.getParticles().data() <= coll->getSecondParticle() &&
            coll->getSecondParticle() <=
                gas.getParticles().data() + gas.getParticles().size()))
        throw std::invalid_argument(
            "Collision second particle does not belong to gas.");
      else {
        particles_ = gas.getParticles();
        t0_ = gas.getTime() - collision->getTime();
        time_ = gas.getTime();  // yes
        boxSide_ = gas.getBoxSide();
        p1Index_ = getPIndex(coll->getFirstParticle(), gas);
        p2Index_ = getPIndex(coll->getSecondParticle(), gas);
      }
    } else {
      particles_ = gas.getParticles();
      t0_ = gas.getTime() - collision->getTime();
      time_ = gas.getTime();  // yes
      boxSide_ = gas.getBoxSide();
      // std::cout << "Adding p1_ at index " <<
      // getPIndex(collision->getFirstParticle(), gas) << '\n';
      p1Index_ = getPIndex(collision->getFirstParticle(), gas);
      p2Index_ = -1;
      const WallCollision* coll{static_cast<const WallCollision*>(collision)};
      wall_ = coll->getWall();
    }
  }
}

char GasData::getCollType() const {
  assert(p1Index_ >= 0 && p1Index_ <= static_cast<int>(particles_.size()));
  if (p2Index_ == -1) {
    // check that wall exists??
    return 'w';
  } else
    return 'p';
}

const Particle& GasData::getP2() const {
  if (getCollType() == 'w')
    throw std::logic_error("Asked for p2 in wall collision");
  else
    return particles_[p2Index_];
}

int GasData::getP2Index() const {
  if (getCollType() == 'w')
    throw std::logic_error("Asked for p2 index in wall collision");
  else
    return p2Index_;
}

Wall GasData::getWall() const {
  if (getCollType() == 'p')
    throw std::logic_error("Asked for wall from a particle collision.");
  else
    return wall_;
}

bool GasData::operator==(const GasData& data) const {
  return particles_ == data.particles_ && t0_ == data.t0_ &&
         time_ == data.time_ && boxSide_ == data.boxSide_ &&
         p1Index_ == data.p1Index_ && p2Index_ == data.p2Index_ &&
         wall_ == data.wall_;
}

// SimOutput methods

// wondering if I should even keep these two setters?

void SimOutput::setFramerate(double framerate) {
  if (framerate <= 0) {
    throw std::invalid_argument("Non-positive framerate provided.");
  } else {
    gDeltaT_ = 1. / framerate;
  }
}

SimOutput::SimOutput(unsigned int statSize, double framerate)
    : statSize_(statSize) {
  setFramerate(framerate);
}

void SimOutput::addData(const GasData& data) {
  if (nParticles_ == 0)
    if (data.getParticles().size() != 0)
      nParticles_ = data.getParticles().size();
  if (data.getParticles().size() != nParticles_)
    throw std::invalid_argument("Non-matching particle numbers.");
  else {
    std::lock_guard<std::mutex> rawDataGuard {rawDataMtx_};
    if (rawData_.size() > 0) {
      if (data.getTime() < rawData_.back().getTime()) {
        throw std::invalid_argument(
            "Argument time less than latest raw data piece time");
      }
    }
    rawData_.emplace_back(data);
		std::lock_guard<std::mutex> doneGuard {doneMtx_};
    done_ = false;
		rawDataCv_.notify_one();
  }
}

void SimOutput::processData() {
  std::vector<GasData> data {};
  std::unique_lock<std::mutex> rawDataLock(rawDataMtx_, std::defer_lock);
  while (true) {
		rawDataLock.lock();
    rawDataCv_.wait_for(rawDataLock, std::chrono::milliseconds(100),
  		[this] {
			std::lock_guard<std::mutex> doneGuard(doneMtx_);
			return !rawData_.empty() || done_;
			}
		);
		{ // doneGuard scope
		std::lock_guard<std::mutex> doneGuard {doneMtx_};
		if (done_ && rawData_.empty()) {
      break;
    }
		} // end of doneGuard scope
    int nStats{static_cast<int>(rawData_.size() / statSize_)};
    // std::cout << "nStats for this iteration is " << rawData_.size() << "/" << statSize_ << " = " << nStats << std::endl;

		if (nStats != 0) {
    	std::move(rawData_.begin(), rawData_.begin() + nStats * statSize_,
    	          std::back_inserter(data));
    	// std::cout << "GasData count = " << data.size() << std::endl;
    	rawData_.erase(rawData_.begin(), rawData_.begin() + nStats * statSize_);
    	rawDataLock.unlock();

			//std::cout << "Data size: " << data.size() << "; ";
    	processStats(data);
			/*{
			std::lock_guard<std::mutex> guard {statsMtx_};
			//std::cout << "Stats size: " << stats_.size() << std::endl;
			}*/
		} else {
			rawDataLock.unlock();
		}
		data.clear();
  }
}

void SimOutput::processData(const Camera& camera, const RenderStyle& style,
                            bool stats) {
  // std::cout << "Started processing data.\n";
  std::vector<GasData> data {};
	std::unique_lock<std::mutex> rawDataLock {rawDataMtx_, std::defer_lock};
  while (true) {
		rawDataLock.lock();
    rawDataCv_.wait_for(rawDataLock, std::chrono::milliseconds(100),
		[this] {
		std::lock_guard<std::mutex> doneGuard {doneMtx_};
		return !rawData_.empty() || done_; });
    // std::cout << "Done status: " << done_ << ". RawData emptiness: " <<
    // rawData_.empty() << std::endl;
		{ // doneGuard scope
		std::lock_guard<std::mutex> doneGuard {doneMtx_};
    if (done_ && rawData_.empty()) {
      break;
    }
		} // end of doneGuard scope
    int nStats{static_cast<int>(rawData_.size() / statSize_)};

    std::move(rawData_.begin(), rawData_.begin() + nStats * statSize_,
              std::back_inserter(data));
    rawData_.erase(rawData_.begin(), rawData_.begin() + nStats * statSize_);
    rawDataLock.unlock();

    std::thread sThread;
    std::thread gThread;

    if (stats)
      sThread = std::thread([this, &data]() {
        try {
          processStats(data);
        } catch (const std::exception& e) {
          std::cerr << "Error in stats thread: " << e.what() << std::endl;
          std::terminate();
        }
      });

    gThread = std::thread([this, &data, &camera, &style]() {
      try {
        processGraphics(data, camera, style);
      } catch (const std::exception& e) {
        std::cerr << "Error in graphics thread: " << e.what() << std::endl;
        std::terminate();
      }
    });

    if (stats && sThread.joinable()) sThread.join();
    if (gThread.joinable()) gThread.join();
		data.clear();
  }
}

// So alright basically yeah I'm trying to guarantee a constant interval between
// renders, even across function calls, which means the time variable must be
// initialized with the same value it was left at at the previous function call.
// This necessitates a class invariant of providing data sets which are
// subsequent in time. Lol.

void SimOutput::processGraphics(const std::vector<GasData>& data,
                                const Camera& camera,
                                const RenderStyle& style) {
  // std::cout << "Processing graphics... ";
  // std::cout.flush();
  std::vector<sf::Texture> renders{};
  if (!gTime_.has_value()) gTime_ = data[0].getTime() - gDeltaT_;
  sf::RenderTexture picture;

  assert(gDeltaT_ > 0.);
  assert(*gTime_ <= data[0].getTime());

  renders.reserve((data.back().getTime() - data.front().getTime()) / gDeltaT_ +
                  1);

  for (const GasData& dat : data) {
    while (*gTime_ + gDeltaT_ <= dat.getTime()) {
      // std::cout << "Drawing gas at time " << *gTime_ + gDeltaT_ << std::endl;
      gTime_ = *gTime_ + gDeltaT_;
      drawGas(dat, camera, picture, style, *gTime_ - dat.getTime());
      renders.emplace_back(std::move(picture.getTexture()));
    }
  }

	std::lock_guard<std::mutex> rendersGuard {rendersMtx_};
  std::move(renders.begin(), renders.end(), std::back_inserter(renders_));
  // std::cout << "done!\n";
}

void SimOutput::processStats(const std::vector<GasData>& data) {
  // std::cout << "Started processing stats.\n";
  std::vector<TdStats> stats{};

  assert(!std::div(data.size(), statSize_).rem);

  stats.reserve(std::div(data.size(), statSize_).quot);

  // std::cout << "Reached pre-loop\n";
  for (int i{0}; i < std::div(data.size(), statSize_).quot; ++i) {
    TdStats stat{data[i * statSize_]};
    for (int j{1}; j < statSize_; ++j) {
      stat.addData(data[i * statSize_ + j]);
    }
    stats.emplace_back(stat);
  }
  // std::cout << "Done with loop\n";
	std::lock_guard<std::mutex> statsGuard {statsMtx_};
  std::move(stats.begin(), stats.end(), std::back_inserter(stats_));
}

std::vector<TdStats> SimOutput::getStats(bool emptyQueue) {
  if (emptyQueue) {
    std::deque<TdStats> stats;
		{
		std::lock_guard<std::mutex> guard {statsMtx_};
    stats = {std::exchange(stats_, {})};
		stats_.clear();
		}
		return std::vector<TdStats>(stats.begin(), stats.end());
  } else {
    std::lock_guard<std::mutex> guard(statsMtx_);
    return std::vector<TdStats>(stats_.begin(), stats_.end());
  }
}

std::vector<sf::Texture> SimOutput::getRenders(bool emptyQueue) {
  if (emptyQueue) {
		std::lock_guard<std::mutex> rendersGuard {rendersMtx_};
    std::vector<sf::Texture> renders(std::make_move_iterator(renders_.begin()),
                                     std::make_move_iterator(renders_.end()));
    renders_.clear();
    return renders;
  } else {
    std::lock_guard<std::mutex> guard(rendersMtx_);
    return std::vector<sf::Texture>(renders_.begin(), renders_.end());
  }
}

void SimOutput::setDone() {
	std::lock_guard<std::mutex> guard {doneMtx_};
	done_ = true;
}

bool SimOutput::isDone() {
	std::lock_guard<std::mutex> guard {doneMtx_};
	return done_;
}

}  // namespace gasSim

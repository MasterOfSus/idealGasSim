#include "statistics.hpp"

#include <SFML/Config.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <ios>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <execution>

#include <RtypesCore.h>
#include <TClass.h>
#include <TH1D.h>
#include <TAxis.h>
#include <TGraph.h>
#include <TMultiGraph.h>
#include <TCanvas.h>
#include <TImage.h>

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

TdStats::TdStats(const GasData& firstState, const TH1D& speedsHTemplate)
    : wallPulses_{},
      lastCollPositions_(std::vector<PhysVectorD>(
          firstState.getParticles().size(), {0., 0., 0.})),
      freePaths_{},
      t0_(firstState.getT0()),
      time_(firstState.getTime()),
      boxSide_(firstState.getBoxSide()) {
	if (speedsHTemplate.GetEntries() != 0.) {
		throw std::invalid_argument("Non-empty speedsH template provided.");
	}
	speedsH_ = speedsHTemplate;
	speedsH_.SetDirectory(nullptr);
  T_ = std::accumulate(
           firstState.getParticles().begin(), firstState.getParticles().end(),
           0.,
           [](double x, const Particle& p) { return x + p.speed * p.speed; }) *
       Particle::mass / getNParticles() / 3.;
  if (firstState.getCollType() == 'w') {
    addPulse(firstState);
  } else if (firstState.getCollType() == 'p') {
    lastCollPositions_[firstState.getP2Index()] = firstState.getP2().position;
  }
  lastCollPositions_[firstState.getP1Index()] = firstState.getP1().position;
	std::for_each(
			firstState.getParticles().begin(), firstState.getParticles().end(),
			[this] (const Particle& p) {
			//	std::cerr << "Filling histo with value " << p.speed.norm() << std::endl;
				speedsH_.Fill(p.speed.norm());
				//std::cerr << "speedsH_ entries = " << speedsH_.GetEntries()
				//<< ", bin content for last added occourrence = "
				//<< speedsH_.GetBinContent(speedsH_.FindBin(p.speed.norm()))
				//<< ", bin number = " << speedsH_.GetNbinsX() << std::endl;
			}
	);
}

bool isNegligible(double epsilon, double x);

TdStats::TdStats(const GasData& data, const TdStats& prevStats)
    : wallPulses_{},
      freePaths_{},
      t0_(data.getT0()),
      time_(data.getTime()),
      boxSide_(data.getBoxSide()) {
  if (data.getParticles().size() != prevStats.getNParticles()) {
		std::cout << "data.getParticles size = " << data.getParticles().size() << " != " << prevStats.getNParticles() << " prevStats.getNParticles." << std::endl;
    throw std::invalid_argument("Non-matching particle numbers in arguments.");
  } else if (data.getBoxSide() != prevStats.getBoxSide()) {
    throw std::invalid_argument("Non-matching box sides.");
  } else if (data.getTime() < prevStats.getTime()) {
    throw std::invalid_argument("Stats time is less than gas time.");
  } else if (!isNegligible(
				std::accumulate(
					data.getParticles().begin(),
          data.getParticles().end(), 0.,
          [](double x, const Particle& p) {
            return x + p.speed * p.speed;
          }) * Particle::mass /	data.getParticles().size() / 3. - prevStats.getTemp(),
				prevStats.getTemp())) {
    throw std::invalid_argument("Non-matching temperatures for provided GasData.");
  } else {
		speedsH_ = prevStats.speedsH_;
		speedsH_.Reset("ICES");
		speedsH_.SetDirectory(nullptr);
    lastCollPositions_ = prevStats.lastCollPositions_;
    T_ = prevStats.T_;
    if (data.getCollType() == 'w') {
      addPulse(data);
    } else if (data.getCollType() == 'p') {
      lastCollPositions_[data.getP2Index()] = data.getP2().position;
    }
    lastCollPositions_[data.getP1Index()] = data.getP1().position;
		std::for_each(
				data.getParticles().begin(), data.getParticles().end(),
				[this] (const Particle& p) {
					//std::cerr << "Filling histo with value " << p.speed.norm() << std::endl;
					speedsH_.Fill(p.speed.norm());
					//std::cerr << "speedsH_ entries = " << speedsH_.GetEntries()
					//<< ", bin content for last added occourrence = "
					//<< speedsH_.GetBinContent(speedsH_.FindBin(p.speed.norm())) 
					//<< ", bin number = " << speedsH_.GetNbinsX() << std::endl;
				}
		);
  }
}

TdStats::TdStats(const GasData& data, const TdStats& prevStats, const TH1D& speedsHTemplate)
    : wallPulses_{},
      freePaths_{},
      t0_(data.getT0()),
      time_(data.getTime()),
      boxSide_(data.getBoxSide()) {
  if (data.getParticles().size() != prevStats.getNParticles()) {
		std::cout << "data.getParticles size = " << data.getParticles().size() << " != " << prevStats.getNParticles() << " prevStats.getNParticles." << std::endl;
    throw std::invalid_argument("Non-matching particle numbers in arguments.");
  } else if (data.getBoxSide() != prevStats.getBoxSide()) {
    throw std::invalid_argument("Non-matching box sides.");
  } else if (data.getTime() < prevStats.getTime()) {
    throw std::invalid_argument("Stats time is less than gas time.");
  } else if (!isNegligible(
				std::accumulate(
					data.getParticles().begin(),
          data.getParticles().end(), 0.,
          [](double x, const Particle& p) {
            return x + p.speed * p.speed;
          }) * Particle::mass / data.getParticles().size() / 3. - prevStats.getTemp(),
				prevStats.getTemp())) {
    throw std::invalid_argument("Non-matching temperatures for provided GasData.");
  } else {
		{
		TH1D* defH = new TH1D();
		if (speedsHTemplate.IsEqual(defH)) {
			std::cerr << "Found default histo, initializing with prevStats speedsH_.\n";
			speedsH_ = prevStats.speedsH_;
			speedsH_.SetDirectory(nullptr);
			speedsH_.Reset("ICES");
		} else {
			std::cerr << "Found non-default histo template, initializing with template.\n";
			if (speedsHTemplate.GetEntries() != 0.) {
				delete defH;
				throw std::runtime_error("Non-empty speedsH template provided.");
			} else {
				speedsH_ = speedsHTemplate;
			}
		}
		delete defH;
		}

    lastCollPositions_ = prevStats.lastCollPositions_;
    T_ = prevStats.T_;
    if (data.getCollType() == 'w') {
      addPulse(data);
    } else if (data.getCollType() == 'p') {
      lastCollPositions_[data.getP2Index()] = data.getP2().position;
    }
    lastCollPositions_[data.getP1Index()] = data.getP1().position;
		std::for_each(
				data.getParticles().begin(), data.getParticles().end(),
				[this] (const Particle& p) {
					//std::cerr << "Filling histo with value " << p.speed.norm() << std::endl;
					speedsH_.Fill(p.speed.norm());
					//std::cerr << "speedsH_ entries = " << speedsH_.GetEntries()
					//<< ", bin content for last added occourrence = "
					//<< speedsH_.GetBinContent(speedsH_.FindBin(p.speed.norm())) 
					//<< ", bin number = " << speedsH_.GetNbinsX() << std::endl;
				}
		);
  }
}

TdStats::TdStats(const TdStats& s)
	: wallPulses_(s.wallPulses_),
	lastCollPositions_(s.lastCollPositions_),
	T_(s.T_),
	freePaths_(s.freePaths_),
	speedsH_(s.speedsH_),
	t0_(s.t0_),
	time_(s.time_),
	boxSide_(s.boxSide_) {
		speedsH_.SetDirectory(nullptr);
}

TdStats::TdStats(TdStats&& s) noexcept
	: wallPulses_(std::move(s.wallPulses_)),
	lastCollPositions_(std::move(s.lastCollPositions_)),
	T_(s.T_),
	freePaths_(std::move(s.freePaths_)),
	speedsH_(std::move(s.speedsH_)),
	t0_(s.t0_),
	time_(s.time_),
	boxSide_(s.boxSide_) {
		speedsH_.SetDirectory(nullptr);
		s.speedsH_.SetDirectory(nullptr);
}

TdStats& TdStats::operator=(const TdStats& s) {
	wallPulses_ = s.wallPulses_;
	lastCollPositions_ = s.lastCollPositions_;
	T_ = s.T_;
	freePaths_ = s.freePaths_;
	speedsH_ = s.speedsH_;
	speedsH_.SetDirectory(nullptr);
	t0_ = s.t0_;
	time_ = s.time_;
	boxSide_ = s.boxSide_; 
	return *this;
}

TdStats& TdStats::operator=(TdStats&& s) noexcept {
	wallPulses_ = std::move(s.wallPulses_);
	lastCollPositions_ = std::move(s.lastCollPositions_);
	T_ = s.T_;
	freePaths_ = std::move(s.freePaths_);
	speedsH_ = std::move(s.speedsH_);
	speedsH_.SetDirectory(nullptr);
	s.speedsH_.SetDirectory(nullptr);
	t0_ = s.t0_;
	time_ = s.time_;
	boxSide_ = s.boxSide_; 
	return *this;
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
		// VERY inefficient, could be optimized? (only speeds that change are the collided particles')
		std::for_each(
				data.getParticles().begin(), data.getParticles().end(),
				[this] (const Particle& p) {
					//std::cerr << "Filling histo with value " << p.speed.norm() << std::endl;
					speedsH_.Fill(p.speed.norm());
					//std::cerr << "speedsH_ entries = " << speedsH_.GetEntries()
					//<< ", bin content for last added occourrence = "
					//<< speedsH_.GetBinContent(speedsH_.FindBin(p.speed.norm())) << std::endl;
				}
		);
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
		return -1.;
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

std::deque<GasData> SimOutput::getData() {
  std::lock_guard<std::mutex> rawDataGuard {rawDataMtx_};
	return rawData_;
}

void SimOutput::setFramerate(double framerate) {
  if (framerate <= 0) {
    throw std::invalid_argument("Non-positive framerate provided.");
  } else {
    gDeltaT_.store(1./framerate);
  }
}

SimOutput::SimOutput(size_t statSize, double framerate, const TH1D& speedsHTemplate)
    : statSize_(statSize), speedsHTemplate_(speedsHTemplate) {
  setFramerate(framerate);
	if (speedsHTemplate.GetEntries() != 0) {
		throw std::invalid_argument("Provided non-empty histogram template.");
	}
	assert(speedsHTemplate.GetNbinsX() != 0);
}

size_t SimOutput::getRawDataSize() {
	std::lock_guard<std::mutex> dataGuard (rawDataMtx_);
	return rawData_.size();
}

void SimOutput::addData(const std::vector<GasData>& data) {
	if (data.size()) {
		dataDone_.store(false);
		double prevDTime;
		bool firstD {true};
		for (const GasData& d: data) {
			if (!nParticles_.has_value()) {
				nParticles_ = d.getParticles().size();
			} else {
				if (d.getParticles().size() != nParticles_) {
					throw std::invalid_argument(
						"Non-matching particle numbers."
					);
				}
			}
			if (!firstD && !isNegligible(d.getT0() - prevDTime, d.getTime() - d.getT0())) {
				throw std::invalid_argument("Non sequential data vector provided.");
			}
			prevDTime = d.getTime();
			firstD = false;
		}
		{
		std::lock_guard<std::mutex> rawDataGuard {rawDataMtx_};
		if (rawDataBackTime_.has_value()) {
			if (!isNegligible(data.front().getT0() - rawDataBackTime_.value(), data.front().getTime() - data.front().getT0())) {
				throw std::invalid_argument(
					"Argument time less than latest raw data piece time"
				);
			}
		}
		rawData_.insert(rawData_.end(), data.begin(), data.end());
		rawDataBackTime_ = rawData_.back().getTime();
		}
		rawDataCv_.notify_all();
	}
}

void SimOutput::processData(bool mfpMemory) {
	processing_.store(true);
  std::vector<GasData> data {};
  std::unique_lock<std::mutex> rawDataLock(rawDataMtx_, std::defer_lock);
  while (true) {
		rawDataLock.lock();
    rawDataCv_.wait_for(rawDataLock, std::chrono::milliseconds(100),
  		[this] {
				return rawData_.size() > statSize_.load() || dataDone_.load();
			}
		);
		if (dataDone_ && rawData_.size() < statSize_.load()) {
      break;
    }
    size_t nStats{rawData_.size() / statSize_};
		{ // chunkSize nspc
		size_t chunkSize = statChunkSize_.load();
		if (chunkSize) {
			nStats = nStats > chunkSize? chunkSize: nStats;
		}
		} // chunkSize nspcEnd
		// don't stop make it drop dj blow my speakers up
    // std::cout << "nStats for this iteration is " << rawData_.size() << "/" << statSize_ << " = " << nStats << std::endl;

		if (nStats) {
    	data.insert(
				data.end(),
				std::make_move_iterator(rawData_.begin()),
				std::make_move_iterator(rawData_.begin() + nStats * statSize_)
			);
			assert(data.size());
    	// std::cout << "GasData count = " << data.size() << std::endl;
    	rawData_.erase(rawData_.begin(), rawData_.begin() + nStats * statSize_);
    	rawDataLock.unlock();

			//std::cout << "Data size: " << data.size() << "; ";
    	processStats(data, mfpMemory);
			/*{
			std::lock_guard<std::mutex> guard {statsMtx_};
			//std::cout << "Stats size: " << stats_.size() << std::endl;
			}*/
			resultsCv_.notify_all();
		} else {
			rawDataLock.unlock();
		}
		data.clear();
  }
	processing_.store(false);
}

void SimOutput::processData(const Camera& camera, const RenderStyle& style,
                            bool mfpMemory) {
	processing_.store(true);
  // std::cout << "Started processing data.\n";
  std::vector<GasData> data {};
	std::unique_lock<std::mutex> rawDataLock {rawDataMtx_, std::defer_lock};
  while (true) {
		rawDataLock.lock();
    rawDataCv_.wait_for(rawDataLock, std::chrono::milliseconds(100),
			[this] {
				return rawData_.size() > statSize_.load() || dataDone_;
			}
		);
    // std::cout << "Done status: " << done_ << ". RawData emptiness: " <<
    // rawData_.empty() << std::endl;
    if (dataDone_.load() && rawData_.size() < statSize_.load()) {
      break;
    }

    size_t nStats { rawData_.size() / statSize_ };
		size_t chunkSize { statChunkSize_.load() };
		if (chunkSize) {
			nStats = nStats > chunkSize? chunkSize: nStats;
		}

		if (nStats) {
			std::move(rawData_.begin(), rawData_.begin() + nStats * statSize_,
								std::back_inserter(data));
			rawData_.erase(rawData_.begin(), rawData_.begin() + nStats * statSize_);
			rawDataLock.unlock();

			std::thread sThread;
			std::thread gThread;
			
			{ // guard scope begin
			std::lock_guard<std::mutex> resultsGuard {resultsMtx_};

			sThread = std::thread([this, &data, mfpMemory]() {
				try {
					processStats(data, mfpMemory);
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

			if (sThread.joinable()) sThread.join();
			if (gThread.joinable()) gThread.join();
			} // guard scope end
			addedResults_.store(true);
			resultsCv_.notify_all();
			data.clear();
		} else {
			rawDataLock.unlock();
		}
	}
	processing_.store(false);
}
/*
void argbToSfImage(const UInt_t* argbBffr, sf::Image& img) {
	unsigned ww {img.getSize().x};
	UInt_t argb {};
	sf::Color pxlClr {};
	for (unsigned j {0}; j < img.getSize().y; ++j) {
		for (unsigned i {0}; i < img.getSize().x; ++i) {
			argb = argbBffr[j*ww + i];
			pxlClr = {
				static_cast<sf::Uint8>((argb >> 16) & 0xFF),
				static_cast<sf::Uint8>((argb >> 8) & 0xFF),
				static_cast<sf::Uint8>(argb & 0xFF),
				static_cast<sf::Uint8>((argb >> 24) & 0xFF)
			};
			img.setPixel(i, j, pxlClr);
		}
	}
}
*/
void argbToRgba(const UInt_t* argbBffr, unsigned w, unsigned h, std::vector<sf::Uint8>& pixels) {
    const size_t total = static_cast<size_t>(w) * h;
    pixels.resize(total * 4);

    std::vector<size_t> indices(total);
    std::iota(indices.begin(), indices.end(), 0);

    std::for_each(std::execution::par, indices.begin(), indices.end(),
                  [&](size_t idx) {
        UInt_t argb = argbBffr[idx];
        size_t base = idx * 4;
        pixels[base + 0] = static_cast<sf::Uint8>((argb >> 16) & 0xFF); // R
        pixels[base + 1] = static_cast<sf::Uint8>((argb >> 8) & 0xFF);  // G
        pixels[base + 2] = static_cast<sf::Uint8>(argb & 0xFF);         // B
        pixels[base + 3] = static_cast<sf::Uint8>((argb >> 24) & 0xFF); // A
    });
}

bool isNegligible(double epsilon, double x) {
	return fabs(epsilon/x) < 1E-6;
}
bool isIntMultOf(double x, double dt) {
	return isNegligible(x - dt*std::round(x/dt), dt);
}

std::vector<sf::Texture> SimOutput::getVideo(
		VideoOpts opt, sf::Vector2i windowSize,
		sf::Texture placeholder, TList& prevGraphs, bool emptyStats,
		std::function<void(TH1D&, VideoOpts)> fitLambda,
		std::array<std::function<void()>, 4> drawLambdas) {

	using clock = std::chrono::high_resolution_clock;
	using doubleTime = std::chrono::duration<double>;
	auto ph0Start = clock::now();

	static int nExec {0};
	std::cerr << std::endl << "Started getVideo, call number " << ++nExec << std::endl;

	if ((opt == VideoOpts::justGas && windowSize.x < 400 && windowSize.y < 400)
	|| 	(opt == VideoOpts::justStats && windowSize.x < 600 && windowSize.y < 600)
	||  (windowSize.x < 800 && windowSize.y < 600)) {
		throw std::invalid_argument("Passed window size is too small.");
	}

	if (!(prevGraphs.At(0)->IsA() == TMultiGraph::Class() &&
			prevGraphs.At(1)->IsA() == TGraph::Class() &&
			prevGraphs.At(2)->IsA() == TGraph::Class())) {
		throw std::invalid_argument("Passed graphs list with wrong object types.");
	}
	if (((TMultiGraph*) prevGraphs.At(0))->GetListOfGraphs()->GetSize() != 7) {
		throw std::invalid_argument("Number of graphs in pressure multigraph != 7.");
	}

	std::deque<std::pair<sf::Texture, double>> renders {};
	std::vector<TdStats> stats {};
	std::optional<double> gTime;
	double gDeltaT;


	auto getRendersT0 = [&] (const auto& couples) {
		if (couples.size()) {
			assert(gTime.has_value());
			return couples[0].second;
		} else if (gTime.has_value()) {
			return *gTime;
		} else {
			throw std::logic_error("Tried to get render time for output with no time set.");
		}
	};

	{
		doubleTime ph0t = clock::now() - ph0Start;
		std::cerr << "Phase 0 time: " << ph0t.count() << std::endl;
	}

	std::cerr << "Got to data extraction phase." << std::endl;
	auto ph1Start = clock::now();
	//	// extraction of renders and/or stats clusters compatible with processing algorithm
	// fTime_ initialization, either through gTime_ or through stats[0]
	switch (opt) {
		case VideoOpts::justGas:
			{ // lock scope
			std::lock_guard<std::mutex> rGuard {rendersMtx_};
			{ // locks scope 2
			std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
			gTime = gTime_;
			} // end of locks scope 2
			gDeltaT = gDeltaT_.load();
			if (!gTime.has_value()) {
				return {};
			}
			if (renders_.size()) {
				renders = std::move(renders_);
				renders_.clear();
			}
			} // end of lock scope
			if ((!fTime_.has_value() || !isIntMultOf(*gTime - *fTime_, gDeltaT))) {
				fTime_ = getRendersT0(renders) - gDeltaT;
			}
			break;
		case VideoOpts::justStats:
			{ // lock scope
			std::lock_guard<std::mutex> sGuard {statsMtx_};
			{ // locks scope 2
			std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
			gTime = gTime_;
			} // end of locks scope 2
			gDeltaT = gDeltaT_.load();
			if (stats_.size()) {
				// setting fTime_
				if (!fTime_.has_value()) {
					if (gTime.has_value()) {
						*fTime_ = *gTime + gDeltaT * std::floor((stats[0].getTime0() - *gTime)/gDeltaT);
					} else {
						*fTime_ = stats_.front().getTime0() - gDeltaT;
					}
				}
				if (stats_.back().getTime() >= *fTime_ + gDeltaT) {
					auto sStartI {
						std::lower_bound(stats_.begin(), stats_.end(), *fTime_ + gDeltaT,
						[](const TdStats& s, double value) { return value <= s.getTime0(); })
					};
					stats = std::vector<TdStats> {sStartI, stats_.end()};
					if (emptyStats) {
						stats_.clear();
					}
				}
			} else {
				return {};
			}
			} // end of lock scope
			break;
		case VideoOpts::all:
		case VideoOpts::gasPlusCoords:
			{ // locks scope
			std::unique_lock<std::mutex> resultsLock {resultsMtx_};
			resultsCv_.wait_for(resultsLock,
			std::chrono::milliseconds(10), 
			[this] () { 
				return addedResults_.load();
			});
			std::lock_guard<std::mutex> rGuard {rendersMtx_};
			std::lock_guard<std::mutex> sGuard {statsMtx_};
			{ // locks scope 2
			std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
			gTime = gTime_;
			} // end of locks scope 2
			gDeltaT = gDeltaT_.load();
			std::cerr <<
				"Initialized gTime with value " <<
				(gTime.has_value()? std::to_string(*gTime): "empty") <<
				", gDeltaT with value " << gDeltaT <<
				std::endl;

			if (!gTime.has_value()) {
				return {};
			}
			if (renders_.size() || stats_.size()) {
				std::cerr << "Found sizes: renders_ = " << renders_.size()
					<< ", stats_ = " << stats_.size() << std::endl;
				// dealing with fTime_
				if (!fTime_.has_value() || !isIntMultOf(*gTime - *fTime_, gDeltaT)) {
					std::cerr << "Setting fTime_. Because ";
					if (fTime_.has_value()) {
						std::cerr << "isIntMultOf returned " << isIntMultOf(*gTime - *fTime_, gDeltaT) <<
						" for gTime - fTime_ = " << *gTime - *fTime_ << ", gDeltaT = " << gDeltaT;
					} else {	
						std::cerr << "fTime_.has_value returned " << fTime_.has_value();
					}
					std::cerr << std::endl;
					if (!stats_.size()) {
						fTime_ = getRendersT0(renders_) - gDeltaT;
					} else if (!renders_.size()) {
						fTime_ = *gTime + gDeltaT*std::floor((stats_.front().getTime0() - *gTime)/gDeltaT);
						assert(isIntMultOf(*fTime_ - *gTime, gDeltaT));
					} else {
						if (getRendersT0(renders_) > stats_.front().getTime0()) {
							fTime_ = getRendersT0(renders_) + gDeltaT * std::floor(
								(stats_[0].getTime0() - getRendersT0(renders_))/gDeltaT
							);
							std::cerr << "Found getRendersT0 = " << getRendersT0(renders_)
								<< ", stats beginning = " << stats_[0].getTime0()
								<< "; set fTime_ to " << *fTime_ << std::endl;
						} else {
							fTime_ = getRendersT0(renders_) - gDeltaT;
							std::cerr << "Found getRendersT0 = " << getRendersT0(renders_)
							<< ", stats beginning = " << stats_[0].getTime0()
							<< "; set fTime_ to " << *fTime_ << std::endl;
						}
					}
					std::cerr << "Set fTime_. Now fTime_.has_value() returns " << fTime_.has_value() << " with value = " << fTime_.value() << std::endl;
				}
				
				std::cerr << "Starting vector segments extraction." << std::endl;
				// extracting algorithm-compatible vector segments
				if (stats_.size()) {
					auto sStartI {
						std::upper_bound(stats_.begin(), stats_.end(), *fTime_ + gDeltaT,
						[](double value, const TdStats& s) { 
							std::cerr << "Value = " << value;
							value <= s.getTime()?
							 	std::cerr << " <= s.getTime() = " << s.getTime():
								std::cerr << " > s.getTime() = " << s.getTime();
							std::cerr << std::endl;
							return value <= s.getTime();
						})

					};
					std::cerr << "There's nothing to fear but BIG SCARY MONSTERS AAAAAHHHHH!!!!!!!!\n";
					std::cerr << "Found sStartI = " << sStartI - stats_.begin() << std::endl;
					std::cerr << "fTime_ value = " << *fTime_ << " with stats_ front Time0 = " << stats_.front().getTime0() << " and Time = " << stats_.front().getTime() << std::endl;
					if (sStartI != stats_.end()) {
						auto gStartI {
							std::lower_bound(
								renders_.begin(), renders_.end(), *fTime_,
								[](const auto& render, double value) {
									return render.second < value;
								}
							)
						};
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						auto gEndI {
							std::upper_bound(
								renders_.begin(), renders_.end(), stats_.back().getTime(),
								[](double value, auto& render) {
									return value < render.second;
								}
							)
						};
						std::cerr << "Found gStartI = " << gStartI - renders_.begin()
							<< ", gEndI = " << gEndI - renders_.begin() << std::endl;
						stats = std::vector(sStartI, stats_.end());
						if (emptyStats) {
							stats_.clear();
						}
						assert(gStartI <= gEndI); // fTime_ should be always <= stats.back().getTime, if it is available
						// used to have a check for gStartI != renders_.end(), if fuckup check here first
						renders.insert(
							renders.begin(),
							std::make_move_iterator(gStartI),
							std::make_move_iterator(gEndI)
						);
						renders_.erase(gStartI, gEndI);
					}
					std::cerr << "Extracted vectors. Result sizes: stats size = " << stats.size() << ", renders size = " << renders.size() << std::endl;
					if (!stats.size() && !renders.size()) {
						return {};
					}
				} else {
					auto gStartI {
						std::lower_bound(
							renders_.begin(), renders_.end(), *fTime_,
							[](const auto& render, double value) {
								return render.second < value;
							}
						)
					};
					// same as above
					renders.insert(
						renders.begin(),
						std::make_move_iterator(gStartI),
						std::make_move_iterator(renders_.end())
					);
					renders_.erase(gStartI, renders_.end());
				}
			} else {
				return {};
			}
			addedResults_ = false;
			resultsLock.unlock();
			} // end of locks scope
			break;
		default:
			throw std::invalid_argument("Invalid video option provided.");
	}
	{
		doubleTime ph1t = clock::now() - ph1Start;
		std::cerr << "Phase 1 time: " << ph1t.count() << std::endl;
	}

	std::cerr << "Got to second phase." << std::endl;

	auto ph2Start = clock::now();

	// std::cout << "Started variables setup." << std::endl;
	// recurrent variables setup
	// declarations
	// text
	sf::Font font;
	font.loadFromFile("assets/JetBrains-Mono-Nerd-Font-Complete.ttf");
	std::ostringstream Temp {};
	sf::Text TText;
	std::ostringstream Vol {};
	sf::Text VText;
	std::ostringstream Num {};
	sf::Text NText;
	sf::RectangleShape txtBox;
	// graphs
	// double t; // I don't think this is necessary, replaced by fTime_
	TMultiGraph& pGraphs {*(TMultiGraph*)prevGraphs.At(0)};
	TGraph& kBGraph {*(TGraph*)prevGraphs.At(1)};
	TGraph& mfpGraph {*(TGraph*)prevGraphs.At(2)};
	TCanvas cnvs {};
	// image stuff
	TImage* trnsfrImg;
	//sf::Image auxImg;
	std::vector<sf::Uint8> pixels;
	sf::Texture auxTxtr;
	sf::Sprite auxSprt;
	sf::RenderTexture frame;
	sf::RectangleShape box;
	std::vector<sf::Texture> frames {};
	auto drawObj = [&] (
			auto& TDrawable,
			double percPosX, double percPosY,
			const char* drawOpts = "",
			std::function<void()> drawLambda = {}) {
		cnvs.Clear();
		TDrawable.Draw(drawOpts);
		if (drawLambda) {
			drawLambda();
		}
		cnvs.Update();
		trnsfrImg->FromPad(&cnvs);
		argbToRgba(
			trnsfrImg->GetArgbArray(),
			trnsfrImg->GetWidth(), trnsfrImg->GetHeight(),
			pixels);
		auxTxtr.update(pixels.data());
		//auxTxtr.update(auxImg);
		auxSprt.setTexture(auxTxtr);
		auxSprt.setPosition(windowSize.x * percPosX, windowSize.y * percPosY);
		frame.draw(auxSprt);
	};

	TText.setCharacterSize(windowSize.y * 0.05);
	VText.setCharacterSize(windowSize.y * 0.05);
	NText.setCharacterSize(windowSize.y * 0.05);
	// definitions as per available data
	switch (opt) {
		case VideoOpts::gasPlusCoords:
			TText.setCharacterSize(windowSize.y * 0.5/9.);
			VText.setCharacterSize(windowSize.y * 0.5/9.);
			NText.setCharacterSize(windowSize.y * 0.5/9.);
		case VideoOpts::all:
		case VideoOpts::justStats:
			if (stats.size()) {
				Temp << std::fixed << std::setprecision(2) << stats[0].getTemp();
				TText = {"T = " + Temp.str() + "K", font, 18};
				TText.setFont(font);
				TText.setFillColor(sf::Color::Black);
				Vol << std::fixed << std::setprecision(2) << stats[0].getVolume();
				VText = {"V = " + Vol.str() + "m\u00B3", font, 18};
				VText.setFont(font);
				VText.setFillColor(sf::Color::Black);
				Num << std::fixed << std::setprecision(2) << stats[0].getNParticles();
				NText = {"N = " + Num.str(), font, 18};
				NText.setFont(font);
				NText.setFillColor(sf::Color::Black);			
				trnsfrImg = TImage::Create();
			}
			break;
		case VideoOpts::justGas:
			break;
		default:
			throw std::invalid_argument("Invalid video option provided.");
	}

	doubleTime ph2t = clock::now() - ph2Start;
	std::cerr << "Phase 2 time: " << ph2t.count() << std::endl;

	// processing
	auto ph3Start = clock::now();
	doubleTime graphsAddTime {},
	drawObjTime {}, txtDrawTime {}, drawGasTime {}, emplaceTime {},
	setTextureTime {}, boxDrawTime {}, displayTime {}, reserveTime{}, eraseTime {};
	frame.create(windowSize.x, windowSize.y);
	switch (opt) {
		case VideoOpts::justGas:
			trnsfrImg->Delete();
			assert(isIntMultOf(*fTime_ - *gTime, gDeltaT));
			assert(gTime == renders.back().second);
			assert(fTime_ <= getRendersT0(renders));
			box.setSize(static_cast<sf::Vector2f>(windowSize));
			box.setPosition(0, windowSize.y);
			while (*fTime_ + gDeltaT <= gTime) {
				*fTime_ += gDeltaT;
				frame.clear();
				if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
					box.setTexture(&(renders.front().first));
					frame.draw(box);
					frame.display();
					renders.erase(renders.begin());
				} else {
					box.setTexture(&placeholder);
					frame.draw(box);
					frame.display();
				}
				frames.emplace_back(frame.getTexture());
			}
			break;
		case VideoOpts::justStats:
			if (stats.size()) {
				if (*fTime_ + gDeltaT < stats.front().getTime0()) { // insert empty data and use placeholder render
					auto& stat {stats.front()};
					for(int i {0}; i < 7; ++i) {
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						graph->AddPoint(*fTime_, 0.);	
						graph->AddPoint(stat.getTime0(), 0.);	
					}

					mfpGraph.AddPoint(*fTime_, 0.);
					mfpGraph.AddPoint(stat.getTime0(), 0.);
					kBGraph.AddPoint(*fTime_, 0.);
					kBGraph.AddPoint(stat.getTime0(), 0.);

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);;
					//auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., windowSize.y * 0.55, "APL", drawLambdas[1]);

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					//auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

					box.setSize(sf::Vector2f(windowSize.x * 0.5, windowSize.y * 0.5));
					box.setPosition(windowSize.x * 0.5, windowSize.y);
					box.setTexture(&placeholder);
					frame.draw(box);

					frame.display();

					while (*fTime_ + gDeltaT < stat.getTime0()) {
						*fTime_ += gDeltaT;
						frames.emplace_back(frame.getTexture());
					}
				}

				for (const TdStats& stat: stats) {
					for(int i {0}; i < 7; ++i) {
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						if (i) {
							graph->AddPoint(stat.getTime(), stat.getPressure());
						} else {
							graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i - 1)));	
						}
					}

					mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
					kBGraph.AddPoint(
						stat.getTime(),
						stat.getPressure()*stat.getVolume()/(stat.getNParticles()*stat.getTemp())
					);
	
					TH1D speedH {stat.getSpeedH()};

					if (fitLambda) {
						fitLambda(speedH, opt);
					}
					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);;
					//auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., windowSize.y * 6./10., "APL", drawLambdas[1]);

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					//auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(speedH, 0.5, 0., "HIST", drawLambdas[2]);
					drawObj(mfpGraph, 0.5, 0.5, "APL", drawLambdas[3]);

					frame.display(); // whuh? is it to refresh something?

					while (*fTime_ + gDeltaT < stat.getTime()) { 
						*fTime_ += gDeltaT;
						frames.emplace_back(frame.getTexture());
					}
				}
				trnsfrImg->Delete();
				return frames;
			} else {
				return {};
			}
			break;
		case VideoOpts::gasPlusCoords:
			assert(gTime == renders.back().second);
			assert(fTime_ <= getRendersT0(renders));
			{ // case scope
			sf::Vector2f gasSize {static_cast<float>(windowSize.x * 0.5), static_cast<float>(windowSize.y * 8./9.)};
			if (stats.size()) {
				if (*fTime_ + gDeltaT < stats.front().getTime0()) {
					for(int i {0}; i < 7; ++i) { // uhm... is this index right? hopefully
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						graph->AddPoint(*fTime_, 0.);
						graph->AddPoint(stats.front().getTime0(), 0.);	
					}

					mfpGraph.AddPoint(*fTime_, 0.);
					mfpGraph.AddPoint(stats.front().getTime0(), 0.);
					kBGraph.AddPoint(*fTime_, 0.);
					kBGraph.AddPoint(stats.front().getTime0(), 0.);

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					// !!! likely drawing in wrong position everywhere, assumed bottom left corner instead of top left !!!
					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

					txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f/9.f});
					txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8./9.);
					VText.setPosition(windowSize.x * (0.5 + 0.15), windowSize.y * 8./9. + 10.);
					NText.setPosition(windowSize.x * 0.5 + 35., windowSize.y * 8./9. + 10.);
					TText.setPosition(windowSize.x * 0.5 + 60., windowSize.y * 8./9. + 10.);
					frame.draw(txtBox);
					frame.draw(VText);
					frame.draw(NText);
					frame.draw(TText);
					// insert paired renders or placeholders
					box.setSize(gasSize);
					box.setPosition(windowSize.x * 0.5, 0.); // huh?
					while (*fTime_ + gDeltaT < stats.front().getTime0()) {
						*fTime_ += gDeltaT;
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
							box.setTexture(&(renders.front().first));
							frame.draw(box);
							frame.display();
							renders.erase(renders.begin());
						} else {
							box.setTexture(&placeholder);
							frame.draw(box);
							frame.display();
						}
						frames.emplace_back(frame.getTexture());
					}
				}
				assert(*fTime_ + gDeltaT >= stats.front().getTime0());
				while (*fTime_ + gDeltaT < stats.back().getTime()) {
					auto s {
						std::lower_bound(
							stats.begin(), stats.end(), *fTime_ + gDeltaT,
							[](const TdStats& stat, double value) {
								return value <= stat.getTime0();
							}
						)
					};
					assert(s != stats.end());
					const TdStats& stat {*s};
					// make the graphs picture
					for(int i {0}; i < 7; ++i) {
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						if (!i) {
							graph->AddPoint(stat.getTime(), stat.getPressure());
						} else {
							graph->AddPoint(stat.getTime(), stat.getPressure(Wall(i - 1)));	
						}
					}
					kBGraph.AddPoint(
						stat.getTime(),
						stat.getPressure()*stat.getVolume()/(stat.getNParticles()*stat.getTemp())
					);

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

					txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f/9.f});
					txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8./9.);
					VText.setPosition(windowSize.x * 0.5 + 10., windowSize.y * 8./9. + 10.);
					NText.setPosition(windowSize.x * 0.5 + 35., windowSize.y * 8./9. + 10.);
					TText.setPosition(windowSize.x * 0.5 + 60., windowSize.y * 8./9. + 10.);
					frame.draw(txtBox);
					frame.draw(VText);
					frame.draw(NText);
					frame.draw(TText);				
					// insert paired renders or placeholders
					box.setSize(gasSize);
					box.setPosition(windowSize.x * 0.5, 0.); // huh?
					while (*fTime_ + gDeltaT < stat.getTime()) {
						*fTime_ += gDeltaT;
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
							box.setTexture(&(renders.front().first));
							frame.draw(box);
							frame.display();
							renders.erase(renders.begin());
						} else {
							box.setTexture(&placeholder);
							frame.draw(box);
							frame.display();
						}
						frames.emplace_back(frame.getTexture());
					}
				} // while (fTime_ + gDeltaT_ < stats.back().getTime())
			}	else if (renders.size()) {
				assert(renders.back().second == gTime);
				if (*fTime_ + gDeltaT < gTime || isNegligible(*fTime_ + gDeltaT - *gTime, gDeltaT)) {
					for(int i {0}; i < 7; ++i) { // uhm... is this index right? hopefully
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						graph->AddPoint(*fTime_, 0.);	
						graph->AddPoint(*gTime, 0.);	
					}

					mfpGraph.AddPoint(*fTime_, 0.);
					mfpGraph.AddPoint(*gTime, 0.);
					kBGraph.AddPoint(
						*fTime_,
						0.
					);
					kBGraph.AddPoint(
						*gTime,
						0.
					);

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);

					txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 1.f/9.f});
					txtBox.setPosition(windowSize.x * 0.5, windowSize.y * 8./9.);
					VText.setPosition(windowSize.x * 0.5 + 10., windowSize.y * 8./9. + 10.);
					NText.setPosition(windowSize.x * 0.5 + 35., windowSize.y * 8./9. + 10.);
					TText.setPosition(windowSize.x * 0.5 + 60., windowSize.y * 8./9. + 10.);
					frame.draw(txtBox);
					frame.draw(VText);
					frame.draw(NText);
					frame.draw(TText);
					box.setSize(gasSize);
					box.setPosition(windowSize.x * 0.5, 0.); // huh, again, seems like I was thinking bottom left corner
					while (*fTime_ + gDeltaT < gTime || isNegligible(*fTime_ + gDeltaT - *gTime, gDeltaT)) {
						*fTime_ += gDeltaT;
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						if (renders.size() && isNegligible(*fTime_ + gDeltaT - renders[0].second, gDeltaT)) {
							box.setTexture(&(renders.front().first));
							frame.draw(box);
							frame.display();
							renders.erase(renders.begin());
						} else {
							box.setTexture(&placeholder);
							frame.draw(box);
							frame.display();
						}
						frames.emplace_back(frame.getTexture());
					}
				}
			} // else if renders.size()
			} // case scope end
			break;
		case VideoOpts::all:
			assert(gTime == renders.back().second);
			assert(fTime_ <= getRendersT0(renders));
			{ // case scope
			sf::Vector2f gasSize {static_cast<float>(windowSize.x * 0.5), static_cast<float>(windowSize.y * 9./10.)};
			if (stats.size()) {
				auxTxtr.create(windowSize.x * 0.25, windowSize.y * 0.5);
				if (*fTime_ + gDeltaT_ < stats.front().getTime0()) {
					for(int i {0}; i < 7; ++i) { // uhm... is this index right? hopefully
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
						graph->AddPoint(*fTime_, 0.);
						graph->AddPoint(stats.front().getTime0(), 0.);
					}

					mfpGraph.AddPoint(*fTime_, 0.);
					mfpGraph.AddPoint(stats.front().getTime0(), 0.);
					kBGraph.AddPoint(
						*fTime_,
						0.
					);
					kBGraph.AddPoint(
						stats.front().getTime0(),
						0.
					);
					TH1D speedH {};

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
					drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
					drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

					txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
					txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
					VText.setPosition(windowSize.x * 0.25 + 10., windowSize.y * 0.9 + 10.);
					NText.setPosition(windowSize.x * 0.25 + 35., windowSize.y * 0.9 + 10.);
					TText.setPosition(windowSize.x * 0.25 + 60., windowSize.y * 0.9 + 10.);
					frame.draw(txtBox);
					frame.draw(VText);
					frame.draw(NText);
					frame.draw(TText);

					// insert paired renders or placeholders
					box.setSize(gasSize);
					box.setPosition(windowSize.x * 0.25, 0.);
					while (*fTime_ + gDeltaT_ < stats.front().getTime0()) { // same, need to vary implementation for fTime_
						*fTime_ += gDeltaT;
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
							box.setTexture(&(renders.front().first));
							frame.draw(box);
							frame.display();
							renders.erase(renders.begin());
						} else {
							box.setTexture(&placeholder);
							frame.draw(box);
							frame.display();
						}
						frames.emplace_back(frame.getTexture());
					}
				}
				while (*fTime_ + gDeltaT_ < stats.back().getTime()) { // likely skips the first frame here
					assert(*fTime_ + gDeltaT_ >= stats.front().getTime0());
					auto s {
						std::lower_bound(
							stats.begin(), stats.end(), *fTime_,
							[](const TdStats& stat, double value) {
								return stat.getTime0() <= value;
							}
						)
					};
					// std::cerr << "Found stat index = " << s - stats.begin() << " for fTime_ = " << *fTime_ + gDeltaT << std::endl;
					assert(s != stats.end()); 
					const TdStats& stat {*s};
					// make the graphs picture
					auto rootStart = clock::now();
					for(int k {0}; k < 7; ++k) {
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(k)};
						if (k < 6) {
							graph->AddPoint(stat.getTime(), stat.getPressure(Wall(k)));	
						} else {
							graph->AddPoint(stat.getTime(), stat.getPressure());
						}
					}
					mfpGraph.AddPoint(stat.getTime(), stat.getMeanFreePath());
					kBGraph.AddPoint(
						stat.getTime(),
						stat.getPressure()*stat.getVolume()/(stat.getNParticles()*stat.getTemp())
					);
					std::cerr << "Added kB measurement to graph from starting data: \n" 
					<< "Pressure = " << stat.getPressure()
					<< "\nVolume = " << stat.getVolume()
					<< "\nTemperature = " << stat.getTemp()
					<< "\nNParticles = " << stat.getNParticles() << std::endl;
					TH1D speedH;
					speedH = stat.getSpeedH();

					if (fitLambda) {
						fitLambda(speedH, opt);
					}

					graphsAddTime += (doubleTime) (clock::now() - rootStart);

					frame.clear();

					auto drawObjStart = clock::now();
					cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
					//auxImg.create(windowSize.x * 0.25, windowSize.y * 0.5);
					drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
					drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
					drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
					drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);
					drawObjTime += (doubleTime) (clock::now() - drawObjStart);

					auto txtDrawStart = clock::now();
					txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
					txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
					VText.setPosition(windowSize.x * (0.25 + 0.025), windowSize.y * (0.9 + 0.025));
					NText.setPosition(windowSize.x * (0.25 + 0.175), windowSize.y * (0.9 + 0.025));
					TText.setPosition(windowSize.x * (0.25 + 0.325), windowSize.y * (0.9 + 0.025));
					frame.draw(txtBox);
					frame.draw(VText);
					frame.draw(NText);
					frame.draw(TText);
					txtDrawTime += (doubleTime) (clock::now() - txtDrawStart);

					// insert paired renders or placeholders
					box.setSize(gasSize);
					box.setPosition(windowSize.x * 0.25, 0.);
					auto reserveStart = clock::now();
					frames.reserve(renders.size());
					reserveTime += (doubleTime) (clock::now() - reserveStart);
					while (*fTime_ + gDeltaT_ < stat.getTime()) {
						*fTime_ += gDeltaT;
						assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
						if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
							auto drawGasStart = clock::now();
							box.setTexture(&(renders.front().first));
							setTextureTime += (doubleTime) (clock::now() - drawGasStart);
							auto boxDrawStart = clock::now();
							frame.draw(box);
							boxDrawTime += (doubleTime) (clock::now() - boxDrawStart);
							auto displayStart = clock::now();
							frame.display();
							displayTime += (doubleTime) (clock::now() - displayStart);
							auto eraseStart = clock::now();
							renders.pop_front();
							eraseTime += (doubleTime) (clock::now() - eraseStart);
							drawGasTime += (doubleTime) (clock::now() - drawGasStart);
						} else {
							box.setTexture(&placeholder);
							frame.draw(box);
							frame.display();
						}
						auto emplaceStart = clock::now();
						frames.emplace_back(frame.getTexture());
						emplaceTime += (doubleTime) (clock::now() - emplaceStart);
					}
				} // while (fTime_ + gDeltaT_ < stats.back().getTime())
			} else if (renders.size()) {
				for(int i {0}; i < 7; ++i) {
					TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(i)};
					graph->AddPoint(*fTime_, 0.);
					graph->AddPoint(*gTime, 0.);
				}

				mfpGraph.AddPoint(*fTime_, 0.);
				mfpGraph.AddPoint(*gTime, 0.);
				kBGraph.AddPoint(
					*fTime_,
					0.
				);
				kBGraph.AddPoint(
					*gTime,
					0.
				);
				TH1D speedH {};

				frame.clear();

				cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
				//auxImg.create(windowSize.x * 0.25, windowSize.y * 0.5);
				drawObj(pGraphs, 0., 0., "APL", drawLambdas[0]);
				drawObj(kBGraph, 0., 0.5, "APL", drawLambdas[1]);
				drawObj(speedH, 0.75, 0., "HIST", drawLambdas[2]);
				drawObj(mfpGraph, 0.75, 0.5, "APL", drawLambdas[3]);

				txtBox.setSize({windowSize.x * 0.5f, windowSize.y * 0.1f});
				txtBox.setPosition(windowSize.x * 0.25, windowSize.y * 0.9);
				VText.setPosition(windowSize.x * 0.25 + 10., windowSize.y * 0.9 + 10.);
				NText.setPosition(windowSize.x * 0.25 + 35., windowSize.y * 0.9 + 10.);
				TText.setPosition(windowSize.x * 0.25 + 60., windowSize.y * 0.9 + 10.);
				frame.draw(txtBox);
				frame.draw(VText);
				frame.draw(NText);
				frame.draw(TText);
				box.setSize(gasSize);
				box.setPosition(windowSize.x * 0.25, 0.);
				while (*fTime_ + gDeltaT <= gTime) {
					*fTime_ += gDeltaT;
					assert(isIntMultOf(*gTime - *fTime_, gDeltaT));
					if (renders.size() && isNegligible(*fTime_ - renders[0].second, gDeltaT)) {
						box.setTexture(&(renders.front().first));
						frame.draw(box);
						frame.display();
						renders.erase(renders.begin());
					} else {
						box.setTexture(&placeholder);
						frame.draw(box);
						frame.display();
					}
					frames.emplace_back(frame.getTexture());
				}
			} // else if renders.size()
			} // case scope end
			trnsfrImg->Delete();
			{
				doubleTime ph3t = clock::now() - ph3Start;
				std::cerr << "Phase 3 time: " << ph3t.count() << " of which:\n"
				<< "graphsAddTime = " << graphsAddTime.count() << '\n'
				<< "drawObjTime = " << drawObjTime.count() << '\n'
				<< "drawGasTime = " << drawGasTime.count() << " of which:\n"
				<< "~\tsetTexture time = " << setTextureTime.count() << '\n'
				<< "~\tboxDraw time = " << boxDrawTime.count() << '\n'
				<< "~\tdisplay time = " << displayTime.count() << '\n'
				<< "~\terase time = " << eraseTime.count() << '\n'
				<< "~\treserve time = " << reserveTime.count() << '\n'
				<< "emplaceTime = " << emplaceTime.count()
				<< std::endl;
			}
			return frames;
			break;
		default:
			throw std::invalid_argument("Invalid video option provided.");
	}

	return frames;

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
  std::vector<std::pair<sf::Texture, double>> renders{};

	double gTime;
	double gDeltaT {gDeltaT_.load()};
	{ // guard scope
	std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
  if (!gTime_.has_value()) {
		gTime_ = data[0].getT0() - gDeltaT;
	}
  assert(gDeltaT > 0.);
	if (data.size()) {
  	assert(gTime_ <= data[0].getT0());
	}
	gTime = *gTime_;
	} // guard scope
	sf::RenderTexture picture;

  renders.reserve((data.back().getTime() - data.front().getTime()) / gDeltaT +
                  1);
	
	while (gTime + gDeltaT < data[0].getT0()) {
		gTime += gDeltaT;
	}

  for (const GasData& dat : data) {
    while (gTime + gDeltaT <= dat.getTime()) {
      // std::cout << "Drawing gas at time " << *gTime_ + gDeltaT_ << std::endl;
      gTime += gDeltaT;
      drawGas(dat, camera, picture, style, gTime - dat.getTime());
      renders.emplace_back(picture.getTexture(), gTime);
    }
  }

	std::lock_guard<std::mutex> rendersGuard {rendersMtx_};
  std::move(renders.begin(), renders.end(), std::back_inserter(renders_));
	std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
	gTime_ = gTime;

  // std::cout << "done!\n";
}

void SimOutput::processStats(const std::vector<GasData>& data, bool mfpMemory) {
  // std::cout << "Started processing stats.\n";
  std::vector<TdStats> stats{};

	size_t statSize = statSize_.load();
	assert(!(data.size() % statSize));

  stats.reserve(data.size() / statSize);

  // std::cout << "Reached pre-loop\n";
	if (mfpMemory) {
		std::lock_guard<std::mutex> lastStatGuard {lastStatMtx_};
		for (unsigned i{0}; i < data.size() / statSize; ++i) {
			TdStats stat {
				lastStat_.has_value()?
				TdStats {data[i * statSize], *lastStat_}:
				TdStats {data[i * statSize], speedsHTemplate_}
			};
			//std::cerr << "Bin number for lastStat_ speedsH_ = " << lastStat_->getSpeedH().GetNbinsX() << std::endl;
			//std::cerr << "Bin number for speedsHTemplate_ = " << speedsHTemplate_.GetNbinsX() << std::endl;
			for (unsigned j{1}; j < statSize; ++j) {
				stat.addData(data[i * statSize + j]);
			}
			stats.emplace_back(stat);
			lastStat_ = std::move(stat);
		}
	} else {
		for (size_t i{0}; i < data.size() / statSize; ++i) {
			TdStats stat{data[i * statSize], speedsHTemplate_};
			for (size_t j{1}; j < statSize; ++j) {
				stat.addData(data[i * statSize + j]);
			}
			stats.emplace_back(stat);
		}
	}
  // std::cout << "Done with loop\n";
	std::lock_guard<std::mutex> statsGuard {statsMtx_};
	stats_.insert(
		stats_.end(),
		std::make_move_iterator(stats.begin()),
		std::make_move_iterator(stats.end())
	);
}

std::vector<TdStats> SimOutput::getStats(bool emptyQueue) {
  if (emptyQueue) {
		// wtf was I thinking when I wrote this... to be written better in rewrite
    std::deque<TdStats> stats;
		std::lock_guard<std::mutex> lastStatGuard {lastStatMtx_};
		{
		std::lock_guard<std::mutex> statsGuard {statsMtx_};
		lastStat_ = stats.back();
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
  std::vector<sf::Texture> renders {};
  if (emptyQueue) {
		std::lock_guard<std::mutex> rendersGuard {rendersMtx_};
		renders.reserve(renders_.size());
		for (auto& r: renders_) {
			renders.emplace_back(std::move(r.first));
		}
    renders_.clear();
  } else {
    std::lock_guard<std::mutex> guard(rendersMtx_);
		renders.reserve(renders_.size());
		for (auto& r: renders_) {
			renders.emplace_back(r.first);
		}
  }
  return renders;
}

void SimOutput::setStatChunkSize(size_t s) {
	if (s) {
		statChunkSize_.store(s);
	} else {
		throw(std::invalid_argument("Provided null stat chunk size."));
	}
}

void SimOutput::setStatSize(size_t s) {
	if (s) {
		statSize_.store(s);
	} else {
		throw(std::invalid_argument("Provided null stat size."));
	}
}

}  // namespace gasSim

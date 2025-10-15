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

TdStats::TdStats(const GasData& data, const TdStats& prevStats, const TH1D& speedsHTemplate)
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
		{
		TH1D* defH = new TH1D();
		if (speedsHTemplate.IsEqual(defH)) {
			speedsH_ = TH1D();
			speedsH_.SetBins(
				prevStats.speedsH_.GetNbinsX(),
				prevStats.speedsH_.GetXaxis()->GetXmin(),
				prevStats.speedsH_.GetXaxis()->GetXmax()
			);
			speedsH_.SetNameTitle(
				prevStats.speedsH_.GetName(),
				prevStats.speedsH_.GetTitle()
			);
		} else {
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
					speedsH_.Fill(p.speed.norm());
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
		std::lock_guard<std::mutex> gDeltaTGuard {gDeltaTMtx_};
    gDeltaT_ = 1. / framerate;
  }
}

SimOutput::SimOutput(unsigned int statSize, double framerate, const TH1D& speedsHTemplate)
    : statSize_(statSize), speedsHTemplate_(speedsHTemplate) {
  setFramerate(framerate);
	if (speedsHTemplate.GetEntries() != 0) {
		throw std::invalid_argument("Provided non-empty histogram template.");
	}
}

bool SimOutput::dataEmpty() {
	std::lock_guard<std::mutex> dataGuard (rawDataMtx_);
	return rawData_.empty();
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

bool isNegligible(double epsilon, double x) {
	return fabs(epsilon/x) < 1E-6;
}
bool isIntMultOf(double x, double dt) {
	return isNegligible(x - dt*std::round(x/dt), dt);
}

std::vector<sf::Texture> SimOutput::getVideo(VideoOpts opt, sf::Vector2i windowSize, sf::Texture placeholder, TList& prevGraphs, bool emptyStats) {

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

	std::vector<std::pair<sf::Texture, double>> renders {};
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

	// extraction of renders and/or stats clusters compatible with processing algorithm
	// fTime_ initialization, either through gTime_ or through stats[0]
	switch (opt) {
		case VideoOpts::justGas:
			{ // lock scope
			std::lock_guard<std::mutex> rGuard {rendersMtx_};
			{ // locks scope 2
			std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
			std::lock_guard<std::mutex> gDeltaTGuard {gDeltaTMtx_};
			gTime = gTime_;
			gDeltaT = gDeltaT_;
			} // end of locks scope 2
			if (!gTime.has_value()) {
				return {};
			}
			if (renders_.size()) {
				renders.reserve(renders_.size());
				std::move(
					std::make_move_iterator(renders_.begin()),
					std::make_move_iterator(renders_.end()),
					std::back_inserter(renders)
				);
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
			std::lock_guard<std::mutex> gDeltaTGuard {gDeltaTMtx_};
			gTime = gTime_;
			gDeltaT = gDeltaT_;
			} // end of locks scope 2
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
			std::lock_guard<std::mutex> rGuard {rendersMtx_};
			std::lock_guard<std::mutex> sGuard {statsMtx_};
			{ // locks scope 2
			std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
			std::lock_guard<std::mutex> gDeltaTGuard {gDeltaTMtx_};
			gTime = gTime_;
			gDeltaT = gDeltaT_;
			} // end of locks scope 2
			if (!gTime.has_value()) {
				return {};
			}
			if (renders_.size() || stats_.size()) {
				// dealing with fTime_
				if (!fTime_.has_value() || !isIntMultOf(*gTime - *fTime_, gDeltaT)) {
					if (!stats_.size()) {
						*fTime_ = getRendersT0(renders_) - gDeltaT;
					} else if (!renders_.size()) {
						*fTime_ = *gTime + gDeltaT*std::floor((stats.front().getTime0() - *gTime)/gDeltaT);
						assert(isIntMultOf(*fTime_ - *gTime, gDeltaT));
					} else {
						if (getRendersT0(renders_) >= stats_.front().getTime0()) {
							*fTime_ = getRendersT0(renders_) + gDeltaT * std::floor(
								(stats[0].getTime0() - getRendersT0(renders_))/gDeltaT
							);
						} else {
							*fTime_ = getRendersT0(renders_);
						}
					}
				}
				
				// extracting algorithm-compatible vector segments
				if (stats_.size()) {
					auto sStartI {
						std::lower_bound(stats_.begin(), stats_.end(), *fTime_ + gDeltaT,
						[](const TdStats& s, double value) { return value <= s.getTime0(); })
					};
					if (sStartI != stats_.end()) {
						auto gStartI {
							std::lower_bound(
								renders_.begin(), renders_.end(), *fTime_,
								[](const auto& render, double value) {
									return render.second < value;
								}
							)
						};
						assert(!std::fmod((*gTime - *fTime_), gDeltaT));
						auto gEndI {
							std::upper_bound(
								renders_.begin(), renders_.end(), stats_.back().getTime(),
								[](double value, auto& render) {
									return render.second < value;
								}
							)
						};
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
			} // end of locks scope
			break;
		default:
			throw std::invalid_argument("Invalid video option provided.");
	}

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
	sf::Image auxImg;
	sf::Texture auxTxtr;
	sf::Sprite auxSprt;
	sf::RenderTexture frame;
	sf::RectangleShape box;
	std::vector<sf::Texture> frames {};
	auto drawObj = [&] (auto& TDrawable, double percPosX, double percPosY) {
		// cnvs.Clear();?
		TDrawable.Draw();
		cnvs.Update();
		trnsfrImg->FromPad(&cnvs);
		argbToSfImage(trnsfrImg->GetArgbArray(), auxImg);
		auxTxtr.loadFromImage(auxImg);
		auxSprt.setTexture(auxTxtr);
		auxSprt.setPosition(windowSize.x * percPosX, windowSize.y * percPosY);
		frame.draw(auxSprt);
	};

	// definitions as per available data
	switch (opt) {
		case VideoOpts::gasPlusCoords:
		case VideoOpts::all:
		case VideoOpts::justStats:
			if (stats.size()) {
				Temp << std::fixed << std::setprecision(2) << stats[0].getTemp();
				TText = {"T = " + Temp.str() + "K", font, 18};
				TText.setPosition(10.f, 10.f);
				Vol << std::fixed << std::setprecision(2) << stats[0].getVolume();
				VText = {"V = " + Vol.str() + "m\u00B3", font, 18};
				Num << std::fixed << std::setprecision(2) << stats[0].getNParticles();
				NText = {"N = " + Num.str(), font, 18};
				trnsfrImg = TImage::Create();
			}
			break;
		case VideoOpts::justGas:
			break;
		default:
			throw std::invalid_argument("Invalid video option provided.");
	}

	// processing
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
					auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., windowSize.y * 0.55);

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(mfpGraph, 0.5, 0.5);

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
	
					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.45);;
					auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., windowSize.y * 6./10.);

					cnvs.SetCanvasSize(windowSize.x * 0.5, windowSize.y * 0.5);
					auxImg.create(cnvs.GetWindowWidth(), cnvs.GetWindowHeight());

					drawObj(speedH, 0.5, 0.);
					drawObj(mfpGraph, 0.5, 0.5);

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
					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., 0.5);

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
					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., 0.5);

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
					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., 0.5);

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
					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., 0.5);
					drawObj(speedH, 0.75, 0.);
					drawObj(mfpGraph, 0.75, 0.5);

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
					assert(s != stats.end());
					const TdStats& stat {*s};
					// make the graphs picture
					for(int k {0}; k < 7; ++k) {
						TGraph* graph {(TGraph*) pGraphs.GetListOfGraphs()->At(k)};
						if (!k) {
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
					TH1D speedH {stat.getSpeedH()};

					frame.clear();

					cnvs.SetCanvasSize(windowSize.x * 0.25, windowSize.y * 0.5);
					drawObj(pGraphs, 0., 0.);
					drawObj(kBGraph, 0., 0.5);
					drawObj(speedH, 0.75, 0.);
					drawObj(mfpGraph, 0.75, 0.5);

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
					while (*fTime_ + gDeltaT_ < stat.getTime()) {
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
				drawObj(pGraphs, 0., 0.);
				drawObj(kBGraph, 0., 0.5);
				drawObj(speedH, 0.75, 0.);
				drawObj(mfpGraph, 0.75, 0.5);

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
	double gDeltaT;
	{ // guard scope
	std::lock_guard<std::mutex> gTimeGuard {gTimeMtx_};
	std::lock_guard<std::mutex> gDeltaTGuard {gDeltaTMtx_};
  if (!gTime_.has_value()) gTime_ = data[0].getTime() - gDeltaT_;
  assert(gDeltaT_ > 0.);
  assert(gTime_ <= data[0].getTime());
	gTime = *gTime_;
	gDeltaT = gDeltaT_;
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

void SimOutput::processStats(const std::vector<GasData>& data) {
  // std::cout << "Started processing stats.\n";
  std::vector<TdStats> stats{};

  assert(!std::div(data.size(), statSize_).rem);

  stats.reserve(std::div(data.size(), statSize_).quot);

  // std::cout << "Reached pre-loop\n";
  for (int i{0}; i < std::div(data.size(), statSize_).quot; ++i) {
    TdStats stat{data[i * statSize_], speedsHTemplate_};
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

void SimOutput::setDone() {
	std::lock_guard<std::mutex> guard {doneMtx_};
	done_ = true;
}

bool SimOutput::isDone() {
	std::lock_guard<std::mutex> guard {doneMtx_};
	return done_;
}

}  // namespace gasSim

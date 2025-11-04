#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <functional>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <TH1D.h>
#include <TCanvas.h>

#include "graphics.hpp"
#include "physicsEngine.hpp"

namespace gasSim {

// associate a callback to critical points of programs 

class GasData {
 public:
  GasData(const Gas& gas, const Collision* collision);

  const std::vector<Particle>& getParticles() const { return particles_; };
  const Particle& getP1() const { return particles_[p1Index_]; };
  int getP1Index() const { return p1Index_; };
  const Particle& getP2() const;
  int getP2Index() const;
  double getT0() const { return t0_; }
  double getTime() const { return time_; };
  double getBoxSide() const { return boxSide_; };
  Wall getWall() const;
  char getCollType() const;

  bool operator==(const GasData& data) const;

 private:
  std::vector<Particle> particles_;
  double t0_;
  double time_;
  double boxSide_;
  int p1Index_;
  int p2Index_;
  Wall wall_;
};

class TdStats {
 public:
  TdStats(const GasData& firstState, const TH1D& speedsHTemplate);
  TdStats(const GasData& data, const TdStats& prevStats);
  TdStats(const GasData& data, const TdStats& prevStats, const TH1D& speedsHTemplate);
	TdStats(const TdStats& s);
	TdStats(TdStats&& s) noexcept;
  // void setDeltaT(double time);
  // void setBoxSide();
  void addData(const GasData& data);

  // void reset();
  double getPressure(Wall wall) const;
  double getPressure() const;
  double getTemp() const { return T_; }
  unsigned int getNParticles() const { return lastCollPositions_.size(); };
  double getVolume() const { return std::pow(boxSide_, 3); };
  double getBoxSide() const { return boxSide_; };
  double getTime() const { return time_; };
  double getTime0() const { return t0_; };
  double getDeltaT() const { return time_ - t0_; };
	TH1D getSpeedH() const { return speedsH_; }
  double getMeanFreePath() const;
  // const std::vector<PhysVectorD>& getSpeeds() const { return speeds_; };

  bool operator==(const TdStats&) const;
	TdStats& operator=(const TdStats&);
	TdStats& operator=(TdStats&&) noexcept;

 private:
  void addPulse(const GasData& data);

  std::array<double, 6> wallPulses_{};  // cumulated pulse for each wall

  std::vector<PhysVectorD> lastCollPositions_{};
  double T_;  // non varying as for conservation of kinetic energy for every hit

  std::vector<double> freePaths_{};
	TH1D speedsH_;

  double t0_;
  double time_;  // time at latest collision whose data has been added
  double boxSide_;
};

enum class VideoOpts {
	justGas, justStats, gasPlusCoords, all
};

class SimOutput {
 public:
  SimOutput(size_t statSize, double frameRate, const TH1D& speedsHTemplate);

	void setStatChunkSize(size_t statChunkSize);
	size_t getStatChunkSize() { return statChunkSize_.load(); }
  void setStatSize(size_t statSize);
  void setFramerate(double frameRate);

  void addData(const std::vector<GasData>& data);
  void processData(bool mfpMemory = true);
  void processData(const Camera& camera, const RenderStyle& style,
                   bool mfpMemory = true);
	std::vector<sf::Texture> getVideo(
			VideoOpts opt, sf::Vector2i windowSize, sf::Texture placeholder,
			TList& prevGraphs, bool emptyQueue = false,
			std::function<void(TH1D&, VideoOpts)> fitLambda = {},
			std::array<std::function<void()>, 4> drawLambdas = {});

	// why is this method even here? anyway, it is extremely resource-intensive, never use it
  std::deque<GasData> getData();
	size_t getRawDataSize();
  double getFramerate() const { return 1. / gDeltaT_.load(); }
  size_t getStatSize() const { return statSize_.load(); }

  std::vector<TdStats> getStats(bool emptyQueue = false);
  std::vector<sf::Texture> getRenders(bool emptyQueue = false);
	bool isProcessing() { return processing_.load(); }

  bool isDone() { return dataDone_.load(); };
  void setDone() { dataDone_.store(true); };

 private:
  void processStats(const std::vector<GasData>& data, bool mfpMemory);
  void processGraphics(const std::vector<GasData>& data, const Camera& camera,
                       const RenderStyle& style);
	std::atomic<bool> dataDone_ {false};
	std::atomic<bool> processing_ {false};
	std::atomic<bool> addedResults_ {false};

  std::deque<GasData> rawData_;
	std::optional<double> rawDataBackTime_;
  std::mutex rawDataMtx_;
  std::condition_variable rawDataCv_;

	std::atomic<size_t> statSize_;
	std::atomic<size_t> statChunkSize_ {0};
  std::deque<TdStats> stats_;
  std::mutex statsMtx_;
	std::optional<TdStats> lastStat_;
	std::mutex lastStatMtx_;

	std::atomic<double> gDeltaT_;
  std::optional<double> gTime_;
	std::mutex gTimeMtx_;
  std::deque<std::pair<sf::Texture, double>> renders_;
  std::mutex rendersMtx_;
	std::mutex resultsMtx_;
	std::condition_variable resultsCv_;

	std::optional<double> fTime_;

	std::optional<size_t> nParticles_;

	const TH1D speedsHTemplate_;
};

}  // namespace gasSim

#endif

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include <SFML/Graphics/RenderTexture.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "graphics.hpp"
#include "physicsEngine.hpp"

namespace gasSim {

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
  TdStats(const GasData& firstState);
  TdStats(const GasData& data, const TdStats& prevStats);
  // void setDeltaT(double time);
  // void setBoxSide();
  void addData(const GasData& data);

  // void reset();
  double getPressure(Wall wall) const;
  double getPressure() const;
  double getTemp() const { return T_; };
  unsigned int getNParticles() const { return lastCollPositions_.size(); };
  double getVolume() const { return std::pow(boxSide_, 3); };
  double getBoxSide() const { return boxSide_; };
  double getTime() const { return time_; };
  double getTime0() const { return t0_; };
  double getDeltaT() const { return time_ - t0_; };
  double getMeanFreePath() const;
  // const std::vector<PhysVectorD>& getSpeeds() const { return speeds_; };
	
	bool operator==(const TdStats&) const;

 private:

	void addPulse(const GasData& data);

  std::array<double, 6> wallPulses_ {};  // cumulated pulse for each wall

  std::vector<PhysVectorD> lastCollPositions_ {};
  double T_; // non varying as for conservation of kinetic energy for every hit
	// std::vector<PhysVectorD> speeds_{};  // all speed values for each iteration

  std::vector<double> freePaths_ {};

  double t0_;
  double time_; // time at latest collision whose data has been added
  double boxSide_;
};

class SimOutput {
public:
	SimOutput(unsigned int statSize, double frameRate);

	void setStatSize(unsigned int statSize) { statSize_ = statSize; };
	void setFramerate(double frameRate);

	void addData(const GasData& data);
	void processData();
	void processData(const Camera& camera, const RenderStyle& style, bool stats = false);

	const std::deque<GasData>& getData() const { return rawData_; };
	double getFramerate() const { return 1. / gDeltaT_; };
	int getStatSize() const { return statSize_; };
	
	std::vector<TdStats> getStats(bool emptyQueue = false);
	std::vector<sf::Texture> getRenders(bool emptyQueue = false);

	bool isDone() { return done_; };
	void setDone() { done_ = true; };

private:
	void processStats(const std::vector<GasData>& data);
	void processGraphics(const std::vector<GasData>& data, const Camera& camera, const RenderStyle& style);

	std::deque<GasData> rawData_;
	std::mutex rawDataMtx_;
	std::condition_variable rawDataCv_;

	int statSize_;
	std::deque<TdStats> stats_;
	std::mutex statsMtx_;

	double gDeltaT_;
	std::optional<double> gTime_;
	std::deque<sf::Texture> renders_;
	std::mutex rendersMtx_;

	unsigned long nParticles_{0};

	bool done_ {false};
};

}  // namespace gasSim

#endif

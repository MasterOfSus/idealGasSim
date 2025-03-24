#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "physicsEngine.hpp"

namespace gasSim {

class TdStats {
 public:
  TdStats(const Gas* gas);
	TdStats(const Gas* gas, const TdStats& prevStats);
  // void setDeltaT(double time);
  // void setBoxSide();
	void addData(const Collision* collision);

	// void reset();

  double getPressure(Wall wall) const;
	double getTemp() const;
	int getNParticles() const { return lastCollTimes_.size(); };
	double getVolume() const { return std::pow(boxSide_, 3); };
	double getBoxSide() const { return boxSide_; };
	double getDeltaT() const {return time_ - t0_; };
	double getMeanFreePath() const;
	const std::vector<PhysVectorD>& getSpeeds() const { return speeds_; };

 private:
	const Gas* const gas_;

	std::array<double, 6> wallPulses_ {}; // cumulated pulse for each wall

	std::vector<double> lastCollTimes_ {}; // last collision times for each particle
	std::vector<PhysVectorD> speeds_ {}; // all speed values for each iteration

	std::vector<double> freePaths_ {};

  double boxSide_;
	double t0_;
  double time_;

	int nParticles_;
};

}

#endif

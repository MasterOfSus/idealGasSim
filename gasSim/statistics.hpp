#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "physicsEngine.hpp"

namespace gasSim {

class TdStats {
 public:
  TdStats(const Gas& firstState);
	TdStats(const TdStats& stats): t0_(0.), nParticles_(stats.getNParticles()) {};
  // void setDeltaT(double time);
  // void setBoxSide();
	void addData(const Gas& gas, const Collision* collision);

	// void reset();

  double getPressure(Wall wall) const;
	double getTemp() const;
	int getNParticles() const;
	double getVolume() const;
	double getBoxSide() const;
	double getDeltaT() const;
	double getMeanFreePath() const;
	const std::vector<PhysVectorD>& getSpeeds() const;

 private:
	std::array<double, 6> wallPulses_ {}; // cumulated pulse for each wall

	std::vector<double> lastCollTimes_ {}; // last collision times for each particle
	std::vector<PhysVectorD> speeds_ {}; // all speed values for each iteration

	double meanFreePath_;

  double boxSide_;
	const double t0_;
  double time_;

	const int nParticles_;
};

}

#endif

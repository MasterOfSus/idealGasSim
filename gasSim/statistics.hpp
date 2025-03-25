#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "physicsEngine.hpp"

namespace gasSim {

class TdStats {
 public:
  TdStats(const Gas& firstState);
  TdStats(const Gas& gas, const TdStats& prevStats);
  // void setDeltaT(double time);
  // void setBoxSide();
  void addData(const Gas& gas, const Collision* collision);

  // void reset();
  double getPressure(Wall wall) const;
  double getPressure() const;
  double getTemp() const;
  unsigned int getNParticles() const { return speeds_.size(); };
  double getVolume() const { return std::pow(boxSide_, 3); };
  double getBoxSide() const { return boxSide_; };
  double getTime() const { return time_; };
  double getTime0() const { return t0_; };
  double getDeltaT() const { return time_ - t0_; };
  double getMeanFreePath() const;
  const std::vector<PhysVectorD>& getSpeeds() const { return speeds_; };

 private:
  std::array<double, 6> wallPulses_{};  // cumulated pulse for each wall

  std::vector<PhysVectorD>
      lastPositions_{};                // last collision times for each particle
  std::vector<PhysVectorD> speeds_{};  // all speed values for each iteration

  std::vector<double> freePaths_{};

  double t0_;
  double time_;  //Questo tempo è il totale, quindi dovrebbe essere sincato con il time_ del Gas
  double boxSide_;
};

}  // namespace gasSim

#endif

#ifndef TDSTATS_HPP
#define TDSTATS_HPP

#include <TH1D.h>

#include "GasData.hpp"

namespace GS {

class TdStats {
 public:
  TdStats(const GasData& firstState, const TH1D& speedsHTemplate);
  TdStats(const GasData& data, TdStats&& prevStats);
  TdStats(const GasData& data, TdStats&& prevStats,
          const TH1D& speedsHTemplate);

  void addData(const GasData& data);

  double getPressure(Wall wall) const;
  double getPressure() const;
  double getTemp() const { return T; }
  unsigned int getNParticles() const { return lastCollPositions.size(); };
  double getVolume() const { return std::pow(boxSide, 3); };
  double getBoxSide() const { return boxSide; };
  double getTime() const { return time; };
  double getTime0() const { return t0; };
  double getDeltaT() const { return time - t0; };
  TH1D getSpeedH() const { return speedsH; }
  double getMeanFreePath() const;

  bool operator==(const TdStats&) const;

  TdStats(const TdStats& s);
  TdStats& operator=(const TdStats&);
  TdStats(TdStats&& s) noexcept;
  TdStats& operator=(TdStats&&) noexcept;

  ~TdStats() = default;

 private:
  void addPulse(const GasData& data);

  std::array<double, 6> wallPulses{};  // cumulated pulse for each wall

  std::vector<Vector3d> lastCollPositions{};
  double
      T;  // almost invariant as per conservation of kinetic energy in every hit

  std::vector<double> freePaths{};
  TH1D speedsH;

  double t0;
  double time;  // time at latest collision whose data has been added
  double boxSide;
};

}  // namespace GS

#endif

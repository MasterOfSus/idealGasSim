#ifndef TDSTATS_HPP
#define TDSTATS_HPP

#include "PhysicsEngine/GSVector.hpp"

#include <TH1.h>

#include <stddef.h>
#include <array>
#include <cmath>
#include <vector>

namespace GS {

class GasData;
enum class Wall;

class TdStats {
 public:
  TdStats(GasData const& firstState, TH1D const& speedsHTemplate);
  TdStats(GasData const& data, TdStats&& prevStats);
  TdStats(GasData const& data, TdStats&& prevStats,
          TH1D const& speedsHTemplate);

  TdStats(TdStats const& s);
  TdStats& operator=(TdStats const&);
  TdStats(TdStats&& s) noexcept;
  TdStats& operator=(TdStats&&) noexcept;

  ~TdStats() = default;

  double getPressure(Wall wall) const;
  double getPressure() const;
  double getTemp() const { return T; }
  size_t getNParticles() const { return lastCollPositions.size(); }
  double getVolume() const { return std::pow(boxSide, 3); }
  double getBoxSide() const { return boxSide; }
  double getTime() const { return time; }
  double getTime0() const { return t0; }
  double getDeltaT() const { return time - t0; }
  TH1D getSpeedH() const { return speedsH; }
  double getMeanFreePath() const;

  void addData(GasData const& data);

  bool operator==(TdStats const&) const;

 private:
  void addPulse(GasData const& data);

  std::array<double, 6> wallPulses{};  // cumulated pulse for each wall

  std::vector<GSVectorD> lastCollPositions{};
  double T; // would be invariant if not for fp approximation
  std::vector<double> freePaths{};
  TH1D speedsH;

  double t0;
  double time;  // time at latest collision whose data has been added
  double boxSide;
};

}  // namespace GS

#endif

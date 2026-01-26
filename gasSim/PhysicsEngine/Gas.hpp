#ifndef GAS_HPP
#define GAS_HPP

#include <cstddef>
#include <functional>
#include <vector>

#include "Collision.hpp"
#include "Particle.hpp"

namespace GS {

class GasData;
class SimDataPipeline;

class Gas {
 public:
  Gas() : boxSide{1.}, time{0.} { liveInstances.fetch_add(1); }
  Gas(std::vector<Particle>&& particles, double boxSide, double time = 0.);
  Gas(size_t particlesN, double temperature, double boxSide,
      double time = 0.);  // random parametric constructor
  ~Gas() { liveInstances.fetch_sub(1); }

  Gas(Gas const&);
  Gas& operator=(Gas const&) = default;
  Gas(Gas&&) noexcept;
  Gas& operator=(Gas&&) noexcept = default;

  void simulate(
      size_t iterationsN, std::function<bool()> stopper = [] { return false; });
  void simulate(
      size_t iterationsN, SimDataPipeline& SimOutput,
      std::function<bool()> stopper = [] { return false; });
  std::vector<GS::GasData> rawDataSimulate(
      size_t iterationsN);  // mainly for testing purposes
  bool contains(const Particle& p);

  const std::vector<Particle>& getParticles() const { return particles; }
  double getBoxSide() const { return boxSide; }
  double getTime() const { return time; }

  static size_t gasInstances() { return liveInstances.load(); }

 private:
  inline static std::atomic<size_t> liveInstances{0};

  PWCollision firstPWColl();
  PPCollision firstPPColl();
  void move(double dt);

  std::vector<Particle> particles{};
  double boxSide;
  double time;
};
}  // namespace GS

#endif

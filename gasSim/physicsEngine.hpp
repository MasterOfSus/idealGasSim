#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <vector>

namespace gasSim {
namespace physics {
struct PhysVector {
  double x;
  double y;
  double z;

  PhysVector operator+(const PhysVector& v) const;
  PhysVector operator-(const PhysVector& v) const;

  PhysVector operator*(const double c) const;
  PhysVector operator/(const double c) const;

  double operator*(const PhysVector& v) const;

  bool operator==(const PhysVector& v) const;
  bool operator!=(const PhysVector& v) const;

  double norm() const;
};

PhysVector randomVector(const double maxNorm);
PhysVector randomVectorGauss(const double standardDev);

struct Particle {
  static double mass;
  static double radius;

  PhysVector position;
  PhysVector speed;

  bool operator==(const Particle& p) const;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(int nParticles, double temperature, double boxSide);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;

  void updateGasState();  // called in each iteration of the game loop

 private:
  std::vector<Particle> particles_;
  double boxSide_;  // side of the cubical container
  void updatePositions(double time);
};

}  // namespace physics
}  // namespace gasSim

#endif

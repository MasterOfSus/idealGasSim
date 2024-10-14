#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <variant>
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

PhysVector operator*(const double c, const PhysVector v);

PhysVector randomVector(const double maxNorm);
PhysVector randomVectorGauss(const double standardDev);

struct Particle {
  static double mass;
  static double radius;

  PhysVector position;
  PhysVector speed;
};
struct Wall {
  double a, b, c, d;
};

class Collision {
 public:
  enum class CollisionType { ParticleToParticle, ParticleToWall };
  Collision(double t, Particle& p1, Particle& p2);
  Collision(double t, Particle& p1, Wall& w);

  CollisionType getCollisionType() const;
  double getTime() const;

  Particle& getFirstParticle() const;

  Particle& getSecondParticle() const;

  Wall& getWall() const;

 private:
  CollisionType type;
  Particle* firstParticle;
  Particle* secondParticle;
  Wall* wall;
  double time;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(int nParticles, double temperature, double boxSide);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;

  void updateGasState(
      Collision fisrtCollision);  // called in each iteration of the game loop

 private:
  std::vector<Particle> particles_;
  double boxSide_;  // side of the cubical container
  void updatePositions(double time);
};

}  // namespace physics
}  // namespace gasSim

#endif

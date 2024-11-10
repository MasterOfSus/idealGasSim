#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <string>
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
  Collision(double t, Particle& p1);

  double getTime() const;

  Particle* getFirstParticle() const;

  virtual std::string getCollisionType() const = 0;
  virtual void resolveCollision() = 0;

 private:
  Particle* firstParticle_;
  double time;
};

class WallCollision : public Collision {
 public:
  WallCollision(double t, Particle& p1, char wall);
  std::string getCollisionType() const override;
  void resolveCollision() override;

 private:
  char wall_;
};

class ParticleCollision : public Collision {
 public:
  ParticleCollision(double t, Particle& p1, Particle& p2);
  std::string getCollisionType() const override;
  void resolveCollision() override;

 private:
  Particle* secondParticle_;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(int nParticles, double temperature, double boxSide);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;


  void gasLoop(int iteration);
  void updateGasState(
      Collision* fisrtCollision);  // called in each iteration of the game loop
  WallCollision* findFirstWallCollision(double maxTime);
  ParticleCollision* findFirstPartCollision(double maxTime);


 private:
  std::vector<Particle> particles_;
  double boxSide_;  // side of the cubical container
  void updatePositions(double time); // probabilmente non servirà più
};
}  // namespace physics
}  // namespace gasSim

#endif

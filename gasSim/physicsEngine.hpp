#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <set>
#include <string>
#include <variant>
#include <vector>

namespace gasSim {
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

  PhysVector position = {};
  PhysVector speed = {};
};
bool particleOverlap(const Particle& p1, const Particle& p2);

struct Wall {
  double a, b, c, d;
};

class Collision {
 public:
  Collision(double t, Particle* p1);

  double getTime() const;

  Particle* getFirstParticle() const;

  virtual std::string getCollisionType() const = 0;
  virtual void resolve() = 0;

 private:
  double time;
  Particle* firstParticle_;
};

class WallCollision : public Collision {
 public:
  WallCollision(double t, Particle* p1, char wall);
  char getWall() const;
  std::string getCollisionType() const override;
  void resolve() override;

 private:
  char wall_;
};

class ParticleCollision : public Collision {
 public:
  ParticleCollision(double t, Particle* p1, Particle* p2);
  Particle* getSecondParticle() const;
  std::string getCollisionType() const override;
  void resolve() override;

 private:
  Particle* secondParticle_;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(std::vector<Particle> particles, double boxSide);
  Gas(int nParticles, double temperature, double boxSide);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;

  void gasLoop(int nIterations);
  void updateGasState(
      Collision fisrtCollision);  // called in each iteration of the game loop
  WallCollision findFirstWallCollision();
  ParticleCollision findFirstPartCollision();

 private:
  std::vector<Particle> particles_;
  double boxSide_;                    // side of the cubical container
  void updatePositions(double time);  // probabilmente non servirà più
};

double collisionTime(const Particle& p1, const Particle& p2);

WallCollision calculateWallColl(Particle& p, double squareSide);

double collisionTime(double wallDistance, double speed);
}  // namespace gasSim

#endif

#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <set>
#include <string>
#include <SFML/Graphics.hpp>
#include <variant>
#include <vector>

namespace gasSim {
struct PhysVector {
  double x;
  double y;
  double z;

  PhysVector operator-() const;

  PhysVector operator+(const PhysVector& v) const;
  PhysVector operator-(const PhysVector& v) const;
  void operator+=(const PhysVector& v);
  void operator-=(const PhysVector& v);

  PhysVector operator*(const double c) const;
  PhysVector operator/(const double c) const;

  double operator*(const PhysVector& v) const;

  bool operator==(const PhysVector& v) const;
  bool operator!=(const PhysVector& v) const;

  double norm() const;

  void normalize();
};

PhysVector operator*(const double c, const PhysVector v);

PhysVector unifRandoVector(const double maxNorm);
PhysVector gausRandVector(const double standardDev);

struct Particle {
  static double mass;
  static double radius;

  PhysVector position = {};
  PhysVector speed = {};
};

class Box {
  PhysVector vertex;
};

class Hit {
 public:
  double getTime() const { return time_; };

 protected:
  Hit(double time) : time_{time} {};

 private:
  double time_;
};

class WallHit : protected Hit {
 public:
  const Particle& getParticle1() const;
  int getWallIndex() const;
  WallHit(const Particle& particle, const Box& box, int wallIndex_);

 private:
  Particle particle_;
  Box box_;
  int wallIndex_;
};

class PartHit : protected Hit {
 public:
  const Particle& getParticle1() const;
  const Particle& getParticle2() const;
  PartHit(const Particle& particle1, const Particle& particle2);

 private:
  Particle particle1_;
  Particle particle2_;
};
bool particleOverlap(const Particle& p1, const Particle& p2);

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

class PartCollision : public Collision {
 public:
  PartCollision(double t, Particle* p1, Particle* p2);
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
  double getLife() const;

  void gasLoop(int nIterations);

 private:
  std::vector<Particle> particles_;
  double boxSide_;  // side of the cubical container
  double life_{0};

  WallCollision firstWallCollision();
  PartCollision firstPartCollision();
  void updatePositions(double time);

  Hit* getNextHit();
  void solveNextEvent();
};

double collisionTime(const Particle& p1, const Particle& p2);

WallCollision calculateWallColl(Particle& p, double squareSide);

double collisionTime(double wallDistance, double speed);
}  // namespace gasSim

#endif

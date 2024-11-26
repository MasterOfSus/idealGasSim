#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <SFML/Graphics.hpp>
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
  const int getWallIndex() const;
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

class Collision {
 public:
  virtual void solveCollision();

 private:
  Collision() {};
};

class WallCollision : public Collision {
 public:
  virtual void solveCollision();
  WallCollision(const Particle& particle1, const Box& box, int wallIndex);

 private:
  Particle particle1_;
  static Box box_;
  int wallIndex_;
};

class PartCollision : public Collision {
 public:
  virtual void solveCollision();
  PartCollision(const Particle& particle1, const Particle& particle2);
  PartCollision(PartHit partHit);

 private:
  Particle particle1_;
  Particle particle2_;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(int nParticles, double temperature, Box box);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;

  Hit* getNextHit();

  void solveNextEvent();

 private:
  std::vector<Particle> particles_;
  Box box_;
  void updatePositions(double time);
};

}  // namespace physics
}  // namespace gasSim

#endif

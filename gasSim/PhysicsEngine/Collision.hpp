#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <string>

#include "Particle.hpp"

namespace GS {

enum class Wall { Front, Back, Left, Right, Top, Bottom, VOID };

struct Collision {
 public:
  double getTime() const { return time; }
  Particle const* getP1() const { return p1; }
  virtual char getType() const = 0;
  virtual void solve() = 0;
  virtual ~Collision() = 0;
  Collision(double time, Particle* p);

  Particle* getP1() { return p1; };
 private:
  Particle* p1;
  double time;
};

struct PWCollision final : public Collision {
 public:
  Wall getWall() const { return wall; }
  char getType() const override { return 'w'; }
  void solve() override;

  PWCollision(double time, Particle* p1, Wall wall);
 private:
  Wall wall;
};

struct PPCollision final : public Collision {
 public:
  Particle const* getP2() const { return p2; }
  char getType() const override { return 'p'; }
  void solve() override;

  PPCollision(double time, Particle* p1, Particle* p2);
 private:
  Particle* p2;
};

}  // namespace GS

#endif

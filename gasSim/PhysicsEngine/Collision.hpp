#ifndef COLLISION_HPP
#define COLLISION_HPP

namespace GS {

struct Particle;

enum class Wall { Front, Back, Left, Right, Top, Bottom, VOID };

struct Collision {
 public:
  Collision(double time, Particle* p);

  virtual char getType() const = 0;
  virtual void solve() = 0;

  Particle* getP1() { return p1; }
  Particle const* getP1() const { return p1; }
  double getTime() const { return time; }

 private:
  Particle* p1;
  double time;
};

struct PWCollision final : public Collision {
 public:
  PWCollision(double time, Particle* p1, Wall wall);

  void solve() override;
  char getType() const override { return 'w'; }

  Wall getWall() const { return wall; }

 private:
  Wall wall;
};

struct PPCollision final : public Collision {
 public:
  PPCollision(double time, Particle* p1, Particle* p2);

  char getType() const override { return 'p'; }
  void solve() override;

  Particle* getP2() const { return p2; }

 private:
  Particle* p2;
};

}  // namespace GS

#endif

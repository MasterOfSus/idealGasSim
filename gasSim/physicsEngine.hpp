#ifndef PHYSICS_ENGINE_HPP
#define PHYSICS_ENGINE_HPP

#include <SFML/Graphics.hpp>
#include <array>
#include <cmath>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace gasSim {

template <typename FP>
struct PhysVector {
  static_assert(std::is_floating_point_v<FP>);
  FP x;
  FP y;
  FP z;

	// default constructor
	PhysVector(FP vx = 0., FP vy = 0., FP vz = 0.): x {vx}, y {vy}, z {vz} {};

	// conversion constructor
	template <typename FP1>
	explicit PhysVector(const PhysVector<FP1>& v):
	x {static_cast<FP>(v.x)}, y {static_cast<FP>(v.y)}, z {static_cast<FP>(v.z)} {};

  PhysVector operator-() const { return {-x, -y, -z}; }
  PhysVector operator+(const PhysVector& v) const {
    return {x + v.x, y + v.y, z + v.z};
  }
  PhysVector operator-(const PhysVector& v) const {
    return {x - v.x, y - v.y, z - v.z};
  }

  void operator+=(const PhysVector& v) { *this = *this + v; }
  void operator-=(const PhysVector& v) { *this = *this - v; }

  PhysVector operator*(FP c) const { return {x * c, y * c, z * c}; }
  PhysVector operator/(FP c) const { return {x / c, y / c, z / c}; }

  FP operator*(const PhysVector& v) const {
    return x * v.x + y * v.y + z * v.z;
  }

	PhysVector cross(const PhysVector& v) const {
		return {
			y * v.z - z * v.y,
			z * v.x - x * v.z,
			x * v.y - y * v.x
		};
	}

  bool operator==(const PhysVector<FP>& v) const {
    return (x == v.x && y == v.y && z == v.z);
  }
  bool operator!=(const PhysVector<FP>& v) const { return !(*this == v); }

  double norm() const { return std::sqrt(x * x + y * y + z * z); }
  void normalize() { *this = *this / this->norm(); }
};

template <typename FP>
PhysVector<FP> operator*(double c, const PhysVector<FP> v) {
  return v * c;
};

using PhysVectorF = PhysVector<float>;
using PhysVectorD = PhysVector<double>;

PhysVectorD unifRandVector(const double maxNorm);
PhysVectorD gausRandVector(const double standardDev);

enum class Wall { Front, Back, Left, Right, Top, Bottom };

struct Particle {
  static double mass;
  static double radius;

  PhysVectorD position = {};
  PhysVectorD speed = {};
};

bool particleOverlap(const Particle& p1, const Particle& p2);
bool particleInBox(const Particle& part, double boxSide);

class Statistic {
 public:
  Statistic(double boxSide);
  void setDeltaTime(double deltaTime);
  // void setBoxSide();

  void addImpulseOnWall(double speed, Wall wall);
  double getPressure(Wall wall);

  void calculate();

 private:
  std::unordered_map<Wall, double> pressure;
  std::unordered_map<Wall, double> deltaImpulse_;

  double boxSide_;
  double deltaTime_;
};

class Collision {
 public:
  Collision(double t, Particle* p);

  double getTime() const;

  Particle* getFirstParticle() const;

  virtual std::string getCollisionType() const = 0;
  virtual void resolve(Statistic& oldStat) = 0;

 private:
  double time_;
  Particle* firstParticle_;
};

class WallCollision : public Collision {
 public:
  WallCollision(double t, Particle* p, Wall wall);
  Wall getWall() const;
  std::string getCollisionType() const override;
  void resolve(Statistic& oldStat) override;

 private:
  Wall wall_;
};

class PartCollision : public Collision {
 public:
  PartCollision(double t, Particle* p1, Particle* p2);
  Particle* getSecondParticle() const;
  std::string getCollisionType() const override;
  void resolve(Statistic& oldStat) override;

 private:
  Particle* secondParticle_;
};

class Gas {
 public:
  Gas(const Gas& gas);
  Gas(std::vector<Particle> particles, double boxSide, double life = 0.);
  Gas(int nParticles, double temperature, double boxSide);

  const std::vector<Particle>& getParticles() const;
  double getBoxSide() const;
  double getLife() const;

  Statistic gasLoop(int nIterations);

 private:
  std::vector<Particle> particles_;
  double boxSide_;
  double life_{0.};

  WallCollision firstWallCollision();
  PartCollision firstPartCollision();
  void updatePositions(double time);

  void solveNextEvent();
};

double collisionTime(const Particle& p1, const Particle& p2);

WallCollision calculateWallColl(Particle& p, double squareSide);

double collisionTime(double wallDistance, double speed);

}  // namespace gasSim

#endif

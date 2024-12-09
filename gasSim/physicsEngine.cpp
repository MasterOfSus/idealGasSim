#include "physicsEngine.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
// #include <execution> //Se decidiamo di fare qualche operazione in parallelo:
// Sddfqoinp

#include "algorithms.hpp"

namespace gasSim {

// Definition of vector functions
PhysVector PhysVector::operator-() const { return {-x, -y, -z}; }

PhysVector PhysVector::operator+(const PhysVector& v) const {
  return {x + v.x, y + v.y, z + v.z};
}

PhysVector PhysVector::operator-(const PhysVector& v) const {
  return {x - v.x, y - v.y, z - v.z};
}

void PhysVector::operator+=(const PhysVector& v) { *this = *this + v; }

void PhysVector::operator-=(const PhysVector& v) { *this = *this - v; }

PhysVector PhysVector::operator*(const double c) const {
  return {x * c, y * c, z * c};
}

PhysVector PhysVector::operator/(const double c) const {
  return {x / c, y / c, z / c};
}

double PhysVector::operator*(const PhysVector& v) const {
  return x * v.x + y * v.y + z * v.z;
}

bool PhysVector::operator==(const PhysVector& v) const {
  return (x == v.x && y == v.y && z == v.z);
}

PhysVector operator*(const double c, const PhysVector v) { return v * c; }

bool PhysVector::operator!=(const PhysVector& v) const { return !(*this == v); }

double PhysVector::norm() const { return std::sqrt(x * x + y * y + z * z); }

void PhysVector::normalize() { *this = *this / this->norm(); }

PhysVector unifRandVector(const double maxNorm) {
  if (maxNorm <= 0) {
    throw std::invalid_argument("maxNorm must be greater than 0");
  }
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0., std::sqrt(maxNorm / 3));
  return {dist(eng), dist(eng), dist(eng)};
}
PhysVector gausRandVector(const double standardDev) {
  if (standardDev <= 0) {
    throw std::invalid_argument("standardDev must be greater than 0");
  }
  static std::default_random_engine eng(std::random_device{}());
  std::normal_distribution<double> dist(0., standardDev);
  return {dist(eng), dist(eng), dist(eng)};
}
// End of PhysVector functions

// Definitio of Particle functions
bool particleOverlap(const Particle& p1, const Particle& p2) {
  const double minDst = 2 * Particle::radius;
  const double centerDst{(p1.position - p2.position).norm()};
  if (centerDst < minDst) {
    return true;
  } else {
    return false;
  }
}

bool particleInBox(const Particle& part, double boxSide) {
  const double radius = Particle::radius;
  const auto& pos = part.position;

  if (pos.x < radius || pos.x > boxSide - radius || pos.y < radius ||
      pos.y > boxSide - radius || pos.z < radius || pos.z > boxSide - radius) {
    return false;
  }

  return true;
}
// End of Particle functions

// Definition of Collision functions
Collision::Collision(double t, Particle* p) : time_(t), firstParticle_(p) {
  assert(t > 0);
}

double Collision::getTime() const { return time_; }

Particle* Collision::getFirstParticle() const {
  return firstParticle_;
}  // ATTENZIONE QUA COI PUNTATORI CHE PUNTANO

void Collision::resolve() { time_ = 0; }
// End of Collision functions

// Definition of WallCollision functions
WallCollision::WallCollision(double t, Particle* p, char wall)
    : Collision(t, p), wall_(wall) {}

char WallCollision::getWall() const { return wall_; }
std::string WallCollision::getCollisionType() const {
  return "Particle to Wall Collision";
}

void WallCollision::resolve() {
  Particle* part{getFirstParticle()};

  switch (wall_) {
    case 'l':
    case 'r':
      part->speed.x = -part->speed.x;
      break;
    case 'f':
    case 'b':
      part->speed.y = -part->speed.y;
      break;
    case 'u':
    case 'd':
      part->speed.z = -part->speed.z;
      break;
  }
  Collision::resolve();
}
// End of WallCollision functions

// Definition of PartCollision functions
PartCollision::PartCollision(double t, Particle* p1, Particle* p2)
    : Collision(t, p1), secondParticle_(p2) {}

Particle* PartCollision::getSecondParticle() const { return secondParticle_; }

std::string PartCollision::getCollisionType() const {
  return "Particle to Particle Collision";
}

void PartCollision::resolve() {
  Particle* part1{getFirstParticle()};
  Particle* part2{secondParticle_};

  PhysVector centerDist{part1->position - part2->position};
  centerDist.normalize();

  PhysVector speedDiff{part1->speed - part2->speed};

  double projection = centerDist * speedDiff;

  part1->speed -= centerDist * projection;
  part2->speed += centerDist * projection;
  Collision::resolve();
}
// End of PartCollision functions

// Definition of Gas functions
Gas::Gas(const Gas& gas)
    : particles_(gas.getParticles()), boxSide_(gas.getBoxSide()) {}

Gas::Gas(std::vector<Particle> particles, double boxSide)
    : particles_(particles), boxSide_(boxSide) {
  if (particles_.size() == 0) {
    throw std::invalid_argument("particles must have at least 1 element");
  }
  if (boxSide_ <= 0) {
    throw std::invalid_argument("boxSide must be greater than 0");
  }

  std::for_each(particles_.begin(), particles_.end(), [&](const Particle& p) {
    if (particleInBox(p, boxSide_) == false) {
      throw std::invalid_argument("particles must be in the box");
    }
  });

  for_each_couple(
      particles_.begin(), particles_.end(),
      [](const Particle& p1, const Particle& p2) {
        if (particleOverlap(p1, p2) == true) {
          throw std::invalid_argument("particles cannot penetrate each other");
        }
      });
}

Gas::Gas(int nParticles, double temperature, double boxSide)
    : boxSide_(boxSide) {
  if (nParticles <= 0) {
    throw std::invalid_argument("nParticles must be greater than 0");
  }
  if (temperature <= 0) {
    throw std::invalid_argument("temperature must be greater than 0");
  }
  if (boxSide_ <= 0) {
    throw std::invalid_argument("boxSide must be greater than 0");
  }

  double minDst{2 * Particle::radius};
  int elementPerSide{static_cast<int>(std::ceil(std::cbrt(nParticles)))};
  double particleDst = boxSide / elementPerSide;

  if (particleDst <= minDst) {
    throw std::runtime_error(
        "particles are too large/too many, they don't fit in the box");
  }

  double maxSpeed = 4. / 3. * temperature;  // espressione sbagliata appena
  // riesco faccio il calcolo

  auto gridVector = [=](int i) {
    int x{i % elementPerSide};
    int y{(i / elementPerSide) % elementPerSide};
    int z{i / (elementPerSide * elementPerSide)};
    return PhysVector{x * particleDst, y * particleDst, z * particleDst};
  };

  int index{0};
  std::generate_n(std::back_inserter(particles_), nParticles, [=, &index]() {
    Particle p{{gridVector(index)}, unifRandVector(maxSpeed)};
    ++index;
    return p;
  });
}

const std::vector<Particle>& Gas::getParticles() const { return particles_; }

double Gas::getBoxSide() const { return boxSide_; }

double Gas::getLife() const { return life_; }

void Gas::gasLoop(int nIterations) {
  for (int i{0}; i != nIterations; ++i) {
    PartCollision pColl{firstPartCollision()};
    WallCollision wColl{firstWallCollision()};

    Collision* firstColl{nullptr};

    if (pColl.getTime() < wColl.getTime()) {
      firstColl = &pColl;
    } else {
      firstColl = &wColl;
    }

    updatePositions(firstColl->getTime());
    life_ += firstColl->getTime();

    assert(life_ != INFINITY);
    firstColl->resolve();
  }
}

WallCollision Gas::firstWallCollision() {
  WallCollision firstColl{INFINITY, nullptr, 'l'};

  std::for_each(particles_.begin(), particles_.end(), [&](Particle& p) {
    WallCollision coll{calculateWallColl(p, boxSide_)};
    if (coll.getTime() < firstColl.getTime()) {
      firstColl = coll;
    }
  });
  return firstColl;
}

PartCollision Gas::firstPartCollision() {
  double topTime{INFINITY};
  Particle* firstPart{nullptr};
  Particle* secondPart{nullptr};

  for_each_couple(particles_.begin(), particles_.end(),
                  [&](Particle& p1, Particle& p2) {
                    double time{collisionTime(p1, p2)};
                    if (time < topTime) {
                      firstPart = &p1;
                      secondPart = &p2;
                      topTime = time;
                    }
                  });
  return {topTime, firstPart, secondPart};
}

void Gas::updatePositions(double time) {
  std::for_each(particles_.begin(), particles_.end(),
                [time](Particle& part) { part.position += part.speed * time; });
  // Sddfqoinp
  /*std::for_each(std::execution::par, particles_.begin(), particles_.end(),
              [time](Particle& part) { part.position += part.speed * time; });*/
}
// End of Gas functions

double collisionTime(const Particle& p1, const Particle& p2) {
  PhysVector relativeSpd = p1.speed - p2.speed;
  PhysVector relativePos = p1.position - p2.position;

  double a = relativeSpd * relativeSpd;
  double b = relativePos * relativeSpd;
  double c = (relativePos * relativePos) - 4 * std::pow(Particle::radius, 2);

  double result = INFINITY;

  double discriminant = std::pow(b, 2) - a * c;

  if (discriminant > 0) {
    double sqrtDiscriminant = std::sqrt(discriminant);

    double t1 = (-b - sqrtDiscriminant) / a;
    double t2 = (-b + sqrtDiscriminant) / a;

    if (t1 > 0) {
      result = t1;
    } else if (t2 > 0) {
      result = t2;
    }
  }
  return result;
}

WallCollision calculateWallColl(Particle& p, double squareSide) {
  auto wallDistance = [&](double pos, double spd) -> double {
    if (spd < 0) {
      return -Particle::radius - pos;
    } else {
      return squareSide - Particle::radius - pos;
    }
  };

  auto calculateTime = [&](double position, double speed) {
    return wallDistance(position, speed) / speed;
  };

  double timeX{calculateTime(p.position.x, p.speed.x)};
  double timeY{calculateTime(p.position.y, p.speed.y)};
  double timeZ{calculateTime(p.position.z, p.speed.z)};

  double minTime{timeX};

  char wall = (p.speed.x < 0) ? 'l' : 'r';

  if (timeY < minTime) {
    minTime = timeY;
    wall = (p.speed.y < 0) ? 'f' : 'b';
  }

  if (timeZ < minTime) {
    minTime = timeZ;
    wall = (p.speed.z < 0) ? 'd' : 'u';
  }

  assert(minTime > 0);

  return {minTime, &p, wall};
}

// Alternativa a WallCollsion
/*
WallCollision calculateWallColl(Particle& p, double squareSide) {
    struct AxisData {
        double position;
        double speed;
        char negWall;
        char posWall;
    };

    AxisData axes[3] = {
        { p.position.x, p.speed.x, 'l', 'r' },
        { p.position.y, p.speed.y, 'f', 'b' },
        { p.position.z, p.speed.z, 'd', 'u' }
    };

    auto wallDistance = [&](double pos, double spd) {
        if (spd < 0.0) {
            return -Particle::radius - pos;
        } else {
            return squareSide - Particle::radius - pos;
        }
    };

    double minTime = INFINITY;
    char wall = '?';

    for (auto& axis : axes) {
        if (axis.speed == 0.0) {
            continue;
        }

        double distance = wallDistance(axis.position, axis.speed);
        double t = distance / axis.speed;

        // Consideriamo solo tempi positivi (collisioni future)
        if (t > 0.0 && t < minTime) {
            minTime = t;
            wall = (axis.speed < 0.0) ? axis.negWall : axis.posWall;
        }
    }

    assert(std::isfinite(minTime) && minTime > 0.0);

    return {minTime, &p, wall};
}*/
}  // namespace gasSim

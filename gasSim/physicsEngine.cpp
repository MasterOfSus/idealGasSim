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

#include "algorithms.hpp"

namespace gasSim {

// double Particle::radius = 1.;

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

PhysVector unifRandoVector(const double maxNorm) {
  if (maxNorm <= 0) {
    throw std::invalid_argument("maxNorm must be greater than 0");
  }
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0., pow(maxNorm / 3, 1. / 2.));
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
  double centerDistance{(p1.position - p2.position).norm()};
  if (centerDistance < (p1.radius + p2.radius)) {
    return true;
  } else {
    return false;
  }
}
// End of Particle functions

// Definition of Collision functions
Collision::Collision(double t, Particle* p1) : time(t), firstParticle_(p1) {}

double Collision::getTime() const { return time; }

Particle* Collision::getFirstParticle() const {
  return firstParticle_;
}  // ATTENZIONE QUA COI PUNTATORI CHE PUNTANO
// End of Collision functions

// Definition of WallCollision functions
WallCollision::WallCollision(double t, Particle* p1, char wall)
    : Collision(t, p1), wall_(wall) {}

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
    if (p.position.x < 0 || p.position.y < 0 || p.position.z < 0 ||
        p.position.x > boxSide_ || p.position.y > boxSide_ ||
        p.position.z > boxSide_) {
      throw std::invalid_argument("particles must be in the box");
    }
  });

  for_each_couple(
      particles.begin(), particles.end(),
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

  int elementPerSide{static_cast<int>(std::ceil(cbrt(nParticles)))};
  double particleDistance = boxSide / elementPerSide;

  if (particleDistance <= 2 * Particle::radius) {
    throw std::runtime_error(
        "particles are too large/too many, they don't fit in the box");
  }

  double maxSpeed = 4. / 3. * temperature;  // espressione sbagliata appena
  // riesco faccio il calcolo

  auto gridVector = [=](int i) {
    int x{i % elementPerSide};
    int y{(i / elementPerSide) % elementPerSide};
    int z{i / (elementPerSide * elementPerSide)};
    return PhysVector{x * particleDistance, y * particleDistance,
                      z * particleDistance};
  };

  int index{0};
  std::generate_n(std::back_inserter(particles_), nParticles, [=, &index]() {
    Particle p{{gridVector(index)}, unifRandoVector(maxSpeed)};
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

    try {
      pColl = firstPartCollision();

      if (pColl.getTime() < wColl.getTime()) {
        firstColl = &pColl;
      } else {
        firstColl = &wColl;
      }
    } catch (const std::logic_error& e) {
      firstColl = &wColl;
    }

    updatePositions(firstColl->getTime());
    life_ += firstColl->getTime();

    firstColl->resolve();
  }
}

WallCollision Gas::firstWallCollision() {
  WallCollision firstColl{INFINITY, nullptr, 'x'};

  std::for_each(particles_.begin(), particles_.end(), [&](Particle& p) {
    WallCollision coll{calculateWallColl(p, boxSide_)};
    if (coll.getTime() < firstColl.getTime()) {
      firstColl = coll;
    }
  });

  if (firstColl.getTime() == INFINITY) {
    throw std::logic_error("No particle to wall collisions found");
  }
  return firstColl;
}

PartCollision Gas::firstPartCollision() {
  double topTime{INFINITY};
  Particle* firstPart;
  Particle* secondPart;

  for_each_couple(particles_.begin(), particles_.end(),
                  [&](Particle& p1, Particle& p2) {
                    double time{collisionTime(p1, p2)};
                    if (time < topTime) {
                      firstPart = &p1;
                      secondPart = &p2;
                      topTime = time;
                    }
                  });

  if (topTime == INFINITY) {
    throw std::logic_error("No particle to particle collisions found");
  }
  return {topTime, firstPart, secondPart};
}

void Gas::updatePositions(double time) {
  std::for_each(particles_.begin(), particles_.end(),
                [time](Particle& part) { part.position += part.speed * time; });
}
// End of Gas functions

double collisionTime(const Particle& p1, const Particle& p2) {
  PhysVector relativeSpeed = p1.speed - p2.speed;
  PhysVector relativePosition = p1.position - p2.position;

  double a = relativeSpeed * relativeSpeed;
  double b = relativePosition * relativeSpeed;
  double c =
      (relativePosition * relativePosition) - 4 * std::pow(Particle::radius, 2);

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
  auto wallDistance = [&](double position, double speed) -> double {
    if (speed < 0) {
      return -Particle::radius - position;
    } else {
      return squareSide - Particle::radius - position;
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
}  // namespace gasSim

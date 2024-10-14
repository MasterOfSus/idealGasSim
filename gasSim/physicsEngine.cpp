#include "physicsEngine.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <csignal>
#include <random>
#include <stdexcept>

namespace gasSim {
namespace physics {

// Definition of vector functions
PhysVector PhysVector::operator+(const PhysVector& v) const {
  return {x + v.x, y + v.y, z + v.z};
}

PhysVector PhysVector::operator-(const PhysVector& v) const {
  return {x - v.x, y - v.y, z - v.z};
}

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

PhysVector randomVector(const double maxNorm) {
  assert(maxNorm > 0);
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0., pow(maxNorm / 3, 1. / 2.));
  return {dist(eng), dist(eng), dist(eng)};
}
PhysVector randomVectorGauss(const double standardDev) {
  assert(standardDev > 0);
  static std::default_random_engine eng(std::random_device{}());
  std::normal_distribution<double> dist(0., standardDev);
  return {dist(eng), dist(eng), dist(eng)};
}

/*PhysVector gridVector(int n) {
  static int tot;
  static double side;
  static int pointPerSide = (std::ceil(std::cbrt(tot)));

  static double distance = side / pointPerSide;
  // aggiungere accert che controlla che le particelle non si compenetrino

  int x{n % pointPerSide};
  int y{(n / pointPerSide) % pointPerSide};
  int z{n / (pointPerSide * pointPerSide)};

  return {x * distance, y * distance, z * distance};
}*/
// End of PhysVector functions

Collision::Collision(double t, Particle& p1, Particle& p2)
    : type(CollisionType::ParticleToParticle),
      firstParticle(&p1),
      secondParticle(&p2),
      wall(nullptr),
      time(t) {}

Collision::Collision(double t, Particle& p1, Wall& w)
    : type(CollisionType::ParticleToWall),
      firstParticle(&p1),
      secondParticle(nullptr),
      wall(&w),
      time(t) {}

Collision::CollisionType Collision::getCollisionType() const { return type; }
double Collision::getTime() const { return time; }

Particle& Collision::getFirstParticle() const { return *firstParticle; }

Particle& Collision::getSecondParticle() const {
  if (type == CollisionType::ParticleToParticle) {
    return *secondParticle;  // Restituisce la seconda particella come referenza
  } else {
    throw std::runtime_error(
        "No second particle in a ParticleToWall collision");
  }
}

Wall& Collision::getWall() const {
  if (type == CollisionType::ParticleToWall) {
    return *wall;
  } else {
    throw std::runtime_error("No wall in a ParticleToParticle collision");
  }
}

// Definition of Gas functions
Gas::Gas(const Gas& gas) : particles_(gas.particles_), boxSide_(gas.boxSide_) {}

Gas::Gas(int nParticles, double temperature, double boxSide)
    : boxSide_(boxSide) {
  assert(nParticles > 0);
  assert(temperature > 0);
  assert(boxSide > 0);

  int elementPerSide{static_cast<int>(std::ceil(cbrt(nParticles)))};
  double particleDistance = boxSide / elementPerSide;
  assert(particleDistance > 2 * Particle::radius);

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
    Particle p{{gridVector(index)}, randomVector(maxSpeed)};
    ++index;
    return p;
  });
}

const std::vector<Particle>& Gas::getParticles() const { return particles_; }
double Gas::getBoxSide() const { return boxSide_; }
// End of Gas functions

}  // namespace physics
}  // namespace gasSim

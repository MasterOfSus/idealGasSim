#include "physicsEngine.hpp"

#include <algorithm>
#include <cmath>
#include <csignal>
#include <random>

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

bool PhysVector::operator!=(const PhysVector& v) const { return !(*this == v); }

double PhysVector::norm() const { return std::sqrt(x * x + y * y + z * z); }

PhysVector randomVector(const double maxNorm) {
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, pow(maxNorm / 3, 1. / 2.));
  return {dist(eng), dist(eng), dist(eng)};
}

PhysVector gridVector(int n) {
  static int tot;
  static double side;
  static int pointPerSide = (std::ceil(std::cbrt(tot)));

  static double distance = side / pointPerSide;
  // aggiungere accert che controlla che le particelle non si compenetrino

  int x{n % pointPerSide};
  int y{(n / pointPerSide) % pointPerSide};
  int z{n / (pointPerSide * pointPerSide)};

  return {x * distance, y * distance, z * distance};
}
// End of PhysVector functions

// Definition of Gas functions

Gas::Gas(const Gas& gas) : particles_(gas.particles_), boxSide_(gas.boxSide_) {}

Gas::Gas(int nParticles, double temperature, double boxSide)
    : boxSide_(boxSide) {
  int index{0};
  int elementPerSide{static_cast<int>(std::ceil(cbrt(nParticles)))};
  double particleDistance = boxSide / elementPerSide;
  double maxSpeed = 4. / 3. * temperature;  // espressione sbagliata appena
  // riesco faccio il calcolo

  std::generate_n(std::back_inserter(particles_), nParticles, [=, &index]() {
    // aggiungere assert che controlla che le particelle non si
    // compenetrino

    int x{index % elementPerSide};
    int y{(index / elementPerSide) % elementPerSide};
    int z{index / (elementPerSide * elementPerSide)};

    Particle p{
        {x * particleDistance, y * particleDistance, z * particleDistance},
        randomVector(maxSpeed)};
    ++index;
    return p;
  });
}

const std::vector<Particle>& Gas::getParticles() const { return particles_; }
double Gas::getBoxSide() const { return boxSide_; }
// End of Gas functions

}  // namespace physics
}  // namespace gasSim

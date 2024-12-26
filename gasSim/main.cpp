#include <iostream>
#include <set>

#include "physicsEngine.hpp"

double gasSim::Particle::mass = 10;
double gasSim::Particle::radius = 0.1;

std::ostream &operator<<(std::ostream &os, const gasSim::PhysVectorD &vec) {
  os << "PhysVector(" << vec.x << ", " << vec.y << ", " << vec.z << ")   ";
  return os;
}

std::ostream &operator<<(std::ostream &os, const gasSim::Particle &part) {
  os << "Partcle(" << part.position << ", " << part.speed << ")\n";
  return os;
}
int main() {
  gasSim::Gas scureggione{3, 2500, 1E4};
  double t{0};
  double life{0};
  double media{0};
  for (int i{0}; i != 100; ++i) {
    scureggione.gasLoop(1);
    life = scureggione.getLife();
    std::cout << "Time: " << life << '\n';
    std::cout << "deltaT: " << life - t << '\n';
    t = life;
    auto petra{scureggione.getParticles()};
    std::cout << petra[0];
    std::cout << petra[1];
    std::cout << petra[2];
    std::cout << "\n\n\n";
    media += t;
  }
  media = media / 100;
  std::cout << "media: " << media << '\n';

  std::cout <<"\n\n\n\n";
  std::cout << static_cast<int>(gasSim::Wall::Front)<<'\n';
  std::cout << static_cast<int>(gasSim::Wall::Back)<<'\n';
  std::cout << static_cast<int>(gasSim::Wall::Left)<<'\n';
  std::cout << static_cast<int>(gasSim::Wall::Right)<<'\n';
  std::cout << static_cast<int>(gasSim::Wall::Top)<<'\n';
  std::cout << static_cast<int>(gasSim::Wall::Bottom)<<'\n';
}

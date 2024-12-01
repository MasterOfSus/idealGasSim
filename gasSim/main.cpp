#include <iostream>
#include <set>

#include "physicsEngine.hpp"

double gasSim::Particle::mass = 10;
double gasSim::Particle::radius = 0.1;

int main() {
  std::set<int> pluto{1, 2,0};
  std::set<int> paperino{2, 1, 0};

  double pippo{-2.11};
  double gino{0.12};

  if (pluto == paperino) {
    std::cout << "Sborro " << *pluto.begin() << *paperino.begin();

  } else {
    std::cout << "Non sborro";
  }
}
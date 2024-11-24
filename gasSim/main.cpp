#include "physicsEngine.hpp"

double gasSim::physics::Particle::mass = 10;
double gasSim::physics::Particle::radius = 0.1;

int main() {
  int numeroParticelle;
  double temp;
  double side;

  gasSim::physics::Gas Scurreggione{numeroParticelle, temp, side};

  while (true) {
    // Scurreggione.updateGasState(Collision fisrtCollision)
  }
}
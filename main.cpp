#include <iostream>

#include "namespacer.hpp"

namespace thermo {
// all class member functions

double PhysVector::get_module() { return std::sqrt(x * x + y * y + z * z); }
PhysVector::PhysVector(double Ix, double Iy, double Iz) : x{Ix}, y{Iy}, z{Iz} {}

Particle::Particle(PhysVector Ipos, PhysVector Ispeed)
      : position_{Ipos}, speed_{Ispeed} {}


thermo::Gas::Gas(int n) {
  std::default_random_engine eng(std::time(nullptr));
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  for (int i{0}; i != n; ++i) {
    particles_.push_back(
        {{dist(eng), dist(eng), dist(eng)}, {dist(eng), dist(eng), dist(eng)}});
  }
}
// end of member functions

}  // namespace thermo

int main() {
  // QUI INTERFACCIA UTENTE INSERIMENTO DATI
  thermo::PhysVector prova;
  std::cout << prova.get_module();
  thermo::Gas gas{10};  // Creazione del gas con particelle randomizzate

}
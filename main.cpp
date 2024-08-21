#include <iostream>
#include <random>

#include "namespacer.hpp"

namespace thermo {
// all class member functions

double PhysVector::get_module() { return std::sqrt(x * x + y * y + z * z); }
PhysVector::PhysVector(double Ix, double Iy, double Iz) : x{Ix}, y{Iy}, z{Iz} {}
PhysVector PhysVector::operator*(double factor) const {
  return PhysVector{factor * x, factor * y, factor * z};
}

Particle::Particle(PhysVector Ipos, PhysVector Ispeed)
    : position_{Ipos}, speed_{Ispeed} {}
PhysVector Particle::get_speed() { return speed_; };
PhysVector Particle::get_position() { return position_; };
// PhysVector Particle::get_momentum() { return (speed_ * mass_); };

Gas::Gas(int n) {
  std::default_random_engine eng(std::time(nullptr));
  std::uniform_real_distribution<double> dist(0.0, 20.0);
  for (int i{0}; i != n; ++i) {
    particles_.push_back(
        {{dist(eng), dist(eng), dist(eng)}, {dist(eng), dist(eng), dist(eng)}});
  }
}

double Gas::time_impact(std::vector<Particle>::iterator P1,
                        std::vector<Particle>::iterator P2) {
  double a{0};
  double b{0};
  double c{0};
  double result{10000};

  a = std::pow(((*P1).get_speed().x - (*P2).get_speed().x), 2);
  a = a + std::pow(((*P1).get_speed().y - (*P2).get_speed().y), 2);
  a = a + std::pow(((*P1).get_speed().z - (*P2).get_speed().z), 2);

  b = ((*P1).get_position().x - (*P2).get_position().x) *
      ((*P1).get_speed().x - (*P2).get_speed().x);
  b = b + ((*P1).get_position().y - (*P2).get_position().y) *
              ((*P1).get_speed().y - (*P2).get_speed().y);
  b = b + ((*P1).get_position().z - (*P2).get_position().z) *
              ((*P1).get_speed().z - (*P2).get_speed().z);

  c = std::pow(((*P1).get_position().x - (*P2).get_position().x), 2);
  c = c + std::pow(((*P1).get_position().y - (*P2).get_position().y), 2);
  c = c + std::pow(((*P1).get_position().z - (*P2).get_position().z), 2) -
      4 * pow((*P1).radius, 2);

  double delta{std::pow(b, 2) - a * c};

  if (delta > 0) {
    delta = sqrt(delta);
    double t1{(-b - delta) / a};
    double t2{(-b + delta) / a};
    if (t1 > 0) {
      result = t1;
    } else if (t2 > 0) {
      result = t2;
    }
  }

  return result;
}

double Gas::find_iteration() {
  double shortest{1000};
  double time{0};

  for (auto it = particles_.begin(), last = particles_.end(); it != last;
       ++it) {
    for (auto it2 = particles_.begin(), last2 = particles_.end(); it2 != last2;
         ++it2) {
      time = time_impact(it, it2);
      if (time < shortest) {
        shortest = time;
        std::cout << shortest << '\n';
      }
    }
  }
  return shortest;
}
// end of member functions

}  // namespace thermo

int main() {
  // QUI INTERFACCIA UTENTE INSERIMENTO DATI
  
  thermo::Gas gas{10};  // Creazione del gas con particelle randomizzate
  gas.find_iteration();
}
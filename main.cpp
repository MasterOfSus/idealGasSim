#include <iostream>
#include <random>

#include "namespacer.hpp"

namespace thermo {

double calculateA(std::vector<Particle>::iterator const P1,
                  std::vector<Particle>::iterator const P2) {
  double a;
  a = std::pow(((*P1).speed.x - (*P2).speed.x), 2) +
      std::pow(((*P1).speed.y - (*P2).speed.y), 2) +
      std::pow(((*P1).speed.z - (*P2).speed.z), 2);
  return a;
}

double calculateB(std::vector<Particle>::iterator const P1,
                  std::vector<Particle>::iterator const P2) {
  double b;
  b = ((*P1).position.x - (*P2).position.x) * ((*P1).speed.x - (*P2).speed.x) +
      ((*P1).position.y - (*P2).position.y) * ((*P1).speed.y - (*P2).speed.y) +
      ((*P1).position.z - (*P2).position.z) * ((*P1).speed.z - (*P2).speed.z);
  return b;
}
double calculateC(std::vector<Particle>::iterator const P1,
                  std::vector<Particle>::iterator const P2) {
  double c;
  c = std::pow(((*P1).position.x - (*P2).position.x), 2) +
      std::pow(((*P1).position.y - (*P2).position.y), 2) +
      std::pow(((*P1).position.z - (*P2).position.z), 2) -
      4 * pow((*P1).radius, 2);
  return c;
}

// all class member functions

double PhysVector::get_module() { return std::sqrt(x * x + y * y + z * z); }
PhysVector::PhysVector(double Ix, double Iy, double Iz) : x{Ix}, y{Iy}, z{Iz} {}
PhysVector PhysVector::operator*(double factor) const {
  return PhysVector{factor * x, factor * y, factor * z};
}

double PhysVector::operator*(const PhysVector& vector) const {
	return x*vector.x + y*vector.y + z*vector.z;
}

Particle::Particle(PhysVector Ipos, PhysVector Ispeed)
    : position{Ipos}, speed{Ispeed} {}
// PhysVector Particle::get_momentum() { return (speed_ * mass_); };

Box::Box(double Iside) : side{Iside} {}

Collision::Collision(double Itime,
                     std::vector<Particle>::iterator IfirstPaticle,
                     char IwallCollision)
    : time{Itime},
      firstPaticle{IfirstPaticle},
      wallCollision{IwallCollision},
      flag{'w'} {}
Collision::Collision(double Itime,
                     std::vector<Particle>::iterator IfirstPaticle,
                     std::vector<Particle>::iterator IsecondPaticle)
    : time{Itime},
      firstPaticle{IfirstPaticle},
      secondPaticle{IsecondPaticle},
      flag{'p'} {}

Gas::Gas(int n, double l) : box_{l} {
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

  a = calculateA(P1, P2);
  b = calculateB(P1, P2);
  c = calculateC(P1, P2);

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

double Gas::time_impact(std::vector<Particle>::iterator P1, char side) {
  double t;
  switch (side) {
    case 'x':
      if ((*P1).speed.x < 0) {
        t = -(*P1).position.x / (*P1).speed.x;
      } else {
        t = (box_.side - (2 * (*P1).radius) - (*P1).position.x) / (*P1).speed.x;
      }
      break;
    case 'y':
      if ((*P1).speed.y < 0) {
        t = -(*P1).position.y / (*P1).speed.y;
      } else {
        t = (box_.side - (2 * (*P1).radius) - (*P1).position.y) / (*P1).speed.y;
      }
      break;
    case 'z':
      if ((*P1).speed.z < 0) {
        t = -(*P1).position.z / (*P1).speed.z;
      } else {
        t = (box_.side - (2 * (*P1).radius) - (*P1).position.z) / (*P1).speed.z;
      }
      break;
  }
  return t;
}

Collision Gas::find_iteration() {
  double shortestP{1000};
  double shortestW{1000};
  double timeP{0};
  std::vector<Particle>::iterator firstP1;
  std::vector<Particle>::iterator firstP2;
  char firstW;
  double timeW{0};

  int nWall{0};

  for (auto it = particles_.begin(), last = particles_.end(); it != last;
       ++it) {
    std::cout << "ciclo grosso \n";
    for (auto it2{it}, last2 = particles_.end(); it2 != last2; ++it2) {
      timeP = time_impact(it, it2);
      if (timeP < shortestP) {
        shortestP = timeP;
        std::cout << shortestP << '\n';
      }
    }

    for (char w : {'x', 'y', 'z'}) {
      timeW = time_impact(it, w);
      if (timeW < shortestW) {
        shortestW = timeW;
        firstP1 = it;
        firstW = w;
        std::cout << shortestW << '\n';
      }
    }
  }

  if (shortestP < shortestW) {
    return {shortestP, firstP1, firstP2};
  } else {
    return {shortestW, firstP1, firstW};
  }
}
// end of member functions

}  // namespace thermo

int main() {
  // QUI INTERFACCIA UTENTE INSERIMENTO DATI

  thermo::Gas gas{10, 200};  // Creazione del gas con particelle randomizzate
  auto l = gas.find_iteration();
  std::cout << "la prima particella si è schiantata contro un " << l.flag
            << " è stat la particella in posizione x: "
            << (*l.firstPaticle).position.x << "\n dopo un tempo " << l.time;
}

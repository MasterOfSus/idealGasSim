#include <iostream>
#include <random>

#include "namespacer.hpp"

namespace thermo {

// static variables initalization

double Particle::mass_{1.66 * 4};
double Particle::radius_{0.143};

// a set of three auxiliary functions returning parameters for a reduced
// quadratic formula
double get_a(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return std::pow(((*P1).speed_.x_ - (*P2).speed_.x_), 2) +
         std::pow(((*P1).speed_.y_ - (*P2).speed_.y_), 2) +
         std::pow(((*P1).speed_.z_ - (*P2).speed_.z_), 2);
}

double get_b(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return ((*P1).position_.x_ - (*P2).position_.x_) *
             ((*P1).speed_.x_ - (*P2).speed_.x_) +
         ((*P1).position_.y_ - (*P2).position_.y_) *
             ((*P1).speed_.y_ - (*P2).speed_.y_) +
         ((*P1).position_.z_ - (*P2).position_.z_) *
             ((*P1).speed_.z_ - (*P2).speed_.z_);
}

double get_c(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return std::pow(((*P1).position_.x_ - (*P2).position_.x_), 2) +
         std::pow(((*P1).position_.y_ - (*P2).position_.y_), 2) +
         std::pow(((*P1).position_.z_ - (*P2).position_.z_), 2) -
         4 * pow((*P1).radius_, 2);
}

PhysVector grid_vector(int nI, int tot, double side) {
  static int elementPerSide = (std::ceil(cbrt(tot)));
  std::cout << "side: " << side << '\n';
  std::cout << "nI: " << nI << '\n';
  std::cout << "tot: " << tot << '\n';

  static double particleDistance = side / elementPerSide;
  // aggiungere accert che controlla che le particelle non si compenetrino
  std::cout << "particleDistance: " << particleDistance << '\n';

  int x {nI % elementPerSide};
  int y {(nI / elementPerSide) % elementPerSide};
  int z {nI / (elementPerSide * elementPerSide)};

  std::cout << x << ' ' << y << ' ' << z << "\n\n\n";

  return {x * particleDistance, y * particleDistance, z * particleDistance};
}

PhysVector random_vector(double max) {
  static std::default_random_engine eng(std::time(nullptr));
  static std::uniform_real_distribution<double> dist(0.0, max);
  return {dist(eng), dist(eng), dist(eng)};
}

// all class member functions

// physvector implementation
double norm(const PhysVector& v) {
  return std::sqrt(v.x_ * v.x_ + v.y_ * v.y_ + v.z_ * v.z_);
}

PhysVector operator*(const PhysVector& v, double factor) {
  return {factor * v.x_, factor * v.y_, factor * v.z_};
}

PhysVector operator/(const PhysVector& v, double factor) {
  return v * (1 / factor);
}

double operator*(const PhysVector& v1, const PhysVector& v2) {
  return v1.x_ * v2.x_ + v1.y_ * v2.y_ + v1.z_ * v2.z_;
}

PhysVector operator+(const PhysVector& v1, const PhysVector& v2) {
  return {v1.x_ + v2.x_, v1.y_ + v2.y_, v1.z_ + v2.z_};
}

PhysVector operator-(const PhysVector& v1, const PhysVector& v2) {
  return {v1.x_ - v2.x_, v1.y_ - v2.y_, v1.z_ - v2.z_};
}

void PhysVector::operator+=(const PhysVector& v) {
  x_ += v.x_;
  y_ += v.y_;
  z_ += v.z_;
}

// gas implementation
Gas::Gas(const Gas& gas)
    : particles_(gas.particles_.begin(), gas.particles_.end()),
      side_{gas.side_} {}

Gas::Gas(int n, double l, double temperature) : side_{l} {
  double velMax{sqrt(2 * temperature / (3 * Particle::mass_))};

  for (int i{0}; i != n; ++i) {
    particles_.emplace_back(
        Particle{grid_vector(i, n, side_), random_vector(velMax)});
  }
}

double collision_time(const std::vector<Particle>::iterator P1,
                      const std::vector<Particle>::iterator P2) {
  double a{get_a(P1, P2)};
  double b{get_b(P1, P2)};
  double c{get_c(P1, P2)};
  double result{10000.};

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

double collision_time(const std::vector<Particle>::iterator P1, const Wall wall,
                      double side) {
  double t;
  switch (wall.wall_type_) {
    case 'x':
      if ((*P1).speed_.x_ < 0) {
        t = -(*P1).position_.x_ / (*P1).speed_.x_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.x_) / (*P1).speed_.x_;
      }
      break;
    case 'y':
      if ((*P1).speed_.y_ < 0) {
        t = -(*P1).position_.y_ / (*P1).speed_.y_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.y_) / (*P1).speed_.y_;
      }
      break;
    case 'z':
      if ((*P1).speed_.z_ < 0) {
        t = -(*P1).position_.z_ / (*P1).speed_.z_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.z_) / (*P1).speed_.z_;
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
  Wall firstW;
  double timeW{0};

  for (auto it = particles_.begin(), last = particles_.end(); it != last;
       ++it) {
    for (auto it2{it}, last2 = particles_.end(); it2 != last2; ++it2) {
      timeP = collision_time(it, it2);
      if (timeP < shortestP && timeP > 0) {
        shortestP = timeP;
      }
    }

    std::vector<Wall> walls{{'x'}, {'y'}, {'z'}};

    for (Wall wall : walls) {
      timeW = collision_time(it, wall, side_);
      if (timeW < shortestW && timeW > 0) {
        shortestW = timeW;
        firstP1 = it;
        firstW = wall;
        std::cout << shortestW << '\n';
      }
    }
  }

  if (shortestP < shortestW) {
    return {shortestP, {firstP1, firstP2}, {}};
  } else {
    return {shortestW, {firstP1}, {firstW}};
  }
}

void Gas::update_gas_state(Collision collision) {
  double time{collision.time_};
  if (collision.particles_.size() + collision.walls_.size() > 2) {
    std::cout << "Urto a più di 2";
  } else if (collision.particles_.size() == 2) {
    auto P1 = collision.particles_[0];
    auto P2 = collision.particles_[1];

    // Aggiorna le posizioni dei particelle
    P1->position_ += P1->speed_ * time;
    P2->position_ += P2->speed_ * time;

    PhysVector delta_position{P1->position_ - P2->position_};

    PhysVector n{delta_position / norm(delta_position)};
    (*P1).speed_ += n * (n * ((*P2).speed_ - (*P1).speed_));
    (*P2).speed_ += n * (n * ((*P1).speed_ - (*P2).speed_));
  }
}

// end of member functions

}  // namespace thermo

int main() {
  // QUI INTERFACCIA UTENTE INSERIMENTO DATI

  thermo::Gas gas{100, 200.,
                  3.};  // Creazione del gas con particelle randomizzate
  auto l = gas.find_iteration();
  std::cout << "Urto al tempo " << l.time_ << ":\n"
            << "posizioni: \n";
  for (auto it : l.particles_) {
    std::cout << (*it).position_.x_ << ", " << (*it).position_.y_ << ", "
              << (*it).position_.z_ << "\n";
  }
  std::cout << "muri: \n";
  for (auto wall : l.walls_) {
    std::cout << wall.wall_type_ << "\n";
  }
}

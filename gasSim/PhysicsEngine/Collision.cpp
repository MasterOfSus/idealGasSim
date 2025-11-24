#include "Collision.hpp"

#include <stdexcept>

namespace GS {

Collision::Collision(double time, Particle* p1) : p1{p1}, time{time} {
  if (time < 0.) {
    throw std::invalid_argument("Provided negative time");
  }
}

Collision::~Collision() = default;

PWCollision::PWCollision(double time, Particle* p1, Wall wall)
    : Collision(time, p1), wall{wall} {}

void PWCollision::solve() {
  Particle* p{getP1()};
  switch (wall) {
    case Wall::Left:
    case Wall::Right:
      p->speed.x = -p->speed.x;
    case Wall::Front:
    case Wall::Back:
      p->speed.y = -p->speed.y;
    case Wall::Top:
    case Wall::Bottom:
      p->speed.z = -p->speed.z;
    default:
      throw std::logic_error("Provided unknown wall type.");
  }
}

PPCollision::PPCollision(double time, Particle* p1, Particle* p2)
    : Collision(time, p1), p2{p2} {}

void PPCollision::solve() {
  Particle* p1{getP1()};
  Vector3d d{p1->position - p2->position};
  d.normalize();
  Vector3d v{p1->speed - p2->speed};
  double proj = d * v;

  p1->speed -= d * proj;
  p2->speed += d * proj;
}

}  // namespace GS

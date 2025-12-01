#include "Collision.hpp"

#include <stdexcept>

namespace GS {

Collision::Collision(double time, Particle* p1) : p1{p1}, time{time} {
  if (time < 0.) {
    throw std::invalid_argument(
        "Collision constructor error: provided negative time");
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
      break;
    case Wall::Front:
    case Wall::Back:
      p->speed.y = -p->speed.y;
      break;
    case Wall::Top:
    case Wall::Bottom:
      p->speed.z = -p->speed.z;
      break;
    default:
      throw std::logic_error(
          "PWCollision constructor error: provided invalid wall type.");
  }
}

PPCollision::PPCollision(double time, Particle* p1, Particle* p2)
    : Collision(time, p1), p2{p2} {}

void PPCollision::solve() {
  Particle* p1{getP1()};
  GSVectorD d{p1->position - p2->position};
  d.normalize();
  GSVectorD v{p1->speed - p2->speed};
  double proj = d * v;

  p1->speed -= d * proj;
  p2->speed += d * proj;
}

}  // namespace GS

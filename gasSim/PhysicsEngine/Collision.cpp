#include "Collision.hpp"

#include "PhysicsEngine/GSVector.hpp"
#include "PhysicsEngine/Particle.hpp"

#include <stdexcept>

namespace GS {

Collision::Collision(double timeV, Particle* p1p) : p1{p1p}, time{timeV} {
  if (time < 0.) {
    throw std::invalid_argument(
        "Collision constructor error: provided negative time");
  }
}

PWCollision::PWCollision(double timeV, Particle* p1p, Wall wallV)
    : Collision(timeV, p1p), wall{wallV} {}

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

PPCollision::PPCollision(double timeV, Particle* p1p, Particle* p2p)
    : Collision(timeV, p1p), p2{p2p} {}

void PPCollision::solve() {
  Particle* p1p{getP1()};
  GSVectorD d{p1p->position - p2->position};
  d.normalize();
  GSVectorD v{p1p->speed - p2->speed};
  double proj = d * v;

  p1p->speed -= d * proj;
  p2->speed += d * proj;
}

}  // namespace GS

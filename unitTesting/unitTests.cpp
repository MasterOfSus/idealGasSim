#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../gasSim/graphics.hpp"
#include "doctest.h"
// #include "input.hpp"
#include "../gasSim/algorithms.hpp"
#include "../gasSim/output.hpp"
#include "../gasSim/physicsEngine.hpp"

double gasSim::Particle::mass = 10;
double gasSim::Particle::radius = 0.1;

TEST_CASE("Testing algorithms::for_any_couple") {
  std::vector<int> num{1, 2, 3, 4};
  int i{0};
  gasSim::for_each_couple(num.begin(), num.end(), [&i](int a, int b) {
    std::cout << " " << a << " " << b << "\n";
    ++i;
  });
  CHECK(i == 6);
}

TEST_CASE("Testing PhysVector") {
  gasSim::PhysVector vec1{1.1, -2.11, 56.253};
  gasSim::PhysVector vec2{12.3, 0.12, -6.3};
  // gasSim::PhysVector weirdFloatVector{-5.5f, 100000.f, 4.3f};
  // gasSim::PhysVector weirdIntVector{-5, 6, 3};
  gasSim::PhysVector randVec{gasSim::randomVector(5)};
  // gasSim::PhysVector randomvec2(67.);
  std::string expectedMemberType = "double";
  SUBCASE("Constructor") {
    CHECK(vec1.x == 1.1);
    CHECK(vec1.y == -2.11);
    CHECK(vec1.z == 56.253);
    CHECK(vec2.x == 12.3);
    CHECK(vec2.y == 0.12);
    CHECK(vec2.z == -6.3);
  }
  SUBCASE("Random constructor") { CHECK(randVec.norm() <= 5); }
  SUBCASE("Sum") {
    gasSim::PhysVector sum{vec1 + vec2};
    gasSim::PhysVector actualSum{13.4, -1.99, 49.953};
    CHECK(doctest::Approx(sum.x) == actualSum.x);
    CHECK(doctest::Approx(sum.y) == actualSum.y);
    CHECK(doctest::Approx(sum.z) == actualSum.z);
  }
  SUBCASE("Subtraction") {
    gasSim::PhysVector subtraction{vec2 - vec1};
    gasSim::PhysVector actualSubtraction{11.2, 2.23, -62.553};
    CHECK(doctest::Approx(subtraction.x) == actualSubtraction.x);
    CHECK(doctest::Approx(subtraction.y) == actualSubtraction.y);
    CHECK(doctest::Approx(subtraction.z) == actualSubtraction.z);
  }
  SUBCASE("Product") {
    double l{0.25};
    gasSim::PhysVector product1{vec1 * l};
    gasSim::PhysVector product2{l * vec1};
    gasSim::PhysVector actualProduct{0.275, -0.5275, 14.06325};

    CHECK(doctest::Approx(product1.x) == actualProduct.x);
    CHECK(doctest::Approx(product1.y) == actualProduct.y);
    CHECK(doctest::Approx(product1.z) == actualProduct.z);
    CHECK(product1 == product2);
  }
  SUBCASE("Division") {
    double l{10};
    gasSim::PhysVector division{vec1 / l};
    gasSim::PhysVector actualDivision{0.11, -0.211, 5.6253};
    CHECK(doctest::Approx(division.x) == actualDivision.x);
    CHECK(doctest::Approx(division.y) == actualDivision.y);
    CHECK(doctest::Approx(division.z) == actualDivision.z);
  }
  SUBCASE("Scalar Product") {
    double scalarProduct1{vec1 * vec2};
    double scalarProduct2{vec2 * vec1};
    double actualProduct{-341.1171};

    CHECK(scalarProduct1 == actualProduct);
    CHECK(scalarProduct1 == scalarProduct2);
  }
  SUBCASE("Division by zero") {
    double l{0};
    gasSim::PhysVector division{vec1 / l};
    gasSim::PhysVector actualDivision{INFINITY, -INFINITY, INFINITY};

    CHECK(division == actualDivision);
  }
  SUBCASE("Norm") { CHECK(vec1.norm() == doctest::Approx(56.3033)); }
}

TEST_CASE("Testing Collision") {
  double time{4};

  gasSim::PhysVector vec1{4.23, 5.34, 6.45};
  gasSim::PhysVector vec2{5.46, 4.35, 3.24};
  gasSim::Particle part1{vec1, vec2};

  gasSim::PhysVector vec3{13.4, -1.99, 49.953};
  gasSim::PhysVector vec4{0.11, -0.211, 5.6253};
  gasSim::Particle part2{vec3, vec4};

  gasSim::ParticleCollision coll{time, &part1, &part2};
  SUBCASE("Constructor Particle2Particle") {
    CHECK(coll.getFirstParticle()->position == vec1);
    CHECK(coll.getFirstParticle()->speed == vec2);
    CHECK(coll.getSecondParticle()->position == vec3);
    CHECK(coll.getSecondParticle()->speed == vec4);
  }
  SUBCASE("Change the particles") {
    gasSim::PhysVector actualPosition{4, 0, 4};
    gasSim::PhysVector actualSpeed{1.34, 0.04, 9.3924};
    coll.getFirstParticle()->position.x = 4;
    coll.getFirstParticle()->position.y = 0;
    coll.getFirstParticle()->position.z = 4;
    coll.getSecondParticle()->speed = actualSpeed;
    CHECK(coll.getFirstParticle()->position == actualPosition);
    CHECK(coll.getSecondParticle()->speed == actualSpeed);
  }
}
TEST_CASE("Testing collisionTime") {
  gasSim::Particle part1{{0, 0, 0}, {1, 1, 1}};
  gasSim::Particle part2{{0, 0, 1}, {1, 1, -1}};

  std::cout << "\n\n\n\n" << gasSim::collisionTime(part1, part2) << "\n\n\n\n";
}
TEST_CASE("Testing Collision") {
  double time{4};
  char wall{'a'};

  gasSim::PhysVector vec1{4.23, 5.34, 6.45};
  gasSim::PhysVector vec2{5.46, 4.35, 3.24};
  gasSim::Particle part1{vec1, vec2};

  gasSim::WallCollision Collision{time, &part1, wall};
  SUBCASE("Constructor Wall2Particle") {
    CHECK(Collision.getFirstParticle()->position == vec1);
    CHECK(Collision.getFirstParticle()->speed == vec2);
    CHECK(Collision.getWall() == wall);
  }
  SUBCASE("Change the particles") {
    gasSim::PhysVector actualPosition{4, 0, 4};
    Collision.getFirstParticle()->position.x = 4;
    Collision.getFirstParticle()->position.y = 0;
    Collision.getFirstParticle()->position.z = 4;
    CHECK(Collision.getFirstParticle()->position == actualPosition);
  }
  SUBCASE("Constructor Particle2Wall") {
    // Appena abbiamo deciso cos'è un muro
  }
}
TEST_CASE("Testing Gas") {
  double side{9.};
  double temp{1.};
  int partNum{100};

  gasSim::Gas Gas{partNum, temp, side};
  SUBCASE("Speed check") {
    double maxSpeed = 4. / 3. * temp;
    auto firstIt = Gas.getParticles().begin(),
         lastIt = Gas.getParticles().end();

    std::for_each(firstIt, lastIt, [=](const gasSim::Particle& p) {
      CHECK(p.speed.norm() <= maxSpeed);
    });
  }
  SUBCASE("Constructor from gas") {
    gasSim::Gas copyGas{Gas};
    CHECK(copyGas.getBoxSide() == side);
    CHECK(copyGas.getParticles().size() == partNum);
  }
  SUBCASE("Find first Particle Collsion") {
    Gas.findFirstPartCollision(INFINITY);
  }
  SUBCASE("Resolve Collision") { Gas.gasLoop(1); }
}
TEST_CASE("Testing Gas 2") {
  // gasSim::randomVector(-1);
  /*
  gasSim::Particle p1{{0,0,0}, {1,1,1}};
  gasSim::Particle p2{{0,0,0.09}, {5,6,7}};
  std::vector<gasSim::Particle> badParticles{p1,p2};
  gasSim::Gas badGas{badParticles, 5}; */
}

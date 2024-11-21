#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../gasSim/graphics.hpp"
#include "doctest.h"
// #include "input.hpp"
#include "../gasSim/output.hpp"
#include "../gasSim/physicsEngine.hpp"

double gasSim::physics::Particle::mass = 10;
double gasSim::physics::Particle::radius = 0.1;

TEST_CASE("Testing physics::PhysVector") {
  gasSim::physics::PhysVector goodVector1{1.1, -2.11, 56.253};
  gasSim::physics::PhysVector goodVector2{12.3, 0.12, -6.3};
  // gasSim::physics::PhysVector weirdFloatVector{-5.5f, 100000.f, 4.3f};
  // gasSim::physics::PhysVector weirdIntVector{-5, 6, 3};
  gasSim::physics::PhysVector randomVector1{gasSim::physics::randomVector(5)};
  // gasSim::physics::PhysVector randomVector2(67.);
  std::string expectedMemberType = "double";
  SUBCASE("Constructor") {
    CHECK(goodVector1.x == 1.1);
    CHECK(goodVector1.y == -2.11);
    CHECK(goodVector1.z == 56.253);
    CHECK(goodVector2.x == 12.3);
    CHECK(goodVector2.y == 0.12);
    CHECK(goodVector2.z == -6.3);
  }
  SUBCASE("Random constructor") { CHECK(randomVector1.norm() <= 5); }
  SUBCASE("Sum") {
    gasSim::physics::PhysVector sum{goodVector1 + goodVector2};
    gasSim::physics::PhysVector actualSum{13.4, -1.99, 49.953};
    CHECK(doctest::Approx(sum.x) == actualSum.x);
    CHECK(doctest::Approx(sum.y) == actualSum.y);
    CHECK(doctest::Approx(sum.z) == actualSum.z);
  }
  SUBCASE("Subtraction") {
    gasSim::physics::PhysVector subtraction{goodVector2 - goodVector1};
    gasSim::physics::PhysVector actualSubtraction{11.2, 2.23, -62.553};
    CHECK(doctest::Approx(subtraction.x) == actualSubtraction.x);
    CHECK(doctest::Approx(subtraction.y) == actualSubtraction.y);
    CHECK(doctest::Approx(subtraction.z) == actualSubtraction.z);
  }
  SUBCASE("Product") {
    double l{0.25};
    gasSim::physics::PhysVector product{goodVector1 * l};
    gasSim::physics::PhysVector product2{l * goodVector1};
    gasSim::physics::PhysVector actualProduct{0.275, -0.5275, 14.06325};

    CHECK(doctest::Approx(product.x) == actualProduct.x);
    CHECK(doctest::Approx(product.y) == actualProduct.y);
    CHECK(doctest::Approx(product.z) == actualProduct.z);
    CHECK(product == product2);
  }
  SUBCASE("Division") {
    double l{10};
    gasSim::physics::PhysVector product{goodVector1 / l};
    gasSim::physics::PhysVector actualProduct{0.11, -0.211, 5.6253};
    CHECK(doctest::Approx(product.x) == actualProduct.x);
    CHECK(doctest::Approx(product.y) == actualProduct.y);
    CHECK(doctest::Approx(product.z) == actualProduct.z);
  }
  SUBCASE("Scalar Product") {
    double scalarProduct1{goodVector1 * goodVector2};
    double scalarProduct2{goodVector2 * goodVector1};
    double actualProduct{-341.1171};

    CHECK(scalarProduct1 == actualProduct);
    CHECK(scalarProduct1 == scalarProduct2);
  }
  /*
SUBCASE("Division by zero") {
double scalar{0};
gasSim::physics::PhysVector test{INFINITY, INFINITY, INFINITY};
CHECK(v1 / scalar == test);
}
  */
  SUBCASE("Norm") { CHECK(goodVector1.norm() == doctest::Approx(56.3033)); }
}

TEST_CASE("Testing physics::Collision") {
  double goodTime{4};

  gasSim::physics::PhysVector goodVector1{4.23, 5.34, 6.45};
  gasSim::physics::PhysVector goodVector2{5.46, 4.35, 3.24};
  gasSim::physics::Particle goodFirstParticle{goodVector1, goodVector2};

  gasSim::physics::PhysVector goodVector3{13.4, -1.99, 49.953};
  gasSim::physics::PhysVector goodVector4{0.11, -0.211, 5.6253};
  gasSim::physics::Particle goodSecondParticle{goodVector3, goodVector4};

  gasSim::physics::ParticleCollision goodCollision{goodTime, &goodFirstParticle,
                                                   &goodSecondParticle};
  SUBCASE("Constructor Particle2Particle") {
    CHECK(goodCollision.getFirstParticle()->position == goodVector1);
    CHECK(goodCollision.getFirstParticle()->speed == goodVector2);
    CHECK(goodCollision.getSecondParticle()->position == goodVector3);
    CHECK(goodCollision.getSecondParticle()->speed == goodVector4);
  }
  SUBCASE("Change the particles") {
    gasSim::physics::PhysVector actualPosition{4, 0, 4};
    gasSim::physics::PhysVector actualSpeed{1.34, 0.04, 9.3924};
    goodCollision.getFirstParticle()->position.x = 4;
    goodCollision.getFirstParticle()->position.y = 0;
    goodCollision.getFirstParticle()->position.z = 4;
    goodCollision.getSecondParticle()->speed = actualSpeed;
    CHECK(goodCollision.getFirstParticle()->position == actualPosition);
    CHECK(goodCollision.getSecondParticle()->speed == actualSpeed);
  }
}
TEST_CASE("Testing physics::collisionTime") {
  gasSim::physics::Particle goodParticle1{{0, 0, 0}, {1, 1, 1}};
  gasSim::physics::Particle goodParticle2{{0, 0, 1}, {1, 1, -1}};

  std::cout << "\n\n\n\n"
            << gasSim::physics::collisionTime(goodParticle1, goodParticle2)
            << "\n\n\n\n";
}
TEST_CASE("Testing physics::Collision") {
  double goodTime{4};
  char goodWall{'a'};

  gasSim::physics::PhysVector goodVector1{4.23, 5.34, 6.45};
  gasSim::physics::PhysVector goodVector2{5.46, 4.35, 3.24};
  gasSim::physics::Particle goodFirstParticle{goodVector1, goodVector2};

  gasSim::physics::WallCollision goodCollision{goodTime, &goodFirstParticle,
                                               goodWall};
  SUBCASE("Constructor Wall2Particle") {
    CHECK(goodCollision.getFirstParticle()->position == goodVector1);
    CHECK(goodCollision.getFirstParticle()->speed == goodVector2);
    CHECK(goodCollision.getWall() == goodWall);
  }
  SUBCASE("Change the particles") {
    gasSim::physics::PhysVector actualPosition{4, 0, 4};
    goodCollision.getFirstParticle()->position.x = 4;
    goodCollision.getFirstParticle()->position.y = 0;
    goodCollision.getFirstParticle()->position.z = 4;
    CHECK(goodCollision.getFirstParticle()->position == actualPosition);
  }
  SUBCASE("Constructor Particle2Wall") {
    // Appena abbiamo deciso cos'è un muro
  }
}
TEST_CASE("Testing physics::Gas") {
  double goodSide{9.};
  double goodTemperature{1.};
  int goodNumber{100};

  gasSim::physics::Gas goodGas{goodNumber, goodTemperature, goodSide};
  SUBCASE("Speed check") {
    double maxSpeed = 4. / 3. * goodTemperature;
    auto firstIt = goodGas.getParticles().begin(),
         lastIt = goodGas.getParticles().end();

    std::for_each(firstIt, lastIt, [=](const gasSim::physics::Particle& p) {
      CHECK(p.speed.norm() <= maxSpeed);
    });
  }
  SUBCASE("Constructor from gas") {
    gasSim::physics::Gas copyGas{goodGas};
    CHECK(copyGas.getBoxSide() == goodSide);
    CHECK(copyGas.getParticles().size() == goodNumber);
  }
  SUBCASE("Find first Particle Collsion") {
    goodGas.findFirstPartCollision(INFINITY);
  }
  SUBCASE("Resolve Collision"){
    goodGas.gasLoop(1);
  }
}

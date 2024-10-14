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
double gasSim::physics::Particle::radius = 1;

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
    gasSim::physics::PhysVector product2{l*goodVector1};
    gasSim::physics::PhysVector actualProduct{0.275, -0.5275, 14.06325};

    CHECK(doctest::Approx(product.x) == actualProduct.x);
    CHECK(doctest::Approx(product.y) == actualProduct.y);
    CHECK(doctest::Approx(product.z) == actualProduct.z);
    CHECK(product==product2);
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
TEST_CASE("Testing physics::Particle") {
  gasSim::physics::PhysVector goodPosition{1, 1, 1}, goodSpeed{2, 2, 2};
  gasSim::physics::Particle goodParticle{goodPosition, goodSpeed};
  SUBCASE("Equality") {
    gasSim::physics::Particle copyParticle{goodPosition, goodSpeed};
    CHECK(goodParticle == copyParticle);
  }
}
TEST_CASE("Testing physics::Gas") {
  double goodSide{9.};
  double goodTemperature{1.};
  int goodNumber{10};

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
}

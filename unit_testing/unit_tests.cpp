#include <cmath>
#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../gas_sim/graphics.hpp"
#include "doctest.h"
// #include "input.hpp"
#include "../gas_sim/output.hpp"
#include "../gas_sim/physics_engine.hpp"

TEST_CASE("Testing physics::vector") {
  gasSim::physics::vector goodVector1{1.1, -2.11, 56.253};
  gasSim::physics::vector goodVector2{12.3, 0.12, -6.3};
  // gasSim::physics::vector weirdFloatVector{-5.5f, 100000.f, 4.3f};
  // gasSim::physics::vector weirdIntVector{-5, 6, 3};
  gasSim::physics::vector randomVector1{gasSim::physics::randomVector(5)};
  // gasSim::physics::vector randomVector2(67.);
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
    gasSim::physics::vector sum{goodVector1 + goodVector2};
    gasSim::physics::vector actualSum{13.4, -1.99, 49.953};
    CHECK((doctest::Approx(sum.x) == actualSum.x &&
           doctest::Approx(sum.y) == actualSum.y &&
           doctest::Approx(sum.z) == actualSum.z));
  }
  SUBCASE("Subtraction") {
    gasSim::physics::vector subtraction{goodVector2 - goodVector1};
    gasSim::physics::vector actualSubtraction{11.2, 2.23, -62.553};
    CHECK((doctest::Approx(subtraction.x) == actualSubtraction.x &&
           doctest::Approx(subtraction.y) == actualSubtraction.y &&
           doctest::Approx(subtraction.z) == actualSubtraction.z));
  }
  SUBCASE("Product") {
    double l{0.25};
    gasSim::physics::vector product{goodVector1 * l};
    gasSim::physics::vector actualProduct{0.275, -0.5275, 14.06325};
    CHECK((doctest::Approx(product.x) == actualProduct.x &&
           doctest::Approx(product.y) == actualProduct.y &&
           doctest::Approx(product.z) == actualProduct.z));
  }
  SUBCASE("Division") {
    double l{10};
    gasSim::physics::vector product{goodVector1 / l};
    gasSim::physics::vector actualProduct{0.11, -0.211, 5.6253};
    CHECK((doctest::Approx(product.x) == actualProduct.x &&
           doctest::Approx(product.y) == actualProduct.y &&
           doctest::Approx(product.z) == actualProduct.z));
  }
  /*
SUBCASE("Division by zero") {
double scalar{0};
gasSim::physics::vector test{INFINITY, INFINITY, INFINITY};
CHECK(v1 / scalar == test);
}
  */
  SUBCASE("Norm") { CHECK(goodVector1.norm() == doctest::Approx(56.3033)); }
}

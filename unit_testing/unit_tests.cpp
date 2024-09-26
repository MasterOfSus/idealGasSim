#include <cmath>
#include <iostream>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "doctest.h"

#include "../gas_sim/graphics.hpp"
// #include "input.hpp"
#include "../gas_sim/output.hpp"
#include "../gas_sim/physics_engine.hpp"

TEST_CASE("Testing physics.vector")
{
  gasSim::physics::vector v1{1, 2, 3};
  gasSim::physics::vector v2{0, 0, -1};

  SUBCASE("Sum")
  {
    gasSim::physics::vector sum{v1 + v2};
    gasSim::physics::vector test{1, 2, 2};
    CHECK(sum == test);
  }
  SUBCASE("Subtraction")
  {
    gasSim::physics::vector sub{v2 - v1};
    gasSim::physics::vector test{-1, -2, -4};
    CHECK(sub == test);
  }
  SUBCASE("Product")
  {
    double scalar{0.25};
    gasSim::physics::vector test{0.25, 0.5, 0.75};
    CHECK(v1 * scalar == test);
  }
  SUBCASE("Division")
  {
    double scalar{10};
    gasSim::physics::vector test{0.1, 0.2, 0.3};
    CHECK(v1 / scalar == test);
  }
  SUBCASE("Division by zero")
  {
    double scalar{0};
    gasSim::physics::vector test{INFINITY, INFINITY, INFINITY};
    CHECK(v1 / scalar == test);
  }
  SUBCASE("Norm")
  {
    CHECK(v1.norm() == doctest::Approx(3.74166));
  }
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <namespacer.hpp>

#include "doctest.h"

TEST_CASE("Testing the functions for the reduced quadratic formula") {}

TEST_CASE(
    "Testing the struct PhysVector operators and loose functions in which is "
    "passed") {
  PhysVector a;

  REQUIRE(pVect.size() == 0);

  SUBCASE("Testing operator * between vectors") {}
  SUBCASE("Testing operator * between vector and scalar") {}
  SUBCASE("Testing function norm()") { CHECK(a{} ==); }
}
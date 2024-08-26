#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <namespacer.hpp>

#include "cmath"
#include "doctest.h"

TEST_CASE("Testing the functions for the reduced quadratic formula") {}

TEST_CASE(
    "Testing the struct PhysVector operators and loose functions in which is "
    "passed") {
  PhysVector a;

  REQUIRE(pVect.size() == 0);

  SUBCASE("Testing operator * between vectors") {}
  SUBCASE("Testing operator * between vector and scalar") {}
  SUBCASE("Testing function norm()") {
    CHECK(a{0., 0., 0.} == 0.);
    CHECK(a{1. / sqrt(3.), 1. / sqrt(3.), -1. / sqrt(3.)} == 1.)
    CHECK(a{3., 2., -1.} == sqrt(14.));
    CHECK(a{13.5, -8.3, -0.9} == doctest::Approx(15.8729));
    CHECK(a{86.461, 37.927, -58.158} == doctest::Approx(111.029));
    CHECK(a{-30.1024724532, -189.2918006920, 289.0000005000} ==
          doctest::Approx(346.783138209));
  }
}
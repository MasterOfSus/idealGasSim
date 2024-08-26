#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cmath"
#include "doctest.h"
#include "namespacer.hpp"

// TEST_CASE("Testing the functions for the reduced quadratic formula") {}

TEST_CASE("Testing the user-defined type PhysVector implementation") {
  SUBCASE("Testing function norm()") {
    thermo::PhysVector a;

    CHECK(norm(a{0., 0., 0.}) == 0.);
    CHECK(norm(a{1. / sqrt(3.), 1. / sqrt(3.), -1. / sqrt(3.)}) == 1.);
    CHECK(norm(a{3., 2., -1.}) == sqrt(14.));
    CHECK(norm(a{13.5, -8.3, -0.9}) == doctest::Approx(15.8729));
    CHECK(norm(a{86.461, 37.927, -58.158}) == doctest::Approx(111.029));
    CHECK(norm(a{-30.1024724532, -189.2918006920, 289.0000005000}) ==
          doctest::Approx(346.783138209));
  }

  /* SUBCASE("Testing operator * between vectors") {}
   SUBCASE("Testing operator * between vector and scalar") {}
   SUBCASE("Testing operator / between vector and scalar") {}
   SUBCASE("Testing operator + between vectors") {}
   SUBCASE("Testing operator - between vectors") {}
   SUBCASE("Testing operator += between vectors") {}*/
}
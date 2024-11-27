#include <algorithm>
#include <array>
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
double gasSim::Particle::radius = 1;

TEST_CASE("Testing for_any_couple") {
  std::vector<int> num{1, 2, 3, 4};

  SUBCASE("Correct generation of couples") {
    std::vector<std::set<int>> couples{};
    // Usiamo std::set perchè non importa l'ordine

    gasSim::for_each_couple(num.begin(), num.end(),
                            [&](int a, int b) { couples.push_back({a, b}); });

    std::vector<std::set<int>> actualCouples{{1, 2}, {1, 3}, {1, 4},
                                             {2, 3}, {2, 4}, {3, 4}};
    CHECK(couples == actualCouples);
  }
  SUBCASE("Empty input vector") {
    std::vector<int> emptyNum{};
    std::vector<std::set<int>> couples{};

    gasSim::for_each_couple(emptyNum.begin(), emptyNum.end(),
                            [&](int a, int b) { couples.push_back({a, b}); });

    CHECK(couples.empty());
  }
  SUBCASE("Single element vector") {
    std::vector<int> singleNum{5};
    std::vector<std::set<int>> couples{};

    gasSim::for_each_couple(singleNum.begin(), singleNum.end(),
                            [&](int a, int b) { couples.push_back({a, b}); });

    CHECK(couples.empty());
  }
}

TEST_CASE("Testing PhysVector") {
  gasSim::PhysVector vec1{1.1, -2.11, 56.253};
  gasSim::PhysVector vec2{12.3, 0.12, -6.3};
  // gasSim::PhysVector weirdFloatVector{-5.5f, 100000.f, 4.3f};
  // gasSim::PhysVector weirdIntVector{-5, 6, 3};
  gasSim::PhysVector randVec{gasSim::randomVector(5)};
  // gasSim::PhysVector randomvec2(67.);
  SUBCASE("Constructor") {
    CHECK(vec1.x == 1.1);
    CHECK(vec1.y == -2.11);
    CHECK(vec1.z == 56.253);
    CHECK(vec2.x == 12.3);
    CHECK(vec2.y == 0.12);
    CHECK(vec2.z == -6.3);
  }
  SUBCASE("Equality") {
    CHECK(vec1 == vec1);
    CHECK(vec2 == vec2);
  }
  SUBCASE("Random constructor") {
    gasSim::PhysVector randVec2{gasSim::randomVector(5)};
    CHECK(randVec.norm() <= 5);
    CHECK(randVec2 != randVec);
  }
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
  SUBCASE("Division by zero") {
    double l{0};
    gasSim::PhysVector division{vec1 / l};
    gasSim::PhysVector actualDivision{INFINITY, -INFINITY, INFINITY};

    CHECK(division == actualDivision);
  }
  SUBCASE("Scalar Product") {
    double scalarProduct1{vec1 * vec2};
    double scalarProduct2{vec2 * vec1};
    double actualProduct{-341.1171};

    CHECK(scalarProduct1 == actualProduct);
    CHECK(scalarProduct1 == scalarProduct2);
  }
  SUBCASE("Norm") { CHECK(vec1.norm() == doctest::Approx(56.3033)); }
}

TEST_CASE("Testing Particle") {
  SUBCASE("Default constructor") {
    gasSim::PhysVector zeroVec{0, 0, 0};
    gasSim::PhysVector vec1{7.94, 3.60, 4.27};
    gasSim::PhysVector vec2{4.85, 8.94, 5.71};

    gasSim::Particle part1;
    gasSim::Particle part2{vec1, vec2};
    CHECK(part1.position == zeroVec);
    CHECK(part1.speed == zeroVec);
    CHECK(part2.position == vec1);
    CHECK(part2.speed == vec2);
  }
  SUBCASE("particles overlap") {
    gasSim::Particle part1{{5.28, 9.14, 0.36}, {3.49, 2.05, 7.18}};
    gasSim::Particle part2{{6.94, 3.50, 7.18}, {0.16, 9.38, 4.57}};
    gasSim::Particle overlapPart{{5.30, 9.05, 0.37}, {1.29, 7.03, 8.45}};

    CHECK(gasSim::particleOverlap(part1, part2) == false);
    CHECK(gasSim::particleOverlap(part1, overlapPart) == true);
  }
}
TEST_CASE("Testing WallCollision") {
  // Appena sappiamo cos'è un muro
}
TEST_CASE("Testing ParticleCollision") {
  double time{4};

  gasSim::PhysVector vec1{4.23, -5.34, 6.45};
  gasSim::PhysVector vec2{5.46, -4.35, 3.24};
  gasSim::Particle part1{vec1, vec2};

  gasSim::PhysVector vec3{13.4, -1.99, 49.953};
  gasSim::PhysVector vec4{0.11, 0.211, 5.6253};
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
  SUBCASE("1") {
    gasSim::Particle part1{{0, 0, 0}, {1, 1, 1}};
    gasSim::Particle part2{{4, 4, 4}, {0, 0, 0}};
    CHECK(doctest::Approx(gasSim::collisionTime(part1, part2)) == 2.84529);
  }
  SUBCASE("2") {
    gasSim::PhysVector pos1{20992.06862014, -19664.47218241, 6281.158218151};
    gasSim::PhysVector speed1{-2.098936862014, 1.966557218241,
                              -0.6282474445917};
    speed1 = speed1 * 1E4;

    gasSim::PhysVector pos2{3.299915613847, 2.900102791466, -0.6838802535057};
    gasSim::PhysVector speed2{0.8438615274498, -1.027914662675, 1.080195225286};
    speed2 = speed2 * 1E-4;
    gasSim::Particle part1{pos1, speed1};
    gasSim::Particle part2{pos2, speed2};
    CHECK(doctest::Approx(gasSim::collisionTime(part1, part2)) == 1);
  }
  SUBCASE("3") {
    gasSim::Particle part1{{4.82, 1.66, 0.43}, {-6.11, -6.79, 9.18}};
    gasSim::Particle part2{{3.43, 7.54, 6.04}, {7.05, 8.86, -9.04}};
    CHECK(gasSim::collisionTime(part1, part2) == INFINITY);
  }
}

TEST_CASE("Testing Gas constructor") {
  double side{1E3};
  double temp{1.};
  int partNum{100};

  gasSim::Gas Gas{partNum, temp, side};
  SUBCASE("Constructor from gas") {
    gasSim::Gas copyGas{Gas};
    CHECK(copyGas.getBoxSide() == side);
    CHECK(copyGas.getParticles().size() == partNum);
  }
  SUBCASE("Constructor from parameters") {
    gasSim::Particle part1{{8.10, 2.36, 4.75}, {3.83, 3.23, 5.76}};
    gasSim::Particle part2{{4.26, 0.24, 0.22}, {5.92, 5.98, 7.65}};
    gasSim::Particle part3{{2.10, 8.50, 0.32}, {8.92, 7.55, 5.48}};

    gasSim::Particle overlapPart{{7.87, 2.39, 5.00}, {5, 6, 7}};
    std::vector<gasSim::Particle> badParts1{
        part1, part2, part3, overlapPart};  // Particelle che si conpenetrano
    CHECK_THROWS_AS(gasSim::Gas(badParts1, side), std::invalid_argument);

    gasSim::Particle outPart{{1E3 + 1E-4, 7.89, 3.46}, {}};
    std::vector<gasSim::Particle> badParts2{
        part1, outPart, part2, part3};  // Particelle che si conpenetrano
    CHECK_THROWS_AS(gasSim::Gas(badParts2, side), std::invalid_argument);

    std::vector<gasSim::Particle> goodParts{part1, part2, part3};
    CHECK_NOTHROW(gasSim::Gas goodGas{goodParts, side});
  }
  /*
   SUBCASE("Speed check") {
     double maxSpeed = 4. / 3. * temp;
     auto firstIt = Gas.getParticles().begin(),
          lastIt = Gas.getParticles().end();

     std::for_each(firstIt, lastIt, [=](const gasSim::Particle& p) {
       CHECK(p.speed.norm() <= maxSpeed);
     });
   }
   SUBCASE("Find first Particle Collsion") {
     Gas.findFirstPartCollision(INFINITY);
   }
   SUBCASE("Resolve Collision") { Gas.gasLoop(1); }*/
}
TEST_CASE("Testing Gas, find first collision") {
  SUBCASE("Simple collision") {
    double side{1E3};
    gasSim::Particle part1{{0, 0, 0}, {1, 1, 1}};
    gasSim::Particle part2{{4, 4, 4}, {0, 0, 0}};
    std::vector<gasSim::Particle> vec{part1, part2};
    gasSim::Gas gas{vec, side};

    gasSim::ParticleCollision partColl{gas.findFirstPartCollision()};

    auto first = partColl.getFirstParticle();
    auto second = partColl.getSecondParticle();

    // Verifica che le due particelle siano part1 e part2 indipendentemente
    // dall'ordine
    bool condition1 =
        (first->position == part1.position && first->speed == part1.speed &&
         second->position == part2.position && second->speed == part2.speed);
    bool condition2 =
        (first->position == part2.position && first->speed == part2.speed &&
         second->position == part1.position && second->speed == part1.speed);
    
    bool result = condition1 || condition2;

    CHECK(result);
  }
  SUBCASE("None collision") {
    gasSim::Particle part1{{4.82, 1.66, 0.43}, {-6.11, -6.79, 9.18}};
    gasSim::Particle part2{{3.43, 7.54, 6.04}, {7.05, 8.86, -9.04}};
    std::vector<gasSim::Particle> vec{part1, part2};
    gasSim::Gas gas{vec, 40};

    CHECK_THROWS_AS(gas.findFirstPartCollision(), std::runtime_error);
  }
}

TEST_CASE("Testing Gas 2") {
  // gasSim::randomVector(-1);
  /*
  gasSim::Particle p1{{0,0,0}, {1,1,1}};
  gasSim::Particle p2{{0,0,0.09}, {5,6,7}};
  std::vector<gasSim::Particle> badParticles{p1,p2};
  gasSim::Gas badGas{badParticles, 5}; */
}

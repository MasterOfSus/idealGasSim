#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "SFML/Graphics.hpp"

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
  gasSim::PhysVector randVec{gasSim::unifRandoVector(5)};
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
    gasSim::PhysVector randVec2{gasSim::unifRandoVector(5)};
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
  SUBCASE("Normalize") {
    vec1.normalize();
    gasSim::PhysVector normalizeVec1{0.019537, -0.0374756, 0.999107};
    CHECK(doctest::Approx(vec1.x) == normalizeVec1.x);
    CHECK(doctest::Approx(vec1.y) == normalizeVec1.y);
    CHECK(doctest::Approx(vec1.z) == normalizeVec1.z);
  }
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
  double time{4};

  gasSim::PhysVector vec1{10, 10, 0};
  gasSim::PhysVector vec2{0, 0, 10};
  gasSim::Particle part1{vec1, vec2};

  gasSim::WallCollision coll{time, &part1, 'u'};
  SUBCASE("Constructor") {
    CHECK(coll.getFirstParticle()->position == vec1);
    CHECK(coll.getFirstParticle()->speed == vec2);
    CHECK(coll.getWall() == 'u');
  }
  SUBCASE("Change the particles") {
    gasSim::PhysVector actualPosition{4, 0, 4};
    gasSim::PhysVector actualSpeed{1.34, 0.04, 9.3924};
    coll.getFirstParticle()->position.x = 4;
    coll.getFirstParticle()->position.y = 0;
    coll.getFirstParticle()->position.z = 4;
    coll.getFirstParticle()->speed = actualSpeed;
    CHECK(part1.position == actualPosition);
    CHECK(part1.speed == actualSpeed);
  }
}
TEST_CASE("Testing PartCollision") {
  double time{4};

  gasSim::PhysVector vec1{4.23, -5.34, 6.45};
  gasSim::PhysVector vec2{5.46, -4.35, 3.24};
  gasSim::Particle part1{vec1, vec2};

  gasSim::PhysVector vec3{13.4, -1.99, 49.953};
  gasSim::PhysVector vec4{0.11, 0.211, 5.6253};
  gasSim::Particle part2{vec3, vec4};

  gasSim::PartCollision coll{time, &part1, &part2};
  SUBCASE("Constructor") {
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
TEST_CASE("Testing calculateWallColl") {
  SUBCASE("1") {
    gasSim::Particle part{{0.1, 0.1, 5}, {10, 0, 0}};
    gasSim::WallCollision coll{gasSim::calculateWallColl(part, 10)};
    CHECK(doctest::Approx(coll.getTime()) == 0.89);
    CHECK(coll.getFirstParticle() == &part);
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
     Gas.firstPartCollision(INFINITY);
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
    gas.gasLoop(1);

    auto newVec{gas.getParticles()};
    double life{gas.getLife()};

    gasSim::Particle part1F{{2.84529, 2.84529, 2.84529}, {0, 0, 0}};
    gasSim::Particle part2F{{4, 4, 4}, {1, 1, 1}};

    CHECK(doctest::Approx(life) == 2.84529);
    
    CHECK(doctest::Approx(newVec[0].position.x) == part1F.position.x);
    CHECK(doctest::Approx(newVec[0].position.y) == part1F.position.y);
    CHECK(doctest::Approx(newVec[0].position.z) == part1F.position.z);

    CHECK(doctest::Approx(newVec[0].speed.x) == part1F.speed.x);
    CHECK(doctest::Approx(newVec[0].speed.y) == part1F.speed.y);
    CHECK(doctest::Approx(newVec[0].speed.z) == part1F.speed.z);

    CHECK(doctest::Approx(newVec[1].position.x) == part2F.position.x);
    CHECK(doctest::Approx(newVec[1].position.y) == part2F.position.y);
    CHECK(doctest::Approx(newVec[1].position.z) == part2F.position.z);

    CHECK(doctest::Approx(newVec[1].speed.x) == part2F.speed.x);
    CHECK(doctest::Approx(newVec[1].speed.y) == part2F.speed.y);
    CHECK(doctest::Approx(newVec[1].speed.z) == part2F.speed.z);
  }
  /*
  SUBCASE("None collision") {
    gasSim::Particle part1{{4.82, 1.66, 0.43}, {-6.11, -6.79, 9.18}};
    gasSim::Particle part2{{3.43, 7.54, 6.04}, {7.05, 8.86, -9.04}};
    std::vector<gasSim::Particle> vec{part1, part2};
    gasSim::Gas gas{vec, 40};

    CHECK_THROWS_AS(gas.firstPartCollision(), std::logic_error);
  }*/
}

TEST_CASE("Testing Gas 2") {
  // gasSim::randomVector(-1);
  /*
  gasSim::Particle p1{{0,0,0}, {1,1,1}};
  gasSim::Particle p2{{0,0,0.09}, {5,6,7}};
  std::vector<gasSim::Particle> badParticles{p1,p2};
  gasSim::Gas badGas{badParticles, 5}; */
}

// GRAPHICS TESTING

TEST_CASE("Testing the RenderStyle class") {
	gasSim::RenderStyle defStyle {};
	sf::Texture pImage;
	pImage.loadFromFile("./resources/ball.jpg");
	sf::CircleShape pProj {1., 20};
	pProj.setTexture(&pImage);
	gasSim::RenderStyle realStyle {pProj};
	SUBCASE("Constructor") {
		CHECK(defStyle.getBGColor() == sf::Color::White);
		CHECK(defStyle.getWallsOpts() == "udlrfb");
		CHECK(defStyle.getWallsColor() == sf::Color(0, 0, 0, 64));
		CHECK(defStyle.getWOutlineColor() == sf::Color::Black);
		CHECK(defStyle.getPartProj().getFillColor() == sf::Color::Red);
		CHECK(realStyle.getPartProj().getTexture() == &pImage);
	}
	defStyle.setBGColor(sf::Color::Transparent);
	defStyle.setWallsOpts("fb");
	defStyle.setWOutlineColor(sf::Color::Blue);
	defStyle.setWallsColor(sf::Color::Cyan);
	defStyle.setPartProj(pProj);
	SUBCASE("Setters") {
		CHECK_THROWS(defStyle.setWallsOpts("amogus"));

		CHECK(defStyle.getBGColor() == sf::Color::Transparent);
		CHECK(defStyle.getWallsOpts() == "fb");
		CHECK(defStyle.getWOutlineColor() == sf::Color::Blue);
		CHECK(defStyle.getWallsColor() == sf::Color::Cyan);
		CHECK(defStyle.getPartProj().getTexture() == pProj.getTexture());
	}
}

TEST_CASE("Testing the camera class") {
	gasSim::PhysVector focus {0., 0., 0.};
	gasSim::PhysVector sightVector {1., 0., 0.};
	double distance {1.};
	double fov {90.};
	int width {200};
	int height {200};
	gasSim::Camera camera {focus, sightVector, distance, fov, width, height};
	gasSim::Camera camera2 {camera};
	SUBCASE("Constructor") {
		CHECK(camera.getFocus() == focus);
		CHECK(camera.getSight() == sightVector);
		CHECK(camera.getPlaneDistance() == distance);
		CHECK(camera.getWidth() == width);
		CHECK(camera.getHeight() == height);
		CHECK(camera.getFOV() == fov);
		CHECK(camera.getAspectRatio() == 1.);
	}
	gasSim::PhysVector newFocus {-1., -1., 1.};
	gasSim::PhysVector newSight {1., 1., 0.5};
	double newDistance {1.5};
	double newFov {70.};
	int newWidth {1000};
	int newHeight {1600};
	double newRatio {16./9.};
	camera.setFocus(newFocus);
	camera.setSightVector(newSight);
	camera.setPlaneDistance(newDistance);
	camera.setFOV(newFov);
	camera.setResolution(newWidth, newHeight);
	SUBCASE("Setters") {
		// check for throws
		CHECK_THROWS(camera.setFOV(-10.));
		CHECK_THROWS(camera.setFOV(0.));
		CHECK_THROWS(camera.setPlaneDistance(-15.));
		CHECK_THROWS(camera.setPlaneDistance(0.));
		CHECK_THROWS(camera.setResolution(-1, 10));
		CHECK_THROWS(camera.setResolution(0., -1));
		CHECK_THROWS(camera.setSightVector({0., 0., 0.}));
		CHECK_THROWS(camera.setAspectRatio(-15.));
		CHECK_THROWS(camera.setAspectRatio(0.));

		// check for setters correct effect and exception safety
		CHECK(camera.getFocus() == newFocus);
		CHECK(camera.getSight() == newSight);
		CHECK(camera.getPlaneDistance() == newDistance);
		CHECK(camera.getWidth() == newWidth);
		CHECK(camera.getHeight() == newHeight);
		camera.setAspectRatio(newRatio);
		CHECK(camera.getAspectRatio() == newRatio);
		CHECK(camera.getWidth() == newWidth);
	}

	SUBCASE("Auxiliary functions") {
		CHECK(camera2.getTopSide() == doctest::Approx(std::sqrt(2.)));
		CHECK(camera.getTopSide() == doctest::Approx(2.1006));
		CHECK(camera2.getPixelSide() == doctest::Approx(0.007071));
		CHECK(camera.getPixelSide() == doctest::Approx(0.002101));
		CHECK(camera2.getNPixels(2.) == doctest::Approx(2./0.007071));
		CHECK(camera2.getNPixels(-0.5) == doctest::Approx(0.5/0.007071));
		CHECK(camera.getNPixels(2.) == doctest::Approx(2./0.002101));
	}

	gasSim::PhysVector p1 {2., 1., 0.};
	gasSim::PhysVector p2 {-2., 3., -3.};
	gasSim::PhysVector p3 {100., 12., 30.};
	gasSim::PhysVector p4 {2., 2., 2.5};

	gasSim::PhysVector speed {0., 0., 0.};

	gasSim::Particle part1 {p1, speed};
	gasSim::Particle part2 {p2, speed};
	gasSim::Particle part3 {p3, speed};
	gasSim::Particle part4 {p4, speed};
	std::vector<gasSim::Particle> particles {
		part1, part2, part3, part4
	};

	std::vector<gasSim::PhysVector> projections {};

	gasSim::PhysVector projection {};

	SUBCASE("Projection functions") {
		projection = camera.getPointProjection(p1);
		projections.emplace_back(projection);
		CHECK(projection.x == doctest::Approx(168.309));
		CHECK(projection.y == doctest::Approx(-504.927));
		CHECK(projection.z == doctest::Approx(0.5));
		projection = camera.getPointProjection(p2);
		// projections.emplace_back(projection);
		CHECK(projection.x == doctest::Approx(-3786.95));
		CHECK(projection.y == doctest::Approx(-4798.80));
		CHECK(projection.z == doctest::Approx(2.25));
		projection = camera.getPointProjection(p3);
		// projections.emplace_back(projection);
		CHECK(projection.x == doctest::Approx(1186.401));
		CHECK(projection.y == doctest::Approx(-713.2529));
		CHECK(projection.z == doctest::Approx(-0.031469));
		projection = camera.getPointProjection(p4);
		projections.emplace_back(projection);
		CHECK(projection.x == doctest::Approx(0.));
		CHECK(projection.y == doctest::Approx(0.));
		CHECK(projection.z == doctest::Approx(1./3.));
		std::vector<gasSim::PhysVector> realProjs {camera.projectParticles(particles)};
		for (int i {0}; i < 2; ++i) {
			CHECK(projections[i] == realProjs[i]);
		}
	}
}

// ciucciami le Particelle

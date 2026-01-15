#include <TFile.h>
#include <TGraph.h>
#include <TMultiGraph.h>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <cmath>
#include <vector>

#include "SFML/Graphics.hpp"
#include "testingAddons.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "../gasSim/Graphics.hpp"
#include "doctest.h"
// #include "input.hpp"
#include "../gasSim/DataProcessing.hpp"
#include "../gasSim/Input.hpp"
#include "../gasSim/PhysicsEngine.hpp"

// setting particle mass and radius
std::atomic<double> GS::Particle::mass = 10.;
std::atomic<double> GS::Particle::radius = 1.;

TEST_CASE("Testing GSVectorD") {
  GS::GSVectorD vec1{1.1, -2.11, 56.253};
  GS::GSVectorD vec2{12.3, 0.12, -6.3};
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
  SUBCASE("Sum") {
    GS::GSVectorD sum{vec1 + vec2};
    GS::GSVectorD actualSum{13.4, -1.99, 49.953};
    CHECK(doctest::Approx(sum.x) == actualSum.x);
    CHECK(doctest::Approx(sum.y) == actualSum.y);
    CHECK(doctest::Approx(sum.z) == actualSum.z);
  }
  SUBCASE("Subtraction") {
    GS::GSVectorD subtraction{vec2 - vec1};
    GS::GSVectorD actualSubtraction{11.2, 2.23, -62.553};
    CHECK(doctest::Approx(subtraction.x) == actualSubtraction.x);
    CHECK(doctest::Approx(subtraction.y) == actualSubtraction.y);
    CHECK(doctest::Approx(subtraction.z) == actualSubtraction.z);
  }
  SUBCASE("Product") {
    double l{0.25};
    GS::GSVectorD product1{vec1 * l};
    GS::GSVectorD product2{l * vec1};
    GS::GSVectorD actualProduct{0.275, -0.5275, 14.06325};

    CHECK(doctest::Approx(product1.x) == actualProduct.x);
    CHECK(doctest::Approx(product1.y) == actualProduct.y);
    CHECK(doctest::Approx(product1.z) == actualProduct.z);
    CHECK(product1 == product2);
  }
  SUBCASE("Division") {
    double l{10};
    GS::GSVectorD division{vec1 / l};
    GS::GSVectorD actualDivision{0.11, -0.211, 5.6253};
    CHECK(doctest::Approx(division.x) == actualDivision.x);
    CHECK(doctest::Approx(division.y) == actualDivision.y);
    CHECK(doctest::Approx(division.z) == actualDivision.z);
  }
  SUBCASE("Division by zero") {
    double l{0};
    GS::GSVectorD division{vec1 / l};
    GS::GSVectorD actualDivision{INFINITY, -INFINITY, INFINITY};

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
    GS::GSVectorD normalizeVec1{0.019537, -0.0374756, 0.999107};
    CHECK(doctest::Approx(vec1.x) == normalizeVec1.x);
    CHECK(doctest::Approx(vec1.y) == normalizeVec1.y);
    CHECK(doctest::Approx(vec1.z) == normalizeVec1.z);
  }
}

TEST_CASE("Testing Particle") {
  SUBCASE("Default constructor") {
    GS::GSVectorD zeroVec{0, 0, 0};
    GS::GSVectorD vec1{7.94, 3.60, 4.27};
    GS::GSVectorD vec2{4.85, 8.94, 5.71};

    GS::Particle part1;
    GS::Particle part2{vec1, vec2};
    CHECK(part1.position == zeroVec);
    CHECK(part1.speed == zeroVec);
    CHECK(part2.position == vec1);
    CHECK(part2.speed == vec2);
  }
  SUBCASE("particles overlap") {
    GS::Particle part1{{5.28, 9.14, 0.36}, {3.49, 2.05, 7.18}};
    GS::Particle part2{{6.94, 3.50, 7.18}, {0.16, 9.38, 4.57}};
    GS::Particle overlapPart{{5.30, 9.05, 0.37}, {1.29, 7.03, 8.45}};

    CHECK(GS::overlap(part1, part2) == false);
    CHECK(GS::overlap(part1, overlapPart) == true);
  }
  SUBCASE("energy") {
    GS::Particle p1{{1., 0., 2.}, {3., -2., 6.}};
    CHECK(doctest::Approx(energy(p1)) == p1.getMass() * 24.5);
  }
}

TEST_CASE("Testing WallCollision") {
  double time{4};

  GS::GSVectorD vec1{10, 10, 0};
  GS::GSVectorD vec2{0, 0, 10};
  GS::Particle part1{vec1, vec2};

  GS::PWCollision coll{time, &part1, GS::Wall::Top};
  SUBCASE("Constructor") {
    CHECK(coll.getP1()->position == vec1);
    CHECK(coll.getP1()->speed == vec2);
    CHECK(coll.getWall() == GS::Wall::Top);
  }
  SUBCASE("Change the particles") {
    GS::GSVectorD actualPosition{4, 0, 4};
    GS::GSVectorD actualSpeed{1.34, 0.04, 9.3924};
    coll.getP1()->position.x = 4;
    coll.getP1()->position.y = 0;
    coll.getP1()->position.z = 4;
    coll.getP1()->speed = actualSpeed;
    CHECK(part1.position == actualPosition);
    CHECK(part1.speed == actualSpeed);
  }
  SUBCASE("Resolve Collision") {
    coll.solve();
		CHECK(doctest::Approx(coll.getP1()->speed.x) == 0.);
		CHECK(doctest::Approx(coll.getP1()->speed.y) == 0.);
		CHECK(doctest::Approx(coll.getP1()->speed.z) == -10.);
  }
}
TEST_CASE("Testing PartCollision") {
  double time{4};

  GS::GSVectorD vec1{4.23, -5.34, 6.45};
  GS::GSVectorD vec2{5.46, -4.35, 3.24};
  GS::Particle part1{vec1, vec2};

  GS::GSVectorD vec3{13.4, -1.99, 49.953};
  GS::GSVectorD vec4{0.11, 0.211, 5.6253};
  GS::Particle part2{vec3, vec4};

  GS::PPCollision coll{time, &part1, &part2};
  SUBCASE("Constructor") {
    CHECK(coll.getP1()->position == vec1);
    CHECK(coll.getP1()->speed == vec2);
    CHECK(coll.getP2()->position == vec3);
    CHECK(coll.getP2()->speed == vec4);
  }
  SUBCASE("Change the particles") {
    GS::GSVectorD actualPosition{4, 0, 4};
    GS::GSVectorD actualSpeed{1.34, 0.04, 9.3924};
    coll.getP1()->position.x = 4;
    coll.getP1()->position.y = 0;
    coll.getP1()->position.z = 4;
  }
	SUBCASE("Solving method") {
		// getting the particles in contact, even if it doesn't matter since
		// resolution just applies a formula on the normalized distance
		coll.getP1()->position = {0., 0., 0.};
		coll.getP1()->speed = {5.46, 4.35, -3.24};
		GS::GSVectorD p2Pos = {sqrt(2. * GS::Particle::getRadius()), sqrt(2. * GS::Particle::getRadius()), 0.};
		coll.getP2()->position = p2Pos;
		coll.solve();
		CHECK(doctest::Approx(coll.getP1()->speed.x) == .7155);
		CHECK(doctest::Approx(coll.getP1()->speed.y) == -0.3945);
		CHECK(doctest::Approx(coll.getP1()->speed.z) == -3.24);
		CHECK(doctest::Approx(coll.getP2()->speed.x) == 4.8545);
		CHECK(doctest::Approx(coll.getP2()->speed.y) == 4.9555);
		CHECK(doctest::Approx(coll.getP2()->speed.z) == 5.6253);
	}
}

TEST_CASE("Testing collisionTime") {
  SUBCASE("1") {
    GS::Particle part1{{0, 0, 0}, {1, 1, 1}};
    GS::Particle part2{{4, 4, 4}, {0, 0, 0}};
    CHECK(doctest::Approx(GS::collisionTime(part1, part2)) == 2.84529);
  }
  SUBCASE("2") {
    GS::GSVectorD pos1{20992.06862014, -19664.47218241, 6281.158218151};
    GS::GSVectorD speed1{-2.098936862014, 1.966557218241, -0.6282474445917};
    speed1 = speed1 * 1E4;

    GS::GSVectorD pos2{3.299915613847, 2.900102791466, -0.6838802535057};
    GS::GSVectorD speed2{0.8438615274498, -1.027914662675, 1.080195225286};
    speed2 = speed2 * 1E-4;
    GS::Particle part1{pos1, speed1};
    GS::Particle part2{pos2, speed2};
    CHECK(doctest::Approx(GS::collisionTime(part1, part2)) == 1);
  }
  SUBCASE("3") {
    GS::Particle part1{{4.82, 1.66, 0.43}, {-6.11, -6.79, 9.18}};
    GS::Particle part2{{3.43, 7.54, 6.04}, {7.05, 8.86, -9.04}};
    CHECK(GS::collisionTime(part1, part2) == INFINITY);
  }
}

TEST_CASE("Testing Gas constructor") {
  SUBCASE("Throwing behaviour") {
    // Negative and null box side
    CHECK_THROWS(GS::Gas(0, 1., -1.));
    CHECK_THROWS(GS::Gas(0, 2., 0.));
    // Negative and null temperature
    CHECK_THROWS(GS::Gas(1, 0., 1.));
    CHECK_THROWS(GS::Gas(1, -4., 1.));
    // The maximum amount of particles and one over it
    // Theoretically 18^3 should be accepted but isn't because of double
    // approximation, better to be safe than sorry and not round down by any
    // means, therefore depending on value the theoretical maximum number might
    // not be accepted, like here
    GS::Particle::setRadius(.5);
    CHECK_THROWS(GS::Gas(18 * 18 * 18, 1., 20.));
    CHECK_NOTHROW(GS::Gas(18 * 18 * 18 - 1, 1., 20.));
    GS::Particle::setRadius(1.);
    std::vector<GS::Particle> goodPs{{{1.01, 1.01, 1.01}, {1., 0., -1.}},
                                     {{2.01, 3.99, 1.01}, {1., 2., 3.}},
                                     {{3.01, 3.99, 3.01}, {1., 5., 6.}}};
    std::vector<GS::Particle> almostGoodPs{{{1., 1., 1.}, {1., 0., -1.}},
                                           {{2., 4., 1.}, {1., 2., 3.}},
                                           {{3., 4., 3.}, {1., 5., 6.}}};
    double boxSide{5.};
    double time{1.};
    GS::Gas goodGas;
    CHECK_NOTHROW(
        goodGas = GS::Gas(std::vector<GS::Particle>(goodPs), boxSide, time));
    CHECK_THROWS(
        GS::Gas(std::vector<GS::Particle>(almostGoodPs), boxSide, time));
    CHECK(goodGas.getParticles() == goodPs);
    CHECK(goodGas.getBoxSide() == 5.);
		// overlap issue
    std::vector<GS::Particle> badPs{
        {{1.1, 2., 3.}, {1., 1., 2.}},        // !!!
        {{1.1, 2.5, 3.99}, {1., 2., 0.}},     // !!!
        {{3.99, 3.99, 3.99}, {1., 5., 6.}}};
    GS::Gas badGas;
    CHECK_THROWS(badGas = GS::Gas(std::move(badPs), boxSide, time));

    std::vector<GS::Particle> moreBadPs{{{}, {}}};  // outside of box
    CHECK_THROWS(badGas = GS::Gas(std::move(moreBadPs), boxSide, time));
  }
  SUBCASE("Random constructor") {
    GS::Gas rndGas{10, 10., 10.};
    CHECK(rndGas.getParticles().size() == 10);
    CHECK(doctest::Approx(std::accumulate(rndGas.getParticles().begin(),
                                          rndGas.getParticles().end(), 0.,
                                          [](double x, GS::Particle const& p) {
                                            return x + GS::energy(p);
                                          }) *
                          2. / 3. / 10.) == 10.);
    GS::Gas rndGas1{1, 10., 10.};
    CHECK(rndGas1.getParticles().size() == 1);
    CHECK(doctest::Approx(GS::energy(rndGas1.getParticles()[0]) * 2. / 3.) ==
          10.);
    GS::Gas rndGas0{0, 0., 10.};
    CHECK(rndGas0.getParticles().size() == 0);
  }
}

TH1D defaultH{};

TEST_CASE("Testing Gas::simulate method") {
  SUBCASE("Throwing behaviour") {
    GS::Gas empty{};
    GS::SimDataPipeline output{1, 1., defaultH};
    CHECK_THROWS(empty.simulate(1));
    CHECK_THROWS(empty.simulate(5));
    CHECK_THROWS(empty.simulate(1, output));
    CHECK_THROWS(empty.simulate(4, output));
    GS::Gas zeroKelvin{5, 0., 10, -2.};
    CHECK_THROWS(empty.simulate(1));
    CHECK_THROWS(empty.simulate(7));
    CHECK_THROWS(empty.simulate(1, output));
    CHECK_THROWS(empty.simulate(3, output));
  }
}

TEST_CASE("Testing Gas, 1 iteration") {
  SUBCASE("Simple collision 2 particles") {
    double side{1E3};
    GS::Particle part1{{2, 2, 2}, {1, 1, 1}};
    GS::Particle part2{{5, 5, 5}, {0, 0, 0}};
    std::vector<GS::Particle> vec{part1, part2};
    GS::Gas gas{std::vector<GS::Particle>(vec), side};
    gas.simulate(1);

    auto newVec{gas.getParticles()};
    double life{gas.getTime()};

    GS::Particle part1F{{3.84529, 3.84529, 3.84529}, {0, 0, 0}};
    GS::Particle part2F{{5, 5, 5}, {1, 1, 1}};

    CHECK(doctest::Approx(life) == 1.8452995);

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
  SUBCASE("Simple collision particle to wall") {
    double side{1E3};
    GS::Particle part{{2, 500, 500}, {35.9, 0, 0}};
    std::vector<GS::Particle> vec{part};

    GS::Gas gas{std::vector<GS::Particle>(vec), side};
    GS::SimDataPipeline output{1, 1., defaultH};
    gas.simulate(1, output);

    auto newVec{gas.getParticles()};
    double life{gas.getTime()};
    GS::Particle partF{{1E3 - 1, 500, 500}, {-35.9, 0, 0}};

    CHECK(doctest::Approx(life) == 27.77158774373);

    CHECK(doctest::Approx(newVec[0].position.x) == partF.position.x);
    CHECK(doctest::Approx(newVec[0].position.y) == partF.position.y);
    CHECK(doctest::Approx(newVec[0].position.z) == partF.position.z);

    CHECK(doctest::Approx(newVec[0].speed.x) == partF.speed.x);
    CHECK(doctest::Approx(newVec[0].speed.y) == partF.speed.y);
    CHECK(doctest::Approx(newVec[0].speed.z) == partF.speed.z);
  }
}

// GRAPHICS TESTING

TEST_CASE("Testing the RenderStyle class") {
  sf::Texture pImage;
  pImage.loadFromFile("assets/lightBall.png");
  GS::RenderStyle style{pImage};
  SUBCASE("Constructor") {
    CHECK(style.getBGColor() == sf::Color::White);
    CHECK(style.getWallsOpts() == "udlrfb");
    CHECK(style.getWallsColor() == sf::Color(0, 0, 0, 64));
    CHECK(style.getWOutlineColor() == sf::Color::Black);
  }
  style.setBGColor(sf::Color::Transparent);
  style.setWallsOpts("fb");
  style.setWOutlineColor(sf::Color::Blue);
  style.setWallsColor(sf::Color::Cyan);
  SUBCASE("Setters") {
    CHECK_THROWS(style.setWallsOpts("amogus"));

    CHECK(style.getBGColor() == sf::Color::Transparent);
    CHECK(style.getWallsOpts() == "fb");
    CHECK(style.getWOutlineColor() == sf::Color::Blue);
    CHECK(style.getWallsColor() == sf::Color::Cyan);
  }
}

TEST_CASE("Testing the camera class") {
  GS::GSVectorF focus{0., 0., 0.};
  GS::GSVectorF sightVector{1., 0., 0.};
  float distance{1.};
  float fov{90.};
  unsigned width{200};
  unsigned height{200};
  GS::Camera camera{focus, sightVector, distance, fov, width, height};
  GS::Camera camera2{camera};
  SUBCASE("Constructor") {
    CHECK(camera.getFocus() == focus);
    CHECK(camera.getSight() == sightVector);
    CHECK(camera.getPlaneDistance() == distance);
    CHECK(camera.getWidth() == width);
    CHECK(camera.getHeight() == height);
    CHECK(camera.getFOV() == fov);
    CHECK(camera.getAspectRatio() == 1.);
  }
  GS::GSVectorF newFocus{-1., -1., 1.};
  GS::GSVectorF newSight{1., 1., 0.5};
  float newDistance{1.5};
  float newFov{70.};
  unsigned newWidth{1000};
  unsigned newHeight{1600};
  float newRatio{16.f / 9.f};
  camera.setFocus(newFocus);
  camera.setSightVector(newSight);
  camera.setPlaneDistance(newDistance);
  camera.setFOV(newFov);
  camera.setResolution(newHeight, newWidth);
  SUBCASE("Setters") {
    // check for throws
    CHECK_THROWS(camera.setFOV(-10.));
    CHECK_THROWS(camera.setFOV(0.));
    CHECK_THROWS(camera.setPlaneDistance(-15.));
    CHECK_THROWS(camera.setPlaneDistance(0.));
    CHECK_THROWS(camera.setResolution(0, 1));
    CHECK_THROWS(camera.setSightVector({0., 0., 0.}));
    CHECK_THROWS(camera.setAspectRatio(-15.));
    CHECK_THROWS(camera.setAspectRatio(0.));

    // check for setters correct effect and exception safety
    CHECK(camera.getFocus() == newFocus);
    CHECK(camera.getSight() == newSight / newSight.norm());
    CHECK(camera.getPlaneDistance() == newDistance);
    CHECK(camera.getWidth() == newWidth);
    CHECK(camera.getHeight() == newHeight);
    camera.setAspectRatio(newRatio);
    CHECK(camera.getAspectRatio() == doctest::Approx(newRatio).epsilon(0.01));
    CHECK(camera.getWidth() == newWidth);
  }

  SUBCASE("Auxiliary functions") {
    CHECK(camera2.getTopSide() == doctest::Approx(2.));
    CHECK(camera.getTopSide() == doctest::Approx(2.1006));
    CHECK(camera2.getPixelSide() == doctest::Approx(0.01));
    CHECK(camera.getPixelSide() == doctest::Approx(0.002101));
    CHECK(camera2.getNPixels(2.) == doctest::Approx(2. / 0.01));
    CHECK(camera2.getNPixels(-0.5) ==
          doctest::Approx(0.5 / 0.01).epsilon(0.01));
    CHECK(camera.getNPixels(2.) ==
          doctest::Approx(2. / 0.002101).epsilon(0.01));
  }

  GS::GSVectorD p1{2., 1., 0.};
  GS::GSVectorD p2{-2., 3., -3.};
  GS::GSVectorD p3{-100., 12., -30.};
  GS::GSVectorD p4{2., 2., 2.5};

  GS::GSVectorD speed{0., 0., 0.};

  GS::Particle part1{p1, speed};
  GS::Particle part2{p2, speed};
  GS::Particle part3{p3, speed};
  GS::Particle part4{p4, speed};
  std::vector<GS::Particle> particles{part1, part2, part3, part4};

  std::vector<GS::GSVectorF> projections{};

  GS::GSVectorF projection{};

  SUBCASE("Projection functions") {
    projection = camera.getPointProjection(static_cast<GS::GSVectorF>(p1));
    projections.emplace_back(projection);
    CHECK(projection.x == doctest::Approx(168.309f + 500.f));
    CHECK(projection.y == doctest::Approx(-504.927f + 800.f));
    CHECK(projection.z == doctest::Approx(0.5f));
    projection = camera.getPointProjection(static_cast<GS::GSVectorF>(p2));
    CHECK(projection.x == doctest::Approx(-3786.95f + 500.f));
    CHECK(projection.y == doctest::Approx(-4796.80f + 800.f));
    CHECK(projection.z == doctest::Approx(2.25f));
    projection = camera.getPointProjection(static_cast<GS::GSVectorF>(p3));
    CHECK(projection.x == doctest::Approx(835.74f + 500.f));
    CHECK(projection.y == doctest::Approx(94.51f + 800.f));
    CHECK(projection.z == doctest::Approx(-0.022167f));
    projection = camera.getPointProjection(static_cast<GS::GSVectorF>(p4));
    projections.emplace_back(projection);
    CHECK(projection.x == doctest::Approx(0. + 500.f));
    CHECK(projection.y == doctest::Approx(0. + 800.f));
    CHECK(projection.z == doctest::Approx(1.f / 3.f));
    std::vector<GS::GSVectorF> realProjs{camera.projectParticles(particles)};
    for (size_t i{0}; i < 2; ++i) {
      CHECK(projections[i] == realProjs[i]);
    }
  }
}

// STATISTICS Testing

TEST_CASE("Testing the GasData class") {
  std::vector<GS::Particle> particles{{{2., 2., 2.}, {2., 3., 0.75}}};
  std::vector<GS::Particle> moreParticles{
      {{2., 3., 4.}, {1., 0., 0.}},
      {{5., 3., 7.}, {0., 0., 0.}},
      {{5., 6., 7.}, {0., -1., 0.}},
  };
  GS::Gas gas{std::vector<GS::Particle>(particles), 4.};
  GS::Gas moreGas{std::vector<GS::Particle>(moreParticles), 12.};

  gas.simulate(1);
  GS::PWCollision collision{
      1. / 3., const_cast<GS::Particle *>(gas.getParticles().data()),
      GS::Wall::Back};
  GS::GasData data{gas, &collision};

  moreGas.simulate(1);
  GS::PPCollision moreCollision{
      1., const_cast<GS::Particle *>(&moreGas.getParticles()[1]),
      const_cast<GS::Particle *>(&moreGas.getParticles()[2])};
  GS::GasData moreData{moreGas, &moreCollision};

  SUBCASE("Testing the constructor and getters") {
    CHECK(gas.getParticles() == data.getParticles());
    CHECK(data.getTime() == gas.getTime());
    CHECK(data.getBoxSide() == gas.getBoxSide());
    CHECK(data.getP1Index() == 0);
    CHECK(data.getWall() == GS::Wall::Back);
    CHECK(data.getCollType() == 'w');
    CHECK(moreGas.getParticles() == moreData.getParticles());
    CHECK(moreData.getTime() == moreGas.getTime());
    CHECK(moreData.getBoxSide() == moreGas.getBoxSide());
    CHECK(moreData.getP1Index() == 1);
    CHECK(moreData.getP2Index() == 2);
    CHECK(moreData.getCollType() == 'p');
  }
  SUBCASE("Testing getter throws") {
    CHECK_THROWS(data.getP2());
    CHECK_THROWS(moreData.getWall());
  }
}

TEST_CASE("Testing the TdStats class and simOutput processStats function") {
  std::vector<GS::Particle> particles{{{2., 2., 2.}, {2., 3., 0.75}}};
  std::vector<GS::Particle> moreParticles{
      {{2., 3., 4.}, {1., 0., 0.}},
      {{5., 3., 7.}, {0., 0., 0.}},
      {{5., 6., 7.}, {0., -1., 0.}},
  };
  GS::Gas gas{std::vector<GS::Particle>(particles), 4.};
  GS::Gas moreGas{std::vector<GS::Particle>(moreParticles), 12.};
  GS::SimDataPipeline output{5, 1., defaultH};
  gas.simulate(5, output);
  output.processData();
  SUBCASE("Testing the constructor") {
    GS::TdStats stats{output.getStats()[0]};
    CHECK(stats.getBoxSide() == 4.);
    CHECK(stats.getVolume() == 64.);
    CHECK(stats.getDeltaT() == 1.5);
    CHECK(stats.getNParticles() == 1);
  }
  SUBCASE("Testing getters") {
    GS::TdStats stats{output.getStats()[0]};
    CHECK(doctest::Approx(stats.getTemp()) == 45.208333333333);
    CHECK(stats.getPressure(GS::Wall::Front) == 5. / 2.);
    CHECK(stats.getPressure(GS::Wall::Back) == 5. / 2.);
    CHECK(stats.getPressure(GS::Wall::Right) == 10. / 6.);
    CHECK(stats.getPressure(GS::Wall::Left) == 10. / 6.);
    CHECK(stats.getPressure(GS::Wall::Top) == 10. / 16.);
    CHECK(stats.getPressure(GS::Wall::Bottom) == 0.);
    CHECK(stats.getTime0() == 0.);
    CHECK(stats.getTime() == 1.5);
    CHECK(stats.getDeltaT() == 1.5);
    CHECK(stats.getMeanFreePath() == doctest::Approx(4.2964998799 / 4.));
    GS::SimDataPipeline moreOutput{5, 1., defaultH};
    moreGas.simulate(5, moreOutput);
    moreOutput.processData();
    GS::TdStats moreStats{moreOutput.getStats()[0]};
    CHECK(moreStats.getTemp() == 20. / 3. / 3.);
    CHECK(moreStats.getPressure(GS::Wall::Front) == 10. / (11. * 72.));
    CHECK(moreStats.getPressure(GS::Wall::Left) == 0);
    CHECK(moreStats.getPressure(GS::Wall::Back) == 10. / (11. * 72.));
    CHECK(moreStats.getPressure(GS::Wall::Right) == 10. / (11. * 72.));
    CHECK(moreStats.getPressure(GS::Wall::Top) == 0.);
    CHECK(moreStats.getPressure(GS::Wall::Bottom) == 0.);
    CHECK(moreStats.getMeanFreePath() == 2.5);
  }
}

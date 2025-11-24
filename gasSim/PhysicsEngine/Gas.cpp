#include "Gas.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>
#include <stdexcept>
#include <thread>

#include "Collision.hpp"
#include "Particle.hpp"
#include "Vector3.hpp"

bool GS::Gas::contains(const Particle& p) {
  if (particles.size()) {
    double r = p.getRadius();
    Vector3d pos = p.position;
    return (r <= pos.x && pos.x <= boxSide - r && r <= pos.y &&
            pos.y <= boxSide - r && r <= pos.z && pos.z <= boxSide - r &&
            &particles.front() <= &p && &p <= &particles.back());
  } else {
    return false;
  }
}

template <typename Iterator, typename Function>
void for_each_couple(Iterator first, Iterator last, Function f) {
  for (auto it1 = first; it1 != last; ++it1) {
    for (auto it2 = std::next(it1); it2 != last; ++it2) {
      f(*it1, *it2);
    }
  }
}

GS::Gas::Gas(std::vector<GS::Particle>&& particles, double boxSide, double time)
    : particles{particles}, boxSide{boxSide}, time{time} {
  // skipping particles size verification, delegating to methods to provide
  // correct behaviour, ecc.
  if (boxSide <= 0) {
    throw std::invalid_argument("Non-positive box side provided");
  }

  std::for_each(particles.begin(), particles.end(), [&](const Particle& p) {
    if (!contains(p)) {
      particles.clear();
      throw std::invalid_argument("Provided particle is not inside of vector");
    }
  });

  for_each_couple(particles.begin(), particles.end(),
                  [&](const Particle& p1, const Particle& p2) {
                    if (overlap(p1, p2)) {
                      particles.clear();
                      throw std::invalid_argument("Overlapping particles");
                    }
                  });
}

auto unifRandVec{[](const double maxNorm) {
  double theta;
  double phi;
  double rho;
  static std::default_random_engine eng(std::random_device{}());
  static std::uniform_real_distribution<double> baseDist(0, 1);
  if (maxNorm <= 0) {
    throw std::invalid_argument("maxNorm must be greater than 0");
  }
  theta = baseDist(eng) * 2. * M_PI;
  phi = -M_PI / 2. + baseDist(eng) * M_PI;
  rho = baseDist(eng) * maxNorm;
  return GS::Vector3d({rho * cos(phi) * cos(theta), rho * cos(phi) * sin(theta),
                       rho * sin(phi)});
}};

GS::Gas::Gas(unsigned particlesN, double temperature, double boxSide,
             double time)
    : boxSide(boxSide), time{time} {
  if (temperature < 0.) {
    throw std::invalid_argument("Negative temperature provided");
  }
  if (boxSide <= 0.) {
    throw std::invalid_argument("Non-positive boxSide provided");
  }

  if (particlesN) {
    size_t npPerSide{static_cast<size_t>(std::ceil(cbrt(particlesN)))};
    double pR{Particle::getRadius()};
    double latticeUnit{(boxSide - 2 * pR) * 0.99 / npPerSide};

    if (latticeUnit <= 2 * pR) {
      throw std::runtime_error("Particle number too large to fit into box");
    }

    double maxSpeed =
        sqrt(30. / M_PI * temperature /
             Particle::getMass());  // should be correct expression

    auto latticePosition = [=](size_t i) {
      // compute integer lattice coordinate
      size_t pI{i % npPerSide};
      size_t pJ{(i / npPerSide) % npPerSide};
      size_t pK{i / (npPerSide * npPerSide)};

      double x{(pI * latticeUnit) + pR + boxSide * 0.05};
      double y{(pJ * latticeUnit) + pR + boxSide * 0.05};
      double z{(pK * latticeUnit) + pR + boxSide * 0.05};
      return Vector3d{x, y, z};
    };

    unsigned index{0};
    std::generate_n(
        std::back_inserter(particles), particlesN - 1, [=, &index]() {
          Particle p{{latticePosition(index)}, unifRandVec(maxSpeed)};
          ++index;
          return p;
        });

    // ensure as exact a final temperature as possible
    // use first particle's direction to avoid having
    // to generate theta and phi again
    Vector3d d{particles.front().speed};
    d.normalize();
    double missingEnergy{3 * particlesN * temperature -
                         std::accumulate(particles.begin(), particles.end(), 0.,
                                         [](double& acc, const Particle& p) {
                                           acc +=
                                               p.speed.norm() * p.speed.norm();
                                         }) *
                             Particle::getMass() / 2.};
    particles.emplace_back(
        Particle({latticePosition(particlesN - 1),
                  Vector3d({d.y, d.x, d.z}) *
                      sqrt(2. * missingEnergy / Particle::getMass())}));
  }
}

GS::PWCollision GS::Gas::firstPWColl() {
  auto getCollision{[&, this](double position, double speed, Wall negWall,
                              Wall posWall, Particle* p) -> PWCollision {
    double time = (speed < 0)
                      ? (position - Particle::getRadius()) / (-speed)
                      : (boxSide - Particle::getRadius() - position) / speed;
    Wall wall = (speed < 0) ? negWall : posWall;
    return {time, p, wall};
  }};

  auto getWallColl{[&](Particle* p) {
    PWCollision result =
        getCollision(p->position.x, p->speed.x, Wall::Left, Wall::Right, p);
    PWCollision collY =
        getCollision(p->position.y, p->speed.y, Wall::Front, Wall::Back, p);
    PWCollision collZ =
        getCollision(p->position.z, p->speed.z, Wall::Bottom, Wall::Top, p);

    if (result.getTime() > collY.getTime()) {
      result = collY;
    }
    if (result.getTime() > collZ.getTime()) {
      result = collZ;
    }

    return result;
  }};

  PWCollision firstColl{INFINITY, nullptr, Wall::Front};

  std::for_each(particles.begin(), particles.end(), [&](Particle& p) {
    PWCollision c{getWallColl(&p)};
    if (c.getTime() < firstColl.getTime()) {
      firstColl = c;
    }
  });
  if (!firstColl.getP1()) {
    throw std::runtime_error("Tried to find a collision for an empty gas");
  }
  return firstColl;
}

double collisionTime(GS::Particle const& p1, GS::Particle const& p2) {
  GS::Vector3d relPos = p1.position - p2.position;
  GS::Vector3d relSpd = p1.speed - p2.speed;

  double a = relSpd * relSpd;
  double b = relPos * relSpd;
  double c = (relPos * relPos) - 4 * std::pow(GS::Particle::getRadius(), 2);

  double result = INFINITY;

  double delta = std::pow(b, 2) - a * c;

  if (delta > 0) {
    double deltaSqrt = std::sqrt(delta);

    double t1 = (-b - deltaSqrt) / a;
    double t2 = (-b + deltaSqrt) / a;

    if (t1 > 0) {
      result = t1;
    } else if (t2 > 0) {
      result = t2;
    }
  }
  return result;
}

// triangular intexing function for set of n elements
auto trIndex(size_t i, size_t nEls) {
  size_t rowIndex{
      nEls - 2 -
      static_cast<size_t>(std::floor(
          std::sqrt(-8 * i + 4 * nEls * (nEls - 1) - 7) / 2. - 0.5))};
  size_t colIndex{i + rowIndex + 1 - nEls * (nEls - 1) / 2 +
                  (nEls - rowIndex) * ((nEls - rowIndex) - 1) / 2};
  return std::pair<size_t, size_t>(rowIndex, colIndex);
}

GS::PPCollision GS::Gas::firstPPColl() {
  // collision compare lambda
  auto getBestPPCollision{[](PPCollision& c, Particle* p1, Particle* p2) {
    Vector3d relPos{p1->position - p2->position};
    Vector3d relSpd{p1->speed - p2->speed};
    if (relPos * relSpd <= 0.) {
      double cTime{collisionTime(*p1, *p2)};
      if (cTime < c.getTime()) {
        c = {collisionTime(*p1, *p2), p1, p2};
      }
    }
  }};

  size_t nP{particles.size()};

  if (!nP) {
    throw std::runtime_error("Tried to get particle collision from empty gas");
  }

  size_t nChecks{nP * (nP - 1) / 2};

  unsigned nThreads{std::thread::hardware_concurrency()};
  size_t checksPerThread{nChecks / nThreads};
  size_t extraChecks{nChecks % nThreads};

  std::vector<std::thread> threads{};
  std::vector<PPCollision> bestColls(nThreads, {INFINITY, nullptr, nullptr});

  // first thread with extra checks
  threads.push_back(std::thread([&]() {
    PPCollision c{INFINITY, nullptr, nullptr};
    size_t endIndex{checksPerThread + extraChecks};
    for (size_t i{0}; i < endIndex; ++i) {
      std::pair<size_t, size_t> trI{trIndex(i, nP)};
      getBestPPCollision(c, particles.data() + trI.first,
                         particles.data() + trI.second);
    }
    bestColls[0] = c;
  }));

  // other threads with normal checks number
  for (size_t threadIndex{1}; threadIndex < nThreads; ++threadIndex) {
    size_t thrI{threadIndex};
    threads.push_back(std::thread([&, thrI]() {
      PPCollision c{INFINITY, nullptr, nullptr};
      size_t i{thrI * checksPerThread + extraChecks};
      size_t endIndex{(thrI + 1) * checksPerThread + extraChecks};
      for (; i < endIndex; ++i) {
        std::pair<size_t, size_t> trI{trIndex(i, nP)};
        getBestPPCollision(c, particles.data() + trI.first,
                           particles.data() + trI.second);
      }
      bestColls[thrI] = c;
    }));
  }

  for (std::thread& t : threads) {
    if (t.joinable()) t.join();
  }

  return *std::min_element(bestColls.begin(), bestColls.end(),
                           [](const PPCollision& c1, const PPCollision& c2) {
                             return c1.getTime() < c2.getTime();
                           });
}

void GS::Gas::move(double dt) {
  assert(dt != INFINITY);
  assert(dt >= 0);
  std::for_each(particles.begin(), particles.end(),
                [dt](Particle& p) { p.position += p.speed * dt; });
  time += dt;
}

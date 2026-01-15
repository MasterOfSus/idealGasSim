#include "Gas.hpp"

#include <algorithm>
#include <cassert>
#include <random>
#include <stdexcept>
#include <thread>

#include "../DataProcessing/SimDataPipeline.hpp"
#include "GSVector.hpp"

namespace GS {

bool Gas::contains(Particle const& p) {
  if (particles.size()) {
    double r = p.getRadius();
    GSVectorD pos = p.position;
    return (r < pos.x && pos.x < boxSide - r && r < pos.y &&
            pos.y < boxSide - r && r < pos.z && pos.z < boxSide - r &&
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

Gas::Gas(std::vector<Particle>&& particlesV, double boxSideV, double timeV)
    : particles{particlesV}, boxSide{boxSideV}, time{timeV} {
  if (boxSide <= 0) {
    this->boxSide = 1.;
    throw std::invalid_argument(
        "Gas constructor error: non-positive box side provided");
  }

  std::for_each(this->particles.begin(), this->particles.end(),
                [&](Particle const& p) {
                  if (!contains(p)) {
                    this->particles.clear();
                    throw std::invalid_argument(
                        "Gas constructor error: at least one of the provided "
                        "particles is not inside of gas box.");
                  }
                });

  for_each_couple(
      this->particles.begin(), this->particles.end(),
      [&](Particle const& p1, Particle const& p2) {
        if (overlap(p1, p2)) {
          this->particles.clear();
          throw std::invalid_argument(
              "Gas constructor error: provided overlapping particles");
        }
      });
}

auto unifRandVec{[](double maxNorm) {
  double theta;
  double phi;
  double rho;
  static std::default_random_engine eng(std::random_device{}());
  static std::uniform_real_distribution<double> baseDist(0, 1);
  if (maxNorm < 0) {
    throw std::invalid_argument(
        "Random vector generator error: provided negative maxNorm");
  }
  theta = baseDist(eng) * 2. * M_PI;
  phi = -M_PI / 2. + baseDist(eng) * M_PI;
  rho = baseDist(eng) * maxNorm;
  return GSVectorD({rho * cos(phi) * cos(theta), rho * cos(phi) * sin(theta),
                    rho * sin(phi)});
}};

Gas::Gas(size_t particlesN, double temperature, double boxSideV, double timeV)
    : boxSide(boxSideV), time{timeV} {
  if (temperature < 0.) {
    throw std::invalid_argument(
        "Gas constructor error: provided negative temperature");
  }
  if (boxSide <= 0.) {
    throw std::invalid_argument(
        "Gas constructor error: provided non-positive boxSide");
  }

  if (particlesN) {
    size_t npPerSide{
			static_cast<size_t>(std::ceil(cbrt(static_cast<double>(particlesN))))
		};
    double pR{Particle::getRadius()};
    double latticeUnit{(boxSide * 0.95 - 2 * pR) / static_cast<double>(npPerSide - 1)};

    if (latticeUnit <= 2. * pR) {
      throw std::runtime_error(
          "Gas constructor error: provided particle number-radius too large to "
          "fit into box");
    }
    if (!std::isfinite(latticeUnit)) {
      if (particlesN == 1) {
        latticeUnit = 0.;
      } else {
        throw std::runtime_error(
            "Gas constructor error: computed non-finite cubical lattice unit, "
            "aborting");
      }
    }

    double maxSpeed = sqrt(30. / M_PI * temperature / Particle::getMass());

    auto latticePosition = [=](size_t i) {
      // compute integer lattice coordinate
      size_t pI{i % npPerSide};
      size_t pJ{(i / npPerSide) % npPerSide};
      size_t pK{i / (npPerSide * npPerSide)};

      double x{(static_cast<double>(pI) * latticeUnit) + pR + 0.025 * boxSide};
      double y{(static_cast<double>(pJ) * latticeUnit) + pR + 0.025 * boxSide};
      double z{(static_cast<double>(pK) * latticeUnit) + pR + 0.025 * boxSide};
      return GSVectorD{x, y, z};
    };

    size_t index{0};
    std::generate_n(
        std::back_inserter(particles), particlesN - 1, [=, &index]() {
          Particle p{{latticePosition(index)}, unifRandVec(maxSpeed)};
          ++index;
          return p;
        });

    // ensure as exact a final temperature as possible
    GSVectorD direction{unifRandVec(1.)};
    direction.normalize();
    double missingEnergy{3. * static_cast<double>(particlesN) * temperature / 2. -
                         std::accumulate(particles.begin(), particles.end(), 0.,
                                         [](double acc, Particle const& p) {
                                           return acc += energy(p);
                                         })};
    if (missingEnergy < 0.) {
      for (Particle& p : particles) {
        missingEnergy += energy(p);
        p.speed = {0., 0., 0.};
        if (missingEnergy > 0.) {
          break;
        }
      }
    }
    particles.emplace_back(
        Particle({latticePosition(particlesN - 1),
                  direction * sqrt(2. * missingEnergy / Particle::getMass())}));
  } else {
    if (temperature) {
      throw std::invalid_argument(
          "Gas constructor error: asked to reach a temperature with zero "
          "particles");
    }
  }
}

PWCollision Gas::firstPWColl() {
  auto getPWCollision{[&, this](double position, double speed, Wall negWall,
                                Wall posWall, Particle* p) -> PWCollision {
    double cTime = (speed < 0)
                      ? (position - Particle::getRadius()) / (-speed)
                      : (boxSide - Particle::getRadius() - position) / speed;
    Wall wall = (speed < 0) ? negWall : posWall;
    return {cTime, p, wall};
  }};

  auto getWallColl{[&](Particle* p) {
    PWCollision result =
        getPWCollision(p->position.x, p->speed.x, Wall::Left, Wall::Right, p);
    PWCollision collY =
        getPWCollision(p->position.y, p->speed.y, Wall::Front, Wall::Back, p);
    PWCollision collZ =
        getPWCollision(p->position.z, p->speed.z, Wall::Bottom, Wall::Top, p);

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
  if (!firstColl.getP1() && !particles.size()) {
    throw std::runtime_error("firstPWColl error: gas is empty");
  }
  return firstColl;
}

double collisionTime(Particle const& p1, Particle const& p2) {
  GSVectorD relPos = p1.position - p2.position;
  GSVectorD relSpd = p1.speed - p2.speed;

  double a = relSpd * relSpd;
  double b = relPos * relSpd;
  double c = (relPos * relPos) - 4 * std::pow(Particle::getRadius(), 2);

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

// triangular indexing function for set of n elements
/*
auto trIndex(size_t i, size_t nEls) {
  size_t rowIndex{
      nEls - 2 -
      static_cast<size_t>(std::floor(
          std::sqrt(-8. * static_cast<double>(i) + 4. * static_cast<double>(nEls * (nEls - 1) - 7)) / 2. - 0.5))};
  size_t colIndex{i + rowIndex + 1 - nEls * (nEls - 1) / 2 +
                  (nEls - rowIndex) * ((nEls - rowIndex) - 1) / 2};
  return std::pair<size_t, size_t>(rowIndex, colIndex);
}*/

auto trIndex(std::size_t i, std::size_t nEls)
{
    const double di = static_cast<double>(i);
    const double dn = static_cast<double>(nEls);

    const double root =
        std::sqrt(-8.0 * di + 4.0 * dn * (dn - 1.0) - 7.0);

    const std::size_t rowIndex = static_cast<std::size_t>(
        dn - 2.0 - std::floor(root / 2.0 - 0.5)
    );

    const std::size_t colIndex =
        i + rowIndex + 1
        - (nEls * (nEls - 1)) / 2
        + ((nEls - rowIndex) * ((nEls - rowIndex) - 1)) / 2;

    return std::pair<size_t, size_t>(rowIndex, colIndex);
}

PPCollision Gas::firstPPColl() {
  // collision compare lambda
  auto getBestPPCollision{[](PPCollision& c, Particle * p1, Particle * p2) {
		GSVectorD relPos {p1->position - p2->position};
    const GSVectorD relSpd{p1->speed - p2->speed};
    if (relPos * relSpd <= 0.) {
      double cTime{collisionTime(*p1, *p2)};
      if (cTime < c.getTime()) {
        c = {collisionTime(*p1, *p2), p1, p2};
      }
    }
  }};

  size_t nP{particles.size()};

  if (!nP) {
    throw std::runtime_error(
        "firstPPColl error: tried to get particle collision from empty gas");
  }

  size_t nChecks{nP * (nP - 1) / 2};

  size_t nThreads{std::thread::hardware_concurrency()};
  size_t checksPerThread{nChecks / nThreads};
  size_t extraChecks{nChecks % nThreads};

	std::mutex particlesMtx;
  std::vector<std::thread> threads {};
	threads.reserve(nThreads);
	std::mutex bestCollsMtx;
  std::vector<PPCollision> bestColls(nThreads, {INFINITY, nullptr, nullptr});

	// concurrent access on particles is read-only
  // first thread with extra checks
  threads.emplace(threads.begin(), [&]() {
    PPCollision c{INFINITY, nullptr, nullptr};
    size_t endIndex{checksPerThread + extraChecks};
    for (size_t i{0}; i < endIndex; ++i) {
      std::pair<size_t, size_t> trI{trIndex(i, nP)};
      getBestPPCollision(c, particles.data() + trI.first,
                         particles.data() + trI.second);
    }
		std::lock_guard<std::mutex> bestCollsGuard {bestCollsMtx};
    bestColls[0] = c;
  });

	if (checksPerThread) {

  // other threads with normal checks number
  for (size_t threadIndex{1}; threadIndex < nThreads; ++threadIndex) {
    size_t thrI{threadIndex};
    threads.emplace(threads.begin() + static_cast<long>(thrI), [&, thrI]() {
      PPCollision c{INFINITY, nullptr, nullptr};
      size_t i{thrI * checksPerThread + extraChecks};
      size_t endIndex{(thrI + 1) * checksPerThread + extraChecks};
      for (; i < endIndex; ++i) {
        std::pair<size_t, size_t> trngI{trIndex(i, nP)};
        getBestPPCollision(c, particles.data() + trngI.first,
                           particles.data() + trngI.second);
      }
			std::lock_guard<std::mutex> bestCollsGuard {bestCollsMtx};
      bestColls[thrI] = c;
    });
  }

	}

  for (auto& t: threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  return *std::min_element(bestColls.begin(), bestColls.end(),
                           [](PPCollision const& c1, PPCollision const& c2) {
                             return c1.getTime() < c2.getTime();
                           });
}

void Gas::move(double dt) {
  assert(dt != INFINITY);
  assert(dt >= 0);
  std::for_each(particles.begin(), particles.end(),
                [dt](Particle& p) { p.position += p.speed * dt; });
  time += dt;
}

void Gas::simulate(size_t itN, std::function<bool()> stopper) {
  for (size_t i{0}; i < itN && !stopper(); ++i) {
    PPCollision pColl{firstPPColl()};
    PWCollision wColl{firstPWColl()};
    Collision* firstColl{nullptr};

    if (pColl.getTime() < wColl.getTime()) {
      firstColl = &pColl;
    } else {
      firstColl = &wColl;
    }

    double collTime{firstColl->getTime()};

    if (std::isfinite(collTime) && collTime >= 0.) {
      move(firstColl->getTime());
      firstColl->solve();
    } else if (particles.size() == 0) {
      throw std::runtime_error(
          "Simulate error: called simulate on an empty gas");
    } else if (collTime < 0.) {
      throw std::runtime_error(
          "Simulate error: found negative collision time -> aborting");
    } else if (!std::isfinite(collTime)) {
      bool allSpeeds0{true};
      for (Particle const& p : particles) {
        if (allSpeeds0 && p.speed.norm() != 0) {
          allSpeeds0 = false;
        }
      }
      if (allSpeeds0) {
        throw std::runtime_error(
            "Simulate error: called on gas with no moving particles");
      } else {
        throw std::runtime_error(
            "Simulate error: unexplained non finite collision time found -> "
            "aborting");
      }
    }
  }
}

void Gas::simulate(size_t itN, SimDataPipeline& output, std::function<bool()> stopper) {
  std::vector<GasData> tempOutput{};
  tempOutput.reserve(output.getStatSize());

  for (size_t i{0}; i < itN && !stopper();) {
    for (size_t j{0}; j < output.getStatSize() && i < itN && !stopper(); ++j, ++i) {
      PPCollision pColl{firstPPColl()};
      PWCollision wColl{firstPWColl()};
      Collision* firstColl{nullptr};

      if (pColl.getTime() < wColl.getTime()) {
        firstColl = &pColl;
      } else {
        firstColl = &wColl;
      }

      double collTime{firstColl->getTime()};

      if (std::isfinite(collTime) && collTime >= 0.) {
        move(firstColl->getTime());
        firstColl->solve();
        tempOutput.emplace_back(GasData(*this, firstColl));
      } else if (particles.size() == 0) {
        throw std::runtime_error(
            "Simulate error: called simulate on an empty gas");
      } else if (collTime < 0.) {
        throw std::runtime_error(
            "Simulate error: found negative collision time found -> aborting");
      } else if (!std::isfinite(collTime)) {
        bool allSpeeds0{true};
        for (Particle const& p : particles) {
          if (allSpeeds0 && p.speed.norm() != 0) {
            allSpeeds0 = false;
          }
        }
        if (allSpeeds0) {
          throw std::runtime_error(
              "Simulate error: called on gas with no moving particles");
        } else {
          throw std::runtime_error(
              "Simulate error: unexplained non finite collision time found -> "
              "aborting");
        }
      }
    }
    output.addData(std::move(tempOutput));
    tempOutput.clear();
  }

  output.setDone();
}

}  // namespace GS

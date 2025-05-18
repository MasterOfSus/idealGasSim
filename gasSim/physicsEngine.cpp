#include "physicsEngine.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <thread>
// #include <execution> //Se decidiamo di fare qualche operazione in parallelo:
// Sddfqoinp

#include "algorithms.hpp"
#include "statistics.hpp"

namespace gasSim {

// double Particle::radius = 1.;

// Definition of vector functions
PhysVectorD unifRandVector(const double maxNorm) {
  if (maxNorm <= 0) {
    throw std::invalid_argument("maxNorm must be greater than 0");
  }
  static std::default_random_engine eng(std::random_device{}());
  std::uniform_real_distribution<double> dist(-std::sqrt(maxNorm / 3),
                                              std::sqrt(maxNorm / 3));
  return {dist(eng), dist(eng), dist(eng)};
}
PhysVectorD gausRandVector(const double standardDev) {
  if (standardDev <= 0) {
    throw std::invalid_argument("standardDev must be greater than 0");
  }
  static std::default_random_engine eng(std::random_device{}());
  std::normal_distribution<double> dist(0., standardDev);
  return {dist(eng), dist(eng), dist(eng)};
}

// End of PhysVector functions

// Definition of Particle functions
bool overlap(const Particle& p1, const Particle& p2) {
  const double minDst = 2 * Particle::radius;
  const double centerDst{(p1.position - p2.position).norm()};
  if (centerDst < minDst) {
    return true;
  } else {
    return false;
  }
}

bool particleInBox(const Particle& part, double boxSide) {
  const double radius = Particle::radius;
  const auto& pos = part.position;

  if (pos.x < radius || pos.x > boxSide - radius || pos.y < radius ||
      pos.y > boxSide - radius || pos.z < radius || pos.z > boxSide - radius) {
    return false;
  }

  return true;
}
// End of Particle functions

// Definition of Statistic functions
/* Statistic::Statistic(double boxSide) : boxSide_(boxSide) {
  if (boxSide_ <= 0) {
    throw std::invalid_argument("boxSide must be a positive number");
  }
}
void Statistic::setDeltaTime(double deltaTime) {
  deltaTime_ = deltaTime;
  if (deltaTime_ <= 0) {
    throw std::invalid_argument("deltaTime must be a positive number");
  }
}
void Statistic::addImpulseOnWall(double speed, Wall wall) {
  deltaImpulse_[wall] += std::abs(speed) * Particle::mass;
}

double Statistic::getPressure(Wall wall) {
  double area{std::pow(boxSide_, 2)};
  return deltaImpulse_[wall] / (area * deltaTime_);
}
// End of Statistic functions
*/
// Definition of Collision functions
Collision::Collision(double t, Particle* p) : time_(t), firstParticle_(p) {
  if (time_ < 0) {
    throw std::invalid_argument("time must be a positive number");
  }
}

Particle* Collision::getFirstParticle() const {
  return firstParticle_;
}  // ATTENZIONE QUA COI PUNTATORI CHE PUNTANO
// End of Collision functions

// Definition of WallCollision functions
WallCollision::WallCollision(double t, Particle* p, Wall wall)
    : Collision(t, p), wall_(wall) {}

Wall WallCollision::getWall() const { return wall_; }
std::string WallCollision::getType() const { return "Wall"; }
/*
Statistic WallCollision::solve(Statistic& stat) {
  Particle* part{getFirstParticle()};

  double memSpd;
  switch (wall_) {
    case Wall::Left:
    case Wall::Right:
      memSpd = part->speed.x;
      part->speed.x = -part->speed.x;
      break;
    case Wall::Front:
    case Wall::Back:
      memSpd = part->speed.y;
      part->speed.y = -part->speed.y;
      break;
    case Wall::Top:
    case Wall::Bottom:
      memSpd = part->speed.x;
      part->speed.z = -part->speed.z;
      break;
  }
  stat.addImpulseOnWall(memSpd, wall_);

  return stat;
}*/
void WallCollision::solve() {
  Particle& part = *getFirstParticle();
  switch (wall_) {
    case Wall::Left:
    case Wall::Right:
      part.speed.x = -part.speed.x;
      break;
    case Wall::Front:
    case Wall::Back:
      part.speed.y = -part.speed.y;
      break;
    case Wall::Top:
    case Wall::Bottom:
      part.speed.z = -part.speed.z;
      break;
    default:
      throw std::logic_error("Unknown wall type.");
  }
}
// End of WallCollision functions

// Definition of PartCollision functions
PartCollision::PartCollision(double t, Particle* p1, Particle* p2)
    : Collision(t, p1), secondParticle_(p2) {}

const Particle* PartCollision::getSecondParticle() const {
  return secondParticle_;
}

std::string PartCollision::getType() const { return "Particle Collision"; }

void PartCollision::solve() {
  Particle* part1{getFirstParticle()};
  Particle* part2{secondParticle_};

  PhysVector centerDist{part1->position - part2->position};
  centerDist.normalize();

  PhysVector speedDiff{part1->speed - part2->speed};

  double projection = centerDist * speedDiff;

  part1->speed -= centerDist * projection;
  part2->speed += centerDist * projection;
}
// End of PartCollision functions

// Definition of Gas functions
Gas::Gas(const Gas& gas)
    : particles_(gas.getParticles()), boxSide_(gas.getBoxSide()) {}

Gas::Gas(std::vector<Particle> particles, double boxSide, double time)
    : particles_(particles), boxSide_(boxSide), time_(time) {
  if (particles_.size() == 0) {
    throw std::invalid_argument("Empty particle vector.");
  }
  if (boxSide_ <= 0) {
    throw std::invalid_argument("Non positive box side.");
  }
  if (time_ < 0) {
    throw std::invalid_argument("Negative time.");
  }

  std::for_each(particles_.begin(), particles_.end(), [&](const Particle& p) {
    if (particleInBox(p, boxSide_) == false) {
      throw std::invalid_argument("Particles outside of the box.");
    }
  });

  for_each_couple(particles_.begin(), particles_.end(),
                  [](const Particle& p1, const Particle& p2) {
                    if (overlap(p1, p2) == true) {
                      throw std::invalid_argument("Overlapping particles.");
                    }
                  });
}

Gas::Gas(int nParticles, double temperature, double boxSide)
    : boxSide_(boxSide) {
  if (nParticles <= 0) {
    throw std::invalid_argument("Non positive particle number.");
  }
  if (temperature <= 0) {
    throw std::invalid_argument("Non positive temperature.");
  }
  if (boxSide_ <= 0) {
    throw std::invalid_argument("Non positive box side.");
  }

  int elementPerSide{static_cast<int>(std::ceil(cbrt(nParticles)))};
  double particleDistance{boxSide * 0.9 / elementPerSide};
  double radius{Particle::radius};

  if (particleDistance <= 2 * radius) {
    throw std::runtime_error("Can't fit all particles in the box.");
  }

  double maxSpeed = 4. / 3. * temperature;  // espressione sbagliata appena
  // riesco faccio il calcolo

  auto gridVector = [=](int i) {
    int pX{i % elementPerSide};
    int pY{(i / elementPerSide) % elementPerSide};
    int pZ{i / (elementPerSide * elementPerSide)};
    // Posizione nella griglia con numeri interi

    double x{(pX * particleDistance) + radius + boxSide * 0.05};
    double y{(pY * particleDistance) + radius + boxSide * 0.05};
    double z{(pZ * particleDistance) + radius + boxSide * 0.05};
    return PhysVectorD{x, y, z};
  };

  int index{0};
  std::generate_n(std::back_inserter(particles_), nParticles, [=, &index]() {
    Particle p{{gridVector(index)}, unifRandVector(maxSpeed)};
    ++index;
    return p;
  });
}

const std::vector<Particle>& Gas::getParticles() const { return particles_; }

double Gas::getBoxSide() const { return boxSide_; }

double Gas::getTime() const { return time_; }

void Gas::simulate(int nIterations, SimOutput& output) {

	// should modify to not insert first gasData into SimOutput
	
  for (int i{0}; i < nIterations; ++i) {
  	// std::cout << "Started " << i << "th iteration.\n";
		PartCollision pColl{firstPartCollision()};
    WallCollision wColl{firstWallCollision()};
    Collision* firstColl{nullptr};

    if (pColl.getTime() < wColl.getTime())
      firstColl = &pColl;
    else
      firstColl = &wColl;

    move(firstColl->getTime());

    firstColl->solve();

		GasData data {*this, firstColl};
    output.addData(data);
  }
	output.setDone();
	//std::cout << "Elapsed simulation time: " << output.getData().back().getTime() - output.getData()[0].getTime() << std::endl;
}

int Gas::getPIndex(const Particle* p) const { return p - particles_.data(); }

WallCollision Gas::firstWallCollision() {
  WallCollision firstColl{INFINITY, nullptr, Wall::Back};

  std::for_each(particles_.begin(), particles_.end(), [&](Particle& p) {
    WallCollision coll{calculateWallColl(p, boxSide_)};
    if (coll.getTime() < firstColl.getTime()) {
      firstColl = coll;
    }
  });
  return firstColl;
}

PartCollision Gas::firstPartCollision() {

	//std::mutex coutMtx;

	auto trIndex {
		[&] (int i, int nEls) {
			int rowIndex {
				nEls - 2 - static_cast<int>(std::floor(std::sqrt(-8*i + 4*nEls*(nEls - 1) - 7) / 2. - 0.5))
			};
			int colIndex {
				i + rowIndex + 1 - nEls*(nEls - 1)/2 + (nEls - rowIndex)*((nEls - rowIndex) - 1)/2
			};
			return std::pair<int, int>(rowIndex, colIndex);
		}
	};

  auto collChecker {
		[&](Particle& p1, Particle& p2, double& lowestTime, Particle* & firstPart, Particle* & secondPart) {
  		PhysVectorD relPos{p1.position - p2.position};
  		// Ottimizzazione e evita gli errori delle approssimazioni
  		// per l'approssimazione dei double delle particelle
  		// compenetrate
 	 		PhysVectorD relSpd{p1.speed - p2.speed};
   		if (relPos * relSpd <= 0) {
     		double time{collisionTime(p1, p2)};
     			if (time < lowestTime) {
      			firstPart = &p1;
      			secondPart = &p2;
      			lowestTime = time;
    		}
  		}
		}
	};

	int nP {static_cast<int>(particles_.size())};

	int nChecks {nP*(nP - 1) / 2 };
	int nThreads {static_cast<int>(std::thread::hardware_concurrency()) - 1};
	if (std::thread::hardware_concurrency() < 3) {
		nThreads = 3;
	}
	int checksPerThread {nChecks/nThreads};
	int extraChecks {nChecks%nThreads};

	std::vector<std::thread> threads {};
	std::vector<PartCollision> bestColls(nThreads, {INFINITY, nullptr, nullptr});

	threads.push_back(
		std::thread(
			[&] () {
			double lowestTime{INFINITY};
			Particle* firstPart{nullptr};
			Particle* secondPart{nullptr};
			int i {0};
			int endIndex {checksPerThread + extraChecks};
			for (; i < endIndex; ++i) {
				std::pair<int, int> trI {trIndex(i, nP)};
				//coutMtx.lock();
				//std::cout << "Checking pair index: " << i << " converted to triangular index: (" << trI.first << ", " << trI.second << ")\n";
				//std::cout.flush();
				//coutMtx.unlock();
				collChecker(particles_[trI.first], particles_[trI.second],
				lowestTime, firstPart, secondPart);
			}
			PartCollision bestColl {lowestTime, firstPart, secondPart};
			bestColls[0] = bestColl;
			}
		)
	);

	int threadIndex {1};

	for (; threadIndex < nThreads; ++threadIndex) {
		int thrI {threadIndex};
		threads.push_back(
			std::thread(
				[&, thrI] () {
					double lowestTime{INFINITY};
					Particle* firstPart{nullptr};
					Particle* secondPart{nullptr};
					int i {thrI * checksPerThread + extraChecks};
					int endIndex {(thrI + 1) * checksPerThread + extraChecks};
					for (; i < endIndex; ++i) {
						std::pair<int, int> trI {trIndex(i, nP)};
						//coutMtx.lock();
						//std::cout << "Checking pair index: " << i << " converted to triangular index: (" << trI.first << ", " << trI.second << ")\n";
						//std::cout.flush();
						//coutMtx.unlock();
						collChecker(particles_[trI.first], particles_[trI.second],
						lowestTime, firstPart, secondPart);
					}
					PartCollision bestColl {lowestTime, firstPart, secondPart};
					bestColls[thrI] = bestColl;
				}
			)
		);
	}

	for (std::thread& t: threads) {
		if (t.joinable())
			t.join();
	}

  return *std::min_element(bestColls.begin(), bestColls.end(), 
			[] (const PartCollision& c1, const PartCollision& c2) {
				return c1.getTime() < c2.getTime();
			}
			);
}

void Gas::move(double time) {
  assert(time != INFINITY);
  std::for_each(particles_.begin(), particles_.end(),
                [time](Particle& part) { part.position += part.speed * time; });
  time_ += time;
  // Sddfqoinp
  /*std::for_each(std::execution::par, particles_.begin(), particles_.end(),
              [time](Particle& part) { part.position += part.speed * time; });*/
}

void Gas::operator=(const Gas& gas) {
	particles_ = gas.particles_;
	boxSide_ = gas.boxSide_;
	time_ = gas.time_;
}

// End of Gas functions

double collisionTime(const Particle& p1, const Particle& p2) {
  PhysVector relativeSpd = p1.speed - p2.speed;
  PhysVector relativePos = p1.position - p2.position;

  double a = relativeSpd * relativeSpd;
  double b = relativePos * relativeSpd;
  double c = (relativePos * relativePos) - 4 * std::pow(Particle::radius, 2);

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

WallCollision calculateWallColl(Particle& p, double squareSide) {
  auto calculate = [&, squareSide](double position, double speed, Wall negWall,
                                   Wall posWall) -> WallCollision {
    double time = (speed < 0)
                      ? (position - Particle::radius) / (-speed)
                      : (squareSide - Particle::radius - position) / speed;
    Wall wall = (speed < 0) ? negWall : posWall;
    return {time, &p, wall};
  };

  WallCollision collX =
      calculate(p.position.x, p.speed.x, Wall::Left, Wall::Right);
  WallCollision collY =
      calculate(p.position.y, p.speed.y, Wall::Front, Wall::Back);
  WallCollision collZ =
      calculate(p.position.z, p.speed.z, Wall::Bottom, Wall::Top);

  WallCollision minColl = collX;

  if (collY.getTime() < minColl.getTime()) {
    minColl = collY;
  }
  if (collZ.getTime() < minColl.getTime()) {
    minColl = collZ;
  }

  // assert(minColl.getTime() > 0);

  return minColl;
}

// Alternativa 1 a WallCollsion
/*WallCollision calculateWallColl(Particle& p, double squareSide) {
  auto wallDistance = [&](double pos, double spd) -> double {
    if (spd < 0) {
      return -Particle::radius - pos;
    } else {
      return squareSide - Particle::radius - pos;
    }
  };

  auto calculateTime = [&](double position, double speed) {
    return wallDistance(position, speed) / speed;
  };

  double timeX{calculateTime(p.position.x, p.speed.x)};
  double timeY{calculateTime(p.position.y, p.speed.y)};
  double timeZ{calculateTime(p.position.z, p.speed.z)};

  double minTime{timeX};

  char wall = (p.speed.x < 0) ? 'l' : 'r';

  if (timeY < minTime) {
    minTime = timeY;
    wall = (p.speed.y < 0) ? 'f' : 'b';
  }

  if (timeZ < minTime) {
    minTime = timeZ;
    wall = (p.speed.z < 0) ? 'd' : 'u';
  }

  assert(minTime > 0);

  return {minTime, &p, wall};
}*/
// Alternativa 2 a WallCollsion
/*WallCollision calculateWallColl(Particle& p, double squareSide) {
    struct AxisData {
        double position;
        double speed;
        char negWall;
        char posWall;
    };

    AxisData axes[3] = {
        { p.position.x, p.speed.x, 'l', 'r' },
        { p.position.y, p.speed.y, 'f', 'b' },
        { p.position.z, p.speed.z, 'd', 'u' }
    };

    auto wallDistance = [&](double pos, double spd) {
        if (spd < 0.0) {
            return -Particle::radius - pos;
        } else {
            return squareSide - Particle::radius - pos;
        }
    };

    double minTime = INFINITY;
    char wall = '?';

    for (auto& axis : axes) {
        if (axis.speed == 0.0) {
            continue;
        }

        double distance = wallDistance(axis.position, axis.speed);
        double t = distance / axis.speed;

        // Consideriamo solo tempi positivi (collisioni future)
        if (t > 0.0 && t < minTime) {
            minTime = t;
            wall = (axis.speed < 0.0) ? axis.negWall : axis.posWall;
        }
    }

    assert(std::isfinite(minTime) && minTime > 0.0);

    return {minTime, &p, wall};
}*/
}  // namespace gasSim

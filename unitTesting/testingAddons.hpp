#ifndef TESTINGADDONS_HPP
#define TESTINGADDONS_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <thread>
#include <vector>

namespace GS {

struct Particle;

double collisionTime(Particle const& p1, Particle const& p2);

struct randomThreadsMgr {
  randomThreadsMgr() { threads.reserve(1000); }

  void finish();
  void add(std::function<void()> f);
  void start();

  size_t nThreads{0};
  std::vector<std::thread> threads{};
  std::atomic<bool> stop{true};
  std::atomic<bool> abort{false};
};
}  // namespace GS

#endif

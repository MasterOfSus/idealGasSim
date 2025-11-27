#ifndef TESTINGADDONS_HPP
#define TESTINGADDONS_HPP
#include <thread>
#include <functional>
#include "../gasSim/PhysicsEngine.hpp"

namespace GS {
	double collisionTime(Particle const& p1, Particle const& p2);

	struct randomThreadsMgr {
		std::vector<std::thread> threads {};
		void finish();
		void add(std::function<void()> f);
		void start();
		std::atomic<bool> stop {true};
		std::atomic<bool> abort {true};
	};
}

#endif

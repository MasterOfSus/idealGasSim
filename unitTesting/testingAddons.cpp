#include "testingAddons.hpp"
#include <random>
#include <stdexcept>

namespace GS {
	void randomThreadsMgr::add(std::function<void()> f) {
		threads.emplace_back(
			[this, &f] () {
				thread_local std::mt19937 r {std::random_device{}()};
				std::uniform_int_distribution<unsigned> d {0, 10000};
				while (stop.load()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
					if (abort.load()) {
						return;
					}
				}
				while (!stop.load()) {
					std::this_thread::sleep_for(std::chrono::milliseconds(d(r)));
					if (abort.load()) {
						return;
					}
					try {
						f();
					} catch (std::runtime_error const& e){
						abort.store(true);
					} catch (std::invalid_argument const& e) {
						abort.store(true);
					}
				}
			}
		);
	}
	void randomThreadsMgr::finish() {
		stop.store(true);
		for (std::thread& t: threads) {
			if (t.joinable()) {
				t.join();
			}
		}
	}
	void randomThreadsMgr::start() {
		stop.store(false);
	}
}



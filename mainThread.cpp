#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "gasSim/INIReader.h"
#include "gasSim/graphics.hpp"
#include "gasSim/input.hpp"
#include "gasSim/output.hpp"
#include "gasSim/physicsEngine.hpp"
#include "gasSim/statistics.hpp"

double gasSim::Particle::mass;
double gasSim::Particle::radius;

int main(int argc, const char* argv[]) {
  
	try {
		std::cerr << "Started." << std::endl;
    auto opts{gasSim::input::optParse(argc, argv)};
    if (gasSim::input::optControl(argc, opts)) {
      return 0;
    }

    std::string path = opts.count("config") != 0
                           ? opts["config"].as<std::string>()
                           : "configs/gasSim_demo.ini";
    INIReader cFile(path);
    if (cFile.ParseError() != 0) {
      throw std::runtime_error("Can't load " + path);
    }

    gasSim::Particle::mass = cFile.GetReal("gas", "particleMass", -1);
    gasSim::Particle::radius = cFile.GetReal("gas", "particleRadius", -1);

    int pNum = cFile.GetInteger("gas", "particleNum", -1);
    double temp = cFile.GetReal("gas", "temperature", -1);
    double side = cFile.GetReal("gas", "boxSide", -1);
    int iterNum = cFile.GetInteger("gas", "iterationNum", -1);
		int statsN = cFile.GetInteger("results", "statsN", 20);
    bool simult = cFile.GetBoolean("gas", "simultaneous", false);
    gasSim::Gas simulatedGas(pNum, temp, side);

    gasSim::SimOutput output{(unsigned)statsN, 60.};
    std::cout << "Simulation calculations (" << iterNum << ") are underway... \n";
    std::cout.flush();

    auto simLambda{[&simulatedGas, &output, iterNum]() {
      simulatedGas.simulate(iterNum, output);
    }};
    auto simStart = std::chrono::high_resolution_clock::now();
    std::thread simThread{simLambda};

		std::cerr << "Initialized sim thread." << std::endl;

    auto processLambda{	
			[&output]() {
				output.processData();
			}
		};
    auto start = std::chrono::high_resolution_clock::now();
    std::thread processThread{processLambda};

		std::cerr << "Initialized processing thread." << std::endl;

    std::vector<gasSim::TdStats> stats;
    auto statsLambda{[&stats, &output]() {
			while(true) {
				std::vector<gasSim::TdStats> tempStats {output.getStats(true)};
				stats.insert(stats.end(), tempStats.begin(), tempStats.end());
				if (output.isDone() && output.dataEmpty() && output.getStats().empty()) {
					break;
				}
			}
		}};
    std::thread statsThread{statsLambda};

		std::cerr << "Initialized stats thread." << std::endl;
  
		if (simThread.joinable()) {
			simThread.join();
			std::cerr << "Joined simThread." << std::endl;
		}
   	if (processThread.joinable()) {
			processThread.join();
			std::cerr << "Joined processThread." << std::endl;
		}
 	  if (statsThread.joinable()) {
			statsThread.join();
			std::cerr << "Joined statsThread." << std::endl;
		}

		std::cerr << "Got to threads end of life." << std::endl;

		std::cout << "Leftover rawData_ count: " << output.getData().size() << std::endl;
		std::cout << "Leftover stats_ count: " << output.getStats().size() << std::endl;

    auto simEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> simTime = simEnd - simStart;
    std::cout << "Simulation time: " << simTime.count()
              << " -> iterations per second: "
              << static_cast<double>(output.getData().size()) / simTime.count()
              << std::endl;
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> renderTime = stop - start;
    std::cout << "Data processing time: " << renderTime.count() << std::endl;
    std::cout << "Stats count: " << stats.size() << std::endl;

    gasSim::printInitData(pNum, temp, side, iterNum, simult);
    gasSim::printStat((const gasSim::TdStats&)stats.back());
    gasSim::printLastShit();

    return 0;

  } catch (const std::runtime_error& error) {
    std::cout << "RUNTIME ERROR: " << error.what() << std::endl;
    return 1;
  } catch (const std::invalid_argument& error) {
    std::cout << "INVALID ARGUMENT PROVIDED: " << error.what() << std::endl;
    return 1;
  } catch (const std::logic_error& error) {
    std::cout << "LOGIC ERROR: " << error.what() << std::endl;
    return 1;
  }
}

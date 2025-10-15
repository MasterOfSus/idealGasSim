#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <TMultiGraph.h>
#include <TGraph.h>

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
    gasSim::Gas myGas(pNum, temp, side);

    gasSim::SimOutput output{(unsigned)statsN, 60.};
    std::cout << "Simulation calculations (" << iterNum << ") are underway... \n";
    std::cout.flush();

    auto simLambda{[&myGas, &output, iterNum]() {
      myGas.simulate(iterNum, output);
    }};
    auto simStart = std::chrono::high_resolution_clock::now();
    std::thread simThread{simLambda};

		std::cerr << "Initialized sim thread." << std::endl;

		gasSim::PhysVectorF camPos {
			static_cast<gasSim::PhysVectorF>(
				gasSim::PhysVectorD{side * 2.f, side * 1.5f, side*0.75f}
			)
		};
		gasSim::PhysVectorF gasCenter {
			static_cast<gasSim::PhysVectorF>(
				gasSim::PhysVectorD{side * 0.5, side * 0.5, side * 0.5}
			)
		};

		gasSim::Camera camera {gasSim::Camera(
			camPos, gasCenter - camPos, 1.f, 90.f, 800, 600
		)};

   	sf::Texture texture;
		texture.loadFromFile("assets/lightBall.png");
    gasSim::RenderStyle style{texture};
		style.setWallsColor(sf::Color(130, 255, 144, 192));
		style.setBGColor(sf::Color::White);
    std::string options{"ufdl"};
    style.setWallsOpts(options);

    auto processLambda{
			[&output, &camera, &style]() {
				output.processData(camera, style, true);
			}
		};
    auto start = std::chrono::high_resolution_clock::now();
    std::thread processThread{processLambda};

		std::cerr << "Initialized processing thread." << std::endl;

    std::vector<sf::Texture> stats;
		
		TList graphs;
		TMultiGraph pGraphs;
		pGraphs.SetTitle("Pressure graphs.");
		for(int i {0}; i < 7; ++i) {
			pGraphs.Add(new TGraph());
		}
		TGraph kBGraph {};
		TGraph mfpGraph {};
		graphs.Add(&pGraphs);
		graphs.Add(&kBGraph);
		graphs.Add(&mfpGraph);
		sf::Texture placeholder;
		placeholder.loadFromFile("assets/placeholder.png");

    auto displayLambda{[&stats, &output, &graphs, &placeholder]() {
			while(true) {
				std::vector<sf::Texture> tempRndrs {
					output.getVideo(gasSim::VideoOpts::all,
					{1600, 600 * 10 / 9},
					placeholder, graphs, true)};

				stats.insert(stats.end(), tempRndrs.begin(), tempRndrs.end());
				if (output.isDone() && output.dataEmpty() && output.getStats().empty()) {
					break;
				}
			}
		}};
    std::thread statsThread{displayLambda};

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

		std::cerr << "Got to threads end of life. Starting drawing." << std::endl;

    sf::RenderWindow window(
        sf::VideoMode(camera.getWidth(), camera.getHeight()), "SFML Window",
        sf::Style::Default);
    window.setFramerateLimit(60);
	
		sf::Sprite sprt;
		int i {0};
    while (window.isOpen()) {
      // check all the window's events that were triggered since the last
      // iteration of the loop
      sf::Event event;

      // sf::sleep(sf::milliseconds(90));

      // std::cout << "Particles poss and speeds:\n";

      /*
                         * for (const gasSim::Particle& particle : particles) {
        gasSim::PhysVectorD vector = particle.position;
        // std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z
        //   << "), (";
        vector = particle.speed;
        // std::cout << vector.x << ", " << vector.y << ", " << vector.z <<
        // ")\n";
      }
                        */

      while (window.pollEvent(event)) {
        // "close requested" event: we close the window
        if (event.type == sf::Event::Closed) window.close();
      }
      if (i >= (int)stats.size() - 1) {
        std::string replay;
        std::cout << "Replay? ";
        std::cout.flush();
        std::cin >> replay;
        if (replay == "n")
          window.close();
        else
          i = 0;
      }
      window.clear(sf::Color::Yellow);
			sprt.setTexture(stats[i]);
      window.draw(sprt);
      window.display();

      ++i;
      // std::cout << iteration;
      // if (iteration >= 10) window.close();
    }


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

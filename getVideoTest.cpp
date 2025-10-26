#include <Rtypes.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <iterator>
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
		std::cout << "Started." << std::endl;
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
		int statsN = cFile.GetInteger("results", "statsN", 30);
    bool simult = cFile.GetBoolean("gas", "simultaneous", false);
    gasSim::Gas myGas(pNum, temp, side);

		TH1D hTemplate {"hTemplate", "Speeds histo", 40, 0., 40.};
		hTemplate.SetFillColor(kBlue);
    gasSim::SimOutput output{(unsigned)statsN, 60., hTemplate};
    std::cout << "Simulation calculations (" << iterNum << ") are underway... \n";
    std::cout.flush();

    auto simLambda{[&myGas, &output, iterNum]() {
      myGas.simulate(iterNum, output);
    }};
    auto simStart = std::chrono::high_resolution_clock::now();
    std::thread simThread{simLambda};

		std::cout << "Initialized sim thread." << std::endl;

		gasSim::PhysVectorF camPos {
			static_cast<gasSim::PhysVectorF>(
				gasSim::PhysVectorD{side * 1.5f, side * 1.25f, side*0.75f}
			)
		};
		gasSim::PhysVectorF gasCenter {
			static_cast<gasSim::PhysVectorF>(
				gasSim::PhysVectorD{side * 0.5, side * 0.5, side * 0.5}
			)
		};

		gasSim::Camera camera {gasSim::Camera(
			camPos, gasCenter - camPos, 1.f, 90.f, 600, 600
		)};

   	sf::Texture texture;
		texture.loadFromFile("assets/lightBall.png");
    gasSim::RenderStyle style{texture};
		style.setWallsColor(sf::Color(130, 255, 144, 192));
		style.setBGColor(sf::Color::White);
    std::string options{"ufdl"};
    style.setWallsOpts(options);

		auto processStart = std::chrono::high_resolution_clock::now();
    auto processLambda{
			[&output, &camera, &style]() {
				output.processData(camera, style, true);
			}
		};
    auto start = std::chrono::high_resolution_clock::now();
    std::thread processThread{processLambda};

		std::cout << "Initialized processing thread." << std::endl;

    std::vector<sf::Texture> video;
		
		TList graphs;
		TMultiGraph pGraphs;
		pGraphs.SetTitle("Pressure graphs.");
		for(int i {0}; i < 7; ++i) {
			TGraph* g = new TGraph();
			g->SetLineColor(i + 1);
			if (!i) {
				g->SetLineWidth(2);
			}
			pGraphs.Add(g);
		}
		TGraph kBGraph {};
		TGraph mfpGraph {};
		graphs.Add(&pGraphs);
		graphs.Add(&kBGraph);
		graphs.Add(&mfpGraph);
		sf::Texture placeholder;
		placeholder.loadFromFile("assets/placeholder.png");

		std::cout << "Initialized getVideo thread" << std::endl;
		auto displayStart = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> tempRndrsTime {}, insertTime {};
    auto displayLambda{[&video, &output, &graphs, &placeholder, &displayStart, &tempRndrsTime, &insertTime]() {
			// std::this_thread::sleep_for(std::chrono::milliseconds(100));
			std::cout << "Starting!!!!" << std::endl;
			displayStart = std::chrono::high_resolution_clock::now(); // this wasn't here before!!!!
			while(true) {
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				auto tempRndrsStart = std::chrono::high_resolution_clock::now();
				std::vector<sf::Texture> tempRndrs {
					output.getVideo(gasSim::VideoOpts::all,
					{1200, 600 * 10 / 9},
					placeholder, graphs, true)
				};
				tempRndrsTime += (std::chrono::duration<double>) (std::chrono::high_resolution_clock::now() - tempRndrsStart);

				auto insertStart = std::chrono::high_resolution_clock::now();
				video.reserve(tempRndrs.size());
				video.insert(video.end(),
						std::make_move_iterator(tempRndrs.begin()),
						std::make_move_iterator(tempRndrs.end()));
				insertTime += (std::chrono::duration<double>) (std::chrono::high_resolution_clock::now() - insertStart);
				if (output.isDone() && output.dataEmpty() && output.getStats().empty()) {
					std::cerr << "tempRndrsTime = " << tempRndrsTime.count() << '\n'
					<< "insertTime = " << insertTime.count() << std::endl;
					break;
				}
			}
		}};
    std::thread displayThread{displayLambda};

		std::cout << "Initialized stats thread." << std::endl;
  
    auto simEnd = std::chrono::high_resolution_clock::now();
		if (simThread.joinable()) {
			simThread.join();
    	simEnd = std::chrono::high_resolution_clock::now();
			std::cout << "Joined simThread, gasData number = " << output.getData().size() << std::endl;
		}
		auto processEnd = std::chrono::high_resolution_clock::now();
   	if (processThread.joinable()) {
			processThread.join();
			processEnd = std::chrono::high_resolution_clock::now();
			std::cout << "Joined processThread, renders number = " << output.getRenders().size() << ", stats number = " << output.getStats().size() << std::endl;
		}
		auto displayEnd = std::chrono::high_resolution_clock::now();
 	  if (displayThread.joinable()) {
			displayThread.join();
			displayEnd = std::chrono::high_resolution_clock::now();
			std::cout << "Joined getVideo thread." << std::endl;
		}

		std::cout << "Got to threads end of life. Starting drawing." << std::endl;

    sf::RenderWindow window(
        sf::VideoMode(1200, 600*10/9), "SFML Window",
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
      if (i >= (int)video.size() - 1) {
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
			sprt.setTexture(video[i]);
      window.draw(sprt);
      window.display();

      ++i;
      // std::cout << iteration;
      // if (iteration >= 10) window.close();
    }


		std::cout << "Leftover rawData_ count: " << output.getData().size() << std::endl;
		std::cout << "Leftover stats_ count: " << output.getStats().size() << std::endl;

    std::chrono::duration<double> simTime = simEnd - simStart;
    std::cout << "Simulation time: " << simTime.count()
              << " -> iterations per second: "
              << static_cast<double>(iterNum) / simTime.count()
              << std::endl;
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> renderTime = stop - start;
    std::cout << "Total elapsed time: " << renderTime.count() << std::endl;
    std::chrono::duration<double> processTime = processEnd - processStart;
    std::cout << "Frames count: " << video.size() << ", processing time = " << processTime.count() << ", fps = " << static_cast<double>(video.size()) / processTime.count() << std::endl;
    std::chrono::duration<double> displayTime = displayEnd - displayStart;
		std::cout << "Video composition time: " << displayTime.count() << ", fps = " << video.size() / displayTime.count() << std::endl;

    gasSim::printInitData(pNum, temp, side, iterNum, simult);
    // gasSim::printStat((const gasSim::TdStats&)video.back());
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

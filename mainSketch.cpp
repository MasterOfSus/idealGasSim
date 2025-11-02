#include <Rtypes.h>
#include <unistd.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <chrono>
#include <iostream>
#include <iterator>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <TMultiGraph.h>
#include <TGraph.h>
#include <TFile.h>

#include "gasSim/INIReader.h"
#include "gasSim/graphics.hpp"
#include "gasSim/input.hpp"
#include "gasSim/output.hpp"
#include "gasSim/physicsEngine.hpp"
#include "gasSim/statistics.hpp"

double gasSim::Particle::mass;
double gasSim::Particle::radius;

gasSim::PhysVectorD stovec(std::string s) {
	// checking for braces and erasing the first one
	if (s.front() != '{' || s.back() != '}') {
		throw std::invalid_argument("Missing opening and closing braces.");
	}
	s.erase(s.begin());

	std::string x1 {}, x2 {}, x3 {};
	enum coordinate {
		x, y, z, done
	};
	coordinate workingC {x};
	// removing spaces
	for (char c: s) {
		if (workingC == done) {
			return {
				std::stod(x1), std::stod(x2), std::stod(x3)
			};
		}
		if (c == ',') {
			switch (workingC) {
				case x:
					workingC = y;
					break;
				case y:
					workingC = z;
					break;
				case z:
				case done:
					throw std::invalid_argument("Too many commas.");
					break;
			}
		} else if (c == '}') {
			workingC = done;
		} else {
			switch (workingC) {
				case x:
					x1.push_back(c);
					break;
				case y:
					x2.push_back(c);
					break;
				case z:
					x3.push_back(c);
					break;
				case done:
					break;
			}
		}
	}
	return {
		std::stod(x1), std::stod(x2), std::stod(x3)
	};
}

gasSim::VideoOpts stovideoopts (std::string s) {
	if (s == "all") {
		return gasSim::VideoOpts::all;
	} else if (s == "gasPlusCoords") {
		return gasSim::VideoOpts::gasPlusCoords;
	} else if (s == "justStats") {
		return gasSim::VideoOpts::justStats;
	} else if (s == "justGas") {
		return gasSim::VideoOpts::justGas;
	} else {
		throw std::invalid_argument("String not corresponding to available videoOpt.");
	}
}

int main(int argc, const char* argv[]) {
	auto simStart = std::chrono::high_resolution_clock::now();
	try {
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

    gasSim::Particle::mass = cFile.GetReal("simulation parameters", "particleMass", 1);
    gasSim::Particle::radius = cFile.GetReal("simulation parameters", "particleRadius", 1);

    gasSim::Gas myGas(
			cFile.GetInteger("simulation parameters", "nParticles", -1),
			cFile.GetReal("simulation parameters", "targetT", -1),
			cFile.GetReal("simulation parameters", "boxSide", -1)
		);

		TFile inputFile {
			TFile(
				(
				std::ostringstream()
				<< "inputs/"
				<< cFile.Get("simulation parameters", "ROOTInputPath", "input").c_str()
				<< ".root"
				).str().c_str()
			)
		};
		std::cout << (std::ostringstream()
				<< "inputs/"
				<< cFile.Get("simulation parameters", "ROOTInputPath", "input").c_str()
				<< ".root"
				).str().c_str() << std::endl;
		if (inputFile.IsZombie()) {
			throw std::runtime_error("Provided a path not mapping to any ROOT file.");
		}

		TH1D speedsHTemplate {*((TH1D*) inputFile.Get("speedsHTemplate"))};
		if (speedsHTemplate.IsZombie()) {
			throw std::runtime_error("Couldn't find speedsHTemplate in input root file.");
		}

		std::cout << "Found speedsHTemplate." << std::endl;

		long nStats {cFile.GetInteger("simulation parameters", "nStats", 1)};
		if (nStats <= 0) {
			throw std::invalid_argument("Provided negative nStats.");
		}
		gasSim::SimOutput output {
			static_cast<unsigned>(
				nStats > 0?
				nStats:
				throw std::invalid_argument("Provided negative nStats.")
			),
			cFile.GetFloat("output", "framerate", 60.f),
			speedsHTemplate
		};
		gasSim::Gas gas {
			static_cast<int>(
				cFile.GetInteger("simulation parameters", "nParticles", 1)
			),
			cFile.GetFloat("simulation parameters", "targetT", 1),
			cFile.GetFloat("simulation parameters", "boxSide", 1.)
		};

		std::cout << "Starting simulation thread." << std::endl;

		double gasSide {gas.getBoxSide()};
		std::thread simThread {
			[&gas, &output, &cFile] {
				gas.simulate(cFile.GetInteger("simulation parameters", "nIters", 0), output);
			}
		};

		std::cout << "Started simulation thread." << std::endl;

		gasSim::PhysVectorF camPos {
			static_cast<gasSim::PhysVectorF>(
				stovec(
					cFile.Get(
						"rendering", "camPos",
						(std::ostringstream()
						<< "{"
						<< gasSide * 1.5 << ", "
						<< gasSide * 1.25 << ", "
						<< gasSide * 0.75 << '}').str()
					)
				)
			)
		};
		gasSim::PhysVectorF camSight {
			static_cast<gasSim::PhysVectorF>(
				stovec(
					cFile.Get(
						"rendering", "camSight",
						(std::ostringstream()
						<< "{"
						<< gasSide * 0.5 - camPos.x << ", "
						<< gasSide * 0.5 - camPos.y << ", "
						<< gasSide * 0.5 - camPos.z << '}').str()
					)
				)
			)
		};

		std::cout << "Made camera vectors." << std::endl;

		sf::Vector2i windowSize {
			static_cast<int>(cFile.GetInteger("output", "videoResX", 800)),
			static_cast<int>(cFile.GetInteger("output", "videoResY", 600))
		};

		gasSim::Camera camera {
			camPos, camSight,
			cFile.GetFloat("render", "camLength", 1.f),
			cFile.GetFloat("render", "fov", 90.f),
			windowSize.x / 2, windowSize.y * 9 / 10
		};

		std::cout << "Starting textures loading." << std::endl;

   	sf::Texture particleTex;
		particleTex.loadFromFile("assets/" + cFile.Get("render", "particleTexPath", "lightBall") + ".png");
    gasSim::RenderStyle style{particleTex};
		style.setWallsColor(
			sf::Color(cFile.GetInteger("render", "wallsColor", 0x50fa7b80))
		);
		style.setBGColor(
			sf::Color(cFile.GetInteger("render", "gasBgColor", 0xffffffff))
		);
    style.setWallsOpts(
			cFile.Get("render", "wallsOpts", "ufdl")
		);

		bool mfpMemory {
			cFile.GetBoolean("output", "mfpMemory", true)
		};

		const int frameTimems {static_cast<int>(1000./output.getFramerate())};

		std::cout << "Initializing processing thread." << std::endl;

		std::thread processThread {
			[&output, &camera, &style, mfpMemory] {
				output.processData(
					camera, style, mfpMemory
				);
			}
		};

		std::cout << "Initialized processing thread." << std::endl;

		TList* graphsList = new TList();
		TMultiGraph* pGraphs = (TMultiGraph*) inputFile.Get("pGraphs");
		TGraph* kBGraph = (TGraph*) inputFile.Get("kBGraph");
		TGraph* mfpGraph = (TGraph*) inputFile.Get("mfpGraph");
		if (pGraphs->IsZombie() || kBGraph->IsZombie() || mfpGraph->IsZombie()) {
			throw std::runtime_error("Couldn't find one or more graphs in provided root file.");
		}
		graphsList->Add(pGraphs);
		graphsList->Add(kBGraph);
		graphsList->Add(mfpGraph);
		std::cout << "Extracted graphsList." << std::endl;
		gasSim::VideoOpts videoOpt {
			stovideoopts(cFile.Get("output", "videoOpt", "all"))
		};
		std::cout << "Extracted videoOpts." << std::endl;
		sf::Texture placeHolder;
		placeHolder.loadFromFile(
			(std::ostringstream()
			 << "assets/"
			 << cFile.Get("render", "placeHolderName", "placeholder")
			 << ".png").str().c_str()
		);
		std::cout << "Loaded placeholder." << std::endl;

		while (!output.isProcessing()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

		if (cFile.GetBoolean("output", "saveVideo", "false")) {
		
			std::ostringstream cmd {};
			cmd << "ffmpeg -y -f rawvideo -pixel_format rgba -video_size "
				<< windowSize.x << "x" << windowSize.y
				<< " -framerate " << output.getFramerate()
				<< " -i - -c:v libx264 -pix_fmt yuv420p "
				<< "outputs/videos/"
				<< cFile.Get("output", "videoOutputName", "output")
				<< ".mp4";

			std::cout << "Trying to open ffmpeg pipe." << std::endl;
			
			FILE* ffmpeg = popen(
				cmd.str().c_str(),
				"w"
			);
			if (!ffmpeg) {
				throw std::runtime_error("Failed to open ffmpeg pipe.");
			}
			bool lastBatch {false};
			sf::Image auxImg;
			auxImg.create(windowSize.x, windowSize.y);
			while (true) {
				std::vector<sf::Texture> frames {};
				if (output.isDone() && output.dataEmpty() && !output.isProcessing()) {
					lastBatch = true;
				}
				frames.reserve(output.getFramerate() * 10);
				std::vector<sf::Texture> v = {output.getVideo(
					videoOpt, windowSize, placeHolder, *graphsList, true
				)};
				frames.insert(
					frames.end(),
					std::make_move_iterator(v.begin()),
					std::make_move_iterator(v.end())
				);
				for (sf::Texture& t: frames) {
					auxImg = t.copyToImage();
					fwrite(auxImg.getPixelsPtr(), 1, windowSize.x * windowSize.y * 4, ffmpeg);
				}
				if (lastBatch) {
					break;
				}
			}
		} else { // no permanent video output requested -> wiew-once real time video
			sf::RenderWindow window(
				sf::VideoMode(windowSize.x, windowSize.y), "GasSim Display",
				sf::Style::Default
			);
			std::mutex windowMtx;
			window.setFramerateLimit(60);
			window.setActive(false);
			std::atomic<bool> stopBufferLoop {false};
			std::atomic<bool> bufferKillSignal {false};
			std::cout << "Reached video output loop." << std::endl;
			std::atomic<int> queueNumber {1};
			std::atomic<int> launchedPlayThreadsN {0};
			auto playLambda {
				[&window, &windowMtx, &stopBufferLoop, &queueNumber, &launchedPlayThreadsN, frameTimems]
				(std::shared_ptr<std::vector<sf::Texture>> rPtr,
				 int threadN) {
					sf::Sprite auxS;
					while (threadN != queueNumber) {
						std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
					}
					std::cout << "Found queue number " << queueNumber << " equal to thread number " << threadN << ", stopping buffer loop." << std::endl;
					stopBufferLoop = true;
					std::lock_guard<std::mutex> windowGuard {windowMtx};
					window.setActive();
					for (const sf::Texture& r: *rPtr) {
						if (window.isOpen()) {
							auxS.setTexture(r);
							window.draw(auxS);
							window.display();
						}
					}
					queueNumber++;
					if (launchedPlayThreadsN == threadN) {
						std::cout << "Found playThreads number " << launchedPlayThreadsN << " still equal to thread number " << threadN << ": starting up buffering loop." << std::endl;
						stopBufferLoop = false;
					}
					window.setActive(false);
				}
			};
			std::thread bufferingLoop {
				[&stopBufferLoop, &placeHolder, &window, &windowMtx, &bufferKillSignal, frameTimems] () {
					sf::Sprite auxS {placeHolder};
					while (true) {
						if (!stopBufferLoop) {
							std::cout << "Found buffer loop activation signal." << std::endl;
							std::lock_guard<std::mutex> windowGuard {windowMtx};
							window.setActive();
							while (window.isOpen() && !stopBufferLoop) {
								window.clear(sf::Color::Red);
								window.draw(auxS);
								window.display();
								if (bufferKillSignal) {
									break;
								}
							}
							if (!window.isOpen()) {
								window.setActive(false);
								break;
							}
							if (bufferKillSignal) {
								break;
							}
							std::cout << "Buffering loop turned off." << std::endl;
							window.setActive(false);
						} else {
							std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
						}
						if (bufferKillSignal) {
							break;
						}
					}
				}
			};
			bool lastBatch {false};
			double targetBufferTime {
				cFile.GetReal("output", "targetBuffer", 3.)
			};
			while (true) {
				std::shared_ptr<std::vector<sf::Texture>> rendersPtr {std::make_shared<std::vector<sf::Texture>>()};
				rendersPtr->reserve(output.getFramerate() * 10);
				while (rendersPtr->size() <= output.getFramerate() * targetBufferTime) {
					std::vector<sf::Texture> v = {output.getVideo(
						videoOpt, windowSize, placeHolder, *graphsList, true
					)};
					size_t vSize = v.size();
					std::cerr << "Found v size = " << vSize << std::endl;
					rendersPtr->insert(
						rendersPtr->end(),
						std::make_move_iterator(v.begin()),
						std::make_move_iterator(v.end())
					);
					if (output.isDone() && output.dataEmpty() && !output.isProcessing()) {
						std::vector<sf::Texture> v = {output.getVideo(
							videoOpt, windowSize, placeHolder, *graphsList, true
						)};
						rendersPtr->insert(
							rendersPtr->end(),
							std::make_move_iterator(v.begin()),
							std::make_move_iterator(v.end())
						);
						lastBatch = true;
						std::cout << "Renders pointer size = " << rendersPtr->size() << " yielding " << rendersPtr->size() * frameTimems / 1000. << " seconds of video. Time = " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - simStart).count() << std::endl;
						break;
					} else {
						if (v.size() == 0) {
							std::cerr << "Output isDone() = " << output.isDone() << ", output.dataEmpty() = " << output.dataEmpty() << ", output.isProcessing() = " << output.isProcessing() << std::endl;
						}
					}
					std::cout << "Renders pointer size = " << rendersPtr->size() << " yielding " << rendersPtr->size() * frameTimems / 1000. << " seconds of video. Time = " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - simStart).count() << std::endl;
				}

				if (!lastBatch) {
					std::cout << "Sending playthread n. " << 1 + launchedPlayThreadsN << ", time = " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - simStart).count() << std::endl;
					std::thread playThread {
						[playLambda, rendersPtr, &launchedPlayThreadsN] {
							++launchedPlayThreadsN;
							playLambda(rendersPtr, launchedPlayThreadsN);
						}
					};
					playThread.detach();
				} else {
					std::cout << "Sending last playthread at n. " << 1 + launchedPlayThreadsN << ", time = " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - simStart).count() << std::endl;
					std::thread playThread {
						[playLambda, rendersPtr, &launchedPlayThreadsN] {
							++launchedPlayThreadsN;
							playLambda(rendersPtr, launchedPlayThreadsN);
						}
					};
					std::cout << "Joining playThread." << std::endl;
					playThread.join();
					std::lock_guard<std::mutex> windowGuard {windowMtx};
					bufferKillSignal = true;
					break;
				}
			}
			std::cout << "Reached main loop end." << std::endl;
			stopBufferLoop = true;
			std::cout << "Set buffer loop to stop." << std::endl;
			bufferingLoop.join();
			window.close();
		}

		std::cout << "Joining simulation and processing threads." << std::endl;

		if (simThread.joinable()) {
			simThread.join();
		}
		if (processThread.joinable()) {
			processThread.join();
		}

		graphsList->Delete();
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

#include <Rtypes.h>
#include <unistd.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <chrono>
#include <iostream>
#include <iterator>
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

gasSim::VideoOpts stovopts (std::string s) {
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
				cFile.Get("simulation parameters", "ROOTInputPath", "defaultInput").c_str()
			)
		};
		if (inputFile.IsZombie()) {
			throw std::runtime_error("Provided a path not mapping to any ROOT file.");
		}

		TH1D speedsHTemplate {*((TH1D*) inputFile.Get("speedsHTemplate"))};
		if (speedsHTemplate.IsZombie()) {
			throw std::runtime_error("Couldn't find speedsHTemplate in input root file.");
		}

		long nStats {cFile.GetInteger("simulation parameters", "nStats", 1)};
		if (nStats <= 0) {
			throw std::invalid_argument("Provided negative nStats.");
		}
		gasSim::SimOutput output {
			static_cast<unsigned>(
				cFile.GetInteger("simulation parameters", "nStats", 1) > 0?
				cFile.GetInteger("simulation parameters", "nStats", 1):
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

		std::thread simThread {
			[&gas, &output, &cFile] {
				gas.simulate(cFile.GetInteger("simulation parameters", "nIters", 0), output);
			}
		};

		double gasSide {gas.getBoxSide()};
		gasSim::PhysVectorF camPos {
			static_cast<gasSim::PhysVectorF>(
				stovec(
					cFile.Get(
						"rendering", "camPos",
						(std::ostringstream("{")
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
						(std::ostringstream("{")
						<< gasSide * 0.5 - camPos.x << ", "
						<< gasSide * 0.5 - camPos.y << ", "
						<< gasSide * 0.5 - camPos.z << '}').str()
					)
				)
			)
		};
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

   	sf::Texture particleTex;
		particleTex.loadFromFile("assets/" + cFile.Get("render", "particleTexPath", "lightBall.png"));
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

		std::thread processThread {
			[&output, &camera, &style, mfpMemory] {
				output.processData(
					camera, style, mfpMemory
				);
			}
		};

		TList* graphsList = nullptr;
		inputFile.GetObject("graphsList", graphsList);
		if (!graphsList) {
			throw std::runtime_error("Couldn't find graphs list in provided input file.");
		}
		gasSim::VideoOpts videoOpt {
			stovopts(cFile.Get("output", "videoOpt", "all"))
		};
		sf::Texture placeHolder;
		placeHolder.loadFromFile(
			cFile.Get("render", "placeHolderName", "placeholder.png")
		);
		
		std::ostringstream cmd {};
		cmd << "ffmpeg -y -f rawvideo -pixel_format rgba -video_size "
			<< windowSize.x << "x" << windowSize.y
			<< " -framerate " << output.getFramerate()
			<< " -i - -c:v libx264 -pix_fmt yuv420p "
			<< cFile.Get("output", "videoOutputName", "output")
			<< ".mp4";
		FILE* ffmpeg = popen(
			cmd.str().c_str(),
			"w"
		);
		if (!ffmpeg) {
			throw std::runtime_error("Failed to open ffmpeg pipe.");
		}
		std::mutex ffmpegMtx;

		sf::RenderWindow window(
			sf::VideoMode(windowSize.x, windowSize.y), "GasSim Display",
			sf::Style::Default
		);
		std::mutex windowMtx;
		std::atomic<bool> stopBufferLoop {false};
		auto loopVideo {[&stopBufferLoop, &placeHolder, &window, &windowMtx, frameTimems] () {
			sf::Sprite auxS {placeHolder};
			while (true) {
				if (!stopBufferLoop) {
					std::lock_guard<std::mutex> windowGuard {windowMtx};
					while (window.isOpen()) {
						if (!stopBufferLoop) {
							sf::Event e;
							while (window.pollEvent(e)) {
								if (e.type == sf::Event::Closed) {
									window.close();
								}
							}
							window.clear(sf::Color::Red);
							window.draw(auxS);
							window.display();
						} else {
							break;
						}
					}
					if (!window.isOpen()) {
						break;
					}
				} else {
					std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
				}
			}
		}};
		std::atomic<int> queueNumber {1};
		int playThreadN {0};
		auto playLambda {
			[&window, &windowMtx, &stopBufferLoop, &queueNumber, frameTimems]
			(std::shared_ptr<std::vector<sf::Texture>> rPtr,
			 std::shared_ptr<std::mutex> rPtrMtx,
			 std::atomic<bool>& displayStatus,
			 int threadN) {
				std::lock_guard<std::mutex> rPtrsGuard {*rPtrMtx};
				sf::Sprite auxS;
				while (threadN != queueNumber) {
					std::this_thread::sleep_for(std::chrono::milliseconds(frameTimems));
				}
				stopBufferLoop = true;
				std::lock_guard<std::mutex> windowGuard {windowMtx};
				for (const sf::Texture& r: *rPtr) {
					if (window.isOpen()) {
						sf::Event e;
						while (window.pollEvent(e)) {
							if (e.type == sf::Event::Closed) {
								window.close();
							}
						}
						auxS.setTexture(r, true);
						window.draw(auxS);
						window.display();
					}
				}
				displayStatus = true;
				stopBufferLoop = false;
				queueNumber++;
			}
		};
		auto encodeLambda {
			[&ffmpeg, &ffmpegMtx, windowSize]
			(std::shared_ptr<std::vector<sf::Texture>> rPtr,
			std::shared_ptr<std::mutex> rMtxPtr,
			std::atomic<bool>& displayStatus) {
				sf::Context context;
				std::lock_guard<std::mutex> ffmpegGuard {ffmpegMtx};
				sf::Image auxImg;
				auxImg.create(windowSize.x, windowSize.y);
				while (!displayStatus) {
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}
				std::lock_guard<std::mutex> rPtrGuard {*rMtxPtr};
				for (sf::Texture& t: *rPtr) {
					auxImg = t.copyToImage();
					fwrite(auxImg.getPixelsPtr(), 1, windowSize.x * windowSize.y * 4, ffmpeg);
				}
				rPtr->clear();
			}
		};
		window.setFramerateLimit(60);
		std::thread bufferingLoop {loopVideo};
		bufferingLoop.detach();
		bool lastBatch {false};
		std::atomic<bool> doneDisplaying {false};
		std::atomic<bool> doneEncoding {false};
		while (true) {
			std::shared_ptr<std::vector<sf::Texture>> rendersPtr {std::make_shared<std::vector<sf::Texture>>()};
			std::shared_ptr<std::mutex> rendersMtxPtr {std::make_shared<std::mutex>()};
			std::atomic<bool> displayDone {false};
			rendersPtr->reserve(output.getFramerate() * 10);
			while (rendersPtr->size() <= output.getFramerate() * 5) {
				std::vector<sf::Texture> v = {output.getVideo(
					videoOpt, windowSize, placeHolder, *graphsList, true
				)};
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
					break;
				}
			}
			++playThreadN;

			if (!lastBatch) {
				std::thread playThread {
					[playLambda, &rendersPtr, &rendersMtxPtr, &displayDone, playThreadN] {
						playLambda(rendersPtr, rendersMtxPtr, displayDone, playThreadN);
					}
				};
				playThread.detach();
			
				std::thread encodeThread {
					[encodeLambda, &rendersPtr, &rendersMtxPtr, &displayDone] {
						encodeLambda(rendersPtr, rendersMtxPtr, displayDone);
					}
				};
				encodeThread.detach();
			} else {
				std::thread playThread {
					[playLambda, &rendersPtr, &rendersMtxPtr, &displayDone, playThreadN, &doneDisplaying] {
						playLambda(rendersPtr, rendersMtxPtr, displayDone, playThreadN);
						doneDisplaying = true;
					}

				};
				playThread.detach();
			
				std::thread encodeThread {
					[encodeLambda, &rendersPtr, &rendersMtxPtr, &displayDone, &doneEncoding] {
						encodeLambda(rendersPtr, rendersMtxPtr, displayDone);
						doneEncoding = true;
					}
				};
				encodeThread.detach();
				break;
			}
		}

		std::thread displayEnder {
			[&doneDisplaying, &window, &windowMtx] {
				while (!doneDisplaying) {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
				std::lock_guard<std::mutex> windowGuard {windowMtx};
				window.close();
			}
		};
		std::thread encodeEnder {
			[&doneEncoding, &ffmpeg, &ffmpegMtx] {
				while (!doneEncoding) {
					std::this_thread::sleep_for(std::chrono::milliseconds(500));
				}
				std::lock_guard<std::mutex> ffmpegGuard {ffmpegMtx};
				pclose(ffmpeg);
			}
		};

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

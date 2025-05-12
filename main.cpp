#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>
// #include <set>

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
    auto opts{gasSim::input::optParse(argc, argv)};

    /*std::cout << "opts arguments" << std::endl
              << opts.arguments_string() << std::endl;
    std::cout << "argc = " << argc << std::endl;
    // std::cout << "argv =" << argv << std::endl;*/
    if (argc == 1 || opts["help"].as<bool>() || opts["usage"].as<bool>()) {
      return 0;
    } else if (opts.count("tCoords") == 0 && opts.count("kBoltz") == 0 &&
               opts.count("pdfSpeed") == 0 && opts.count("freePath") == 0 &&
               opts.count("collNeg") == 0) {
      throw std::invalid_argument(
          "At least one of the base options (-t,-k,-p,-f,n) has to be "
          "called.");
    }  // maybe this option control can be implemented as a separate function

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
    bool simult = cFile.GetBoolean("gas", "simultaneous", false);
    gasSim::Gas simulatedGas(pNum, temp, side);

    // int nIter = cFile.GetInteger("gas", "iterationNum", -1);
    // gasSim::TdStats simProducts = simulatedGas.simulate(nIter);

    // render stuff
    gasSim::PhysVectorF focus{20., 7., 12.};
    gasSim::PhysVectorF center{5., 5., 5.};
    gasSim::Camera camera(focus, center - focus, 2., 90., 800, 600);

    sf::RenderTexture photo;
    photo.create(camera.getWidth(), camera.getHeight());
    sf::CircleShape shape{1.f};
    shape.setFillColor({193, 130, 255});
    shape.setOutlineColor({41, 27, 56});
    shape.setOutlineThickness(10.);
    gasSim::RenderStyle style{shape};
		style.setWallsColor(sf::Color(130, 255, 144, 192));
		style.setBGColor(sf::Color::White);
    std::string options{"udlfb"};
    style.setWallsOpts(options);
    sf::Sprite picture;
    picture.setTexture(photo.getTexture());

    sf::RenderWindow window(
        sf::VideoMode(camera.getWidth(), camera.getHeight()), "SFML Window",
        sf::Style::Default);
    window.setFramerateLimit(60);

		gasSim::SimOutput output {(unsigned int) iterNum, 60.};

		simulatedGas.simulate(iterNum, output);

		output.processData(camera, true, style);

		std::cout << "Started transferring renders... ";
		std::cout.flush();
		std::vector<sf::Texture> renders {output.getRenders()};
		std::cout << "done!\n";
		std::cout << "Renders count: " << renders.size() << std::endl;
		std::vector<gasSim::TdStats> stats {output.getStats()};
		std::cout << "Stats count: " << stats.size() << std::endl;

    int i{0};

		std::vector<sf::Sprite> renderSprites {};

		std::cout << "Started transferring textures to sprites... ";
		std::cout.flush();
		for (const sf::Texture& t: renders) {
			renderSprites.emplace_back(t);
		}
		std::cout << "done!\n";

    // run the program as long as the window is open
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
			if (i >= (int) renderSprites.size() - 1) {
				std::string replay;
				std::cout << "Replay? ";
				std::cout.flush();
				std::cin >> replay;
				if (replay == "n")
					window.close();
				else i = 0;
			}
			window.clear(sf::Color::Yellow);
      window.draw(renderSprites[i]);
      window.display();

			++i;
      // std::cout << iteration;
      // if (iteration >= 10) window.close();
    }
    // std::cout << "\n";

    /*
    std::cout << "pressure: " << simProducts.getPressure() << std::endl;
    std::cout << "temperature: " << simProducts.getTemp() << std::endl;
    std::cout << "volume: " << simProducts.getVolume() << std::endl;
    std::cout << "n particles: " << simProducts.getNParticles() << std::endl;*/
    gasSim::printInitData(pNum, temp, side, iterNum, simult);
    // gasSim::printStat(simProducts);
    gasSim::printStat((const gasSim::TdStats&) stats.back());
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

  // std::cout << "bombardini goosini" << std::endl << std::endl;
}

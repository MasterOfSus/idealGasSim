#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>
// #include <set>
#include "INIReader.h"
#include "input.hpp"
#include "physicsEngine.hpp"
#include "statistics.hpp"

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
                           : "gasSim_demo.ini";
    INIReader cFile(path);
    if (cFile.ParseError() != 0) {
      throw std::runtime_error("Can't load " +
                               path);  // will become "Can't load selected file"
                                       // if + path doesn't work
    }

    gasSim::Particle::mass = cFile.GetReal("gas", "particleMass", -1);
    gasSim::Particle::radius = cFile.GetReal("gas", "particleRadius", -1);

    int pNum = cFile.GetInteger("gas", "particleNum", -1);
    double temp = cFile.GetReal("gas", "temperature", -1);
    double side = cFile.GetReal("gas", "boxSide", -1);
    gasSim::Gas pablo(pNum, temp, side);

    int nIter = cFile.GetInteger("gas", "iterationNum", -1);
    gasSim::TdStats enrico = pablo.simulate(nIter);

    std::cout << enrico.getPressure() << std::endl;
    std::cout << enrico.getTemp() << std::endl;
    std::cout << enrico.getVolume() << std::endl;
    std::cout << enrico.getNParticles() << std::endl;

    return 0;

  } catch (const std::exception& error) {
    std::cout << error.what() << std::endl;
    return 1;
  }

  // std::cout << "bombardini goosini" << std::endl << std::endl;
}
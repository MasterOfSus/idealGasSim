#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>
// #include <set>

#include "input.hpp"
// #include "physicsEngine.hpp"

int main(int argc, const char* argv[]) {
  auto opts{gasSim::input::optParse(argc, argv)};

  std::cout << "opts arguments" << std::endl
            << opts.arguments_string() << std::endl;
  std::cout << "argc = " << argc << std::endl;
  // std::cout << "argv =" << argv << std::endl;
  if (argc == 1 || opts["help"].as<bool>() || opts["usage"].as<bool>()) {
    return 0;
  } /*else if (opts.count("demo") != 0 && opts.count("config") != 0) {
    throw std::invalid_argument(
        "Options --demo and --config are mutually exclusive.");
  }  // maybe this option control can be implemented as a separate function*/
  std::cout << "bombardini goosini" << std::endl << std::endl;
}
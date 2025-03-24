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
  if (argc == 1 || opts.count("help") || opts.count("usage")) {
    return 0;
  };
}
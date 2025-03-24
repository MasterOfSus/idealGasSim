#include "input.hpp"

#include <iostream>

namespace gasSim {
namespace input {

cxxopts::ParseResult optParse(int argc, const char* argv[]) {
  cxxopts::Options options(
      "idealGasSim",
      "Physics simulation of a particle gas determined only by kinetik energy");
  options.add_options("base")("h,help", "Print the list of commands")(
      "usage", "Prints the list of commands and their description");

  options.add_options("gas")(
      "g,gae", "print gae",
      cxxopts::value<int>());  // options concerning the gas
                               // and the physics engine
  options.add_options("results")(
      "e,eli", "print eli",
      cxxopts::value<int>());  // options concerning values/graphs related
                               // to the gas state
  options.add_options("graphics")(
      "a,ale", "print ale",
      cxxopts::value<int>());  // options concerning the visualization of
                               // the gas using SFML

  auto result = options.parse(argc, argv);

  // implementing no options case as "display help message"
  std::vector<std::string> helpOpt{"base", "gas", "results", "graphics"};
  if (argc == 1 || result.count("help")) {
    std::cout << options.help(helpOpt, bool{false}) << std::endl;
  }  // CHECK here for possible problems while building with
     // cmake, could be because argc is not always ==1 if the
     // program is launched w-out options
  else if (result.count("usage")) {  // check cxxopts.hpp line 2009 if you
                                     // want to implement --usage as only
                                     // options w-out the help message
    std::cout << options.help(helpOpt) << std::endl;
  }

  return result;
}

}  // namespace input

}  // namespace gasSim

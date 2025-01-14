#include "input.hpp"

#include <iostream>

namespace gasSim {
namespace input {

cxxopts::ParseResult optParse(int argc, const char* argv[]) {
  std::string helpOpts{"base,gas,results,graphics"};
  cxxopts::Options options(
      "idealGasSim",
      "Physics simulation of a particle gas determined only by kinetik energy");
  options.add_options("base")(
      "h,help", "Print the list of commands",
      cxxopts::value<std::vector<std::string>>()->implicit_value(helpOpts))(
      "usage", "Prints the list of commands and their description",
      cxxopts::value<std::vector<std::string>>()->implicit_value(helpOpts));

  options.add_options("gas");       // options concerning the gas and the
                                    // physics engine
  options.add_options("results");   // options concerning values/graphs related
                                    // to the gas state
  options.add_options("graphics");  // options concerning the visualization of
                                    // the gas using SFML

  auto result = options.parse(argc, argv);

  // implementing no options case as "display help message"
  if (argc ==
      1) {  // CHECK here for possible problems while building with cmake
    std::vector<std::string> helpOpt{"base", "gas", "results", "graphics"};
    std::cout << options.help(helpOpt) << std::endl;
  } else if (result.count("help")) {
    std::cout << options.help(result["help"].as<std::vector<std::string>>(),
                              bool{false})
              << std::endl;
  } else if (result.count("usage")) {
    std::cout << options.help(result["usage"].as<std::vector<std::string>>())
              << std::endl;
  }

  return result;
}

}  // namespace input

}  // namespace gasSim

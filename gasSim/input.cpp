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

<<<<<<< HEAD
  options.add_options(
      "gas")(  // options concerning the gas and the physics engine
      "d,demo", "Starts the gas simulation from predefined data")(
      "c,config", "Starts the gas simulation from a configuration file",
      cxxopts::value<std::string>())(  // value type needs to be changed
                                       // according to text file input handling
      "particleNum", "Sets the number of particles to a user defined value",
      cxxopts::value<int>())("temperature",
                             "Sets the temperature to a user defined value",
                             cxxopts::value<double>())(
      "boxSide",
      "Sets the side of the container lenght to a user defined value",
      cxxopts::value<double>())(
      "simultaneous",
      "Enables check for possible multi-particle/contemporary "
      "collisions");

  options.add_options(
      "results")(  // options concerning values/graphs related to the gas state
      "g,graphs",
      "Enables production and display of histograms on a new window")(
      "p,photo",
      "Enables production and display of a gas photo on a new window")(
      "s,save",
      "Saves produced data (including possible images) as file with user "
      "defined name",
      cxxopts::value<std::string>());  // value type needs to be changed
                                       // according to file generation handling
  options.add_options("graphics")(  // options concerning the visualization of
                                    // the gas using SFML
      "a,ale", "print ale", cxxopts::value<int>());
=======
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
>>>>>>> 2f357739908d770d22fb96b5058254c5400c5ef5

  auto result = options.parse(argc, argv);

  // implementing no options case as "display help message"
  std::vector<std::string> helpOpt{"base", "gas", "results", "graphics"};
<<<<<<< HEAD
  if (argc == 1 || result["help"].as<bool>()) {
=======
  if (argc == 1 || result.count("help")) {
>>>>>>> 2f357739908d770d22fb96b5058254c5400c5ef5
    std::cout << options.help(helpOpt, bool{false}) << std::endl;
  }  // CHECK here for possible problems while building with
     // cmake, could be because argc is not always ==1 if the
     // program is launched w-out options
<<<<<<< HEAD
  else if (result["usage"].as<bool>()) {  // check cxxopts.hpp line 2009 if you
                                          // want to implement --usage as only
                                          // options w-out the help message
=======
  else if (result.count("usage")) {  // check cxxopts.hpp line 2009 if you
                                     // want to implement --usage as only
                                     // options w-out the help message
>>>>>>> 2f357739908d770d22fb96b5058254c5400c5ef5
    std::cout << options.help(helpOpt) << std::endl;
  }

  return result;
}

}  // namespace input

}  // namespace gasSim

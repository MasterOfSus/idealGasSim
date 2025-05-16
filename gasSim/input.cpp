#include "input.hpp"

#include <iostream>

namespace gasSim {
namespace input {

cxxopts::ParseResult optParse(int argc, const char* argv[]) {
  cxxopts::Options options(
      "idealGasSim",
      "Physics simulation of a particle gas determined only by kinetik energy");
  options.add_options("base")(
      "h,help", "Print the list of commands and their description") /*(
"usage", "Prints the list of commands and their description with instructions
regarding how to use them \n")*/
      ("c,config", "Starts the gas simulation from a configuration file \n",
       cxxopts::value<std::string>())(
          "t,tCoords",
          "Measures thermodynamic coordinates of the simulated gas")(
          "k,kBoltz", "Measures Boltzman's constant from the simulated gas")(
          "p,pdfSpeed", "Generates a PDF of the speed that each particle has")(
          "f,freePath", "Measures the mean free path of the particles")(
          "n,collNeg",
          "Generates another gas without collisions between particles to show "
          "their negligibility");

  options.add_options(
      "gas")  // options concerning the gas and the physics engine
      ("particleNum", "Sets the number of particles to a user defined value",
       cxxopts::value<int>())("pMass",
                              "Sets the particle mass to a user defined value",
                              cxxopts::value<double>())(
          "pRadius", "Sets the particle radius to a user defined value",
          cxxopts::value<double>())(
          "temperature", "Sets the temperature to a user defined value",
          cxxopts::value<double>())(
          "boxSide",
          "Sets the side of the container lenght to a user defined value",
          cxxopts::value<double>())(
          "iterationNum",
          "Sets the number of iterations simulated to a user defined value",
          cxxopts::value<int>())(
          "simultaneous",
          "Enables check for possible multi-particle/contemporary "
          "collisions");

  options.add_options(
      "results")(  // options concerning values/graphs related to the gas state
      "g,graphs",
      "Enables production and display of histograms on a new window")(
      "r,render",
      "Enables production and display of a gas render on a new window")(
      "s,save",
      "Saves produced data (including possible images) as file with user "
      "defined name",
      cxxopts::value<std::string>());  // value type needs to be changed
                                       // according to file generation handling
  options.add_options("render")(  // options concerning the visualization of
                                  // the gas using SFML
      "a,ale", "print ale", cxxopts::value<int>());

  auto result = options.parse(argc, argv);

  // implementing no options case as "display help message"
  std::vector<std::string> helpOpt{"base", "gas", "results", "graphics"};
  if (argc == 1 || result["help"].as<bool>()) {
    std::cout << options.help(helpOpt, bool{false}) << std::endl;
  }  // CHECK here for possible problems while building with
     // cmake, could be because argc is not always ==1 if the
     // program is launched w-out options

  /*else if (result["usage"].as<bool>()) {  // check cxxopts.hpp line 2009 if you
                                          // want to implement --usage as only
                                          // options w-out the help message

    std::cout
        << options.help(helpOpt)
        << std::endl;  // check cxxopts.hpp line 2009 if you want to implement
                       // --usage as only options w-out the help message
  }*/

  return result;
}

bool optControl(int argc, cxxopts::ParseResult& opts) {
  if (argc == 1 || opts["help"].as<bool>() /*|| opts["usage"].as<bool>()*/) {
    return true;
  } else if (opts.count("tCoords") == 0 && opts.count("kBoltz") == 0 &&
             opts.count("pdfSpeed") == 0 && opts.count("freePath") == 0 &&
             opts.count("collNeg") == 0) {
    throw std::invalid_argument(
        "At least one of the base options (-t,-k,-p,-f,n) has to be "
        "called.");
  }
  return false;
}

}  // namespace input

}  // namespace gasSim

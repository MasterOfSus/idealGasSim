#include "RenderStyle.hpp"

#include <stdexcept>

void GS::RenderStyle::setWallsOpts(const std::string& opts) {
  bool isGood{true};
  if (opts.length() > 6) {
    throw std::invalid_argument(
        "setWallsOpts error: provided too many wall options");
    isGood = false;
  }
  for (char opt : opts) {
    if (opt != 'u' && opt != 'd' && opt != 'l' && opt != 'r' && opt != 'f' &&
        opt != 'b') {
      throw std::invalid_argument(
          "setWallsOpts error: provided invalid wall option");
      isGood = false;
    }
  }
  if (isGood) {
    wallsOpts.clear();
    if (opts.find('u') != std::string::npos) wallsOpts.push_back('u');
    if (opts.find('d') != std::string::npos) wallsOpts.push_back('d');
    if (opts.find('l') != std::string::npos) wallsOpts.push_back('l');
    if (opts.find('r') != std::string::npos) wallsOpts.push_back('r');
    if (opts.find('f') != std::string::npos) wallsOpts.push_back('f');
    if (opts.find('b') != std::string::npos) wallsOpts.push_back('b');
  }
}

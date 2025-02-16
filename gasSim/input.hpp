#ifndef INPUT_HPP
#define INPUT_HPP
#include "cxxopts.hpp"

namespace gasSim {
namespace input {
cxxopts::ParseResult optParse(int argc, const char* argv[]);
}  // namespace input
}  // namespace gasSim

#endif

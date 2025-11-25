#ifndef INPUT_HPP
#define INPUT_HPP
#include "../Libs/cxxopts.hpp"

namespace GS {
cxxopts::ParseResult optParse(int argc, const char* argv[]);
bool optControl(int argc, cxxopts::ParseResult& opts);
}  // namespace GS

#endif

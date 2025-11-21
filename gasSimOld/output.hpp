#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "statistics.hpp"
namespace gasSim {
void printInitData(int particleNum, double temperature, double boxSide,
                   int iterationNum, bool simultaneous);
void printStat(const TdStats& stats);
void printLastShit();
}  // namespace gasSim

#endif

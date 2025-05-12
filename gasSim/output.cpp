#include "output.hpp"

#include <iomanip>
#include <iostream>

namespace gasSim {
void printInitData(int particleNum, double temperature, double boxSide,
                   int iterationNum, bool simultaneous) {
  std::cout << "╔════════════════════════════════════════════════════╗\n";
  std::cout << "║           Ideal Gas Simulation - [idealGasSim]     ║\n";
  std::cout << "╠════════════════════════════════════════════════════╝\n";
  std::cout << "║ ▸ Simulation Parameters\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Particle Count"
            << ": " << std::right << std::setw(6) << particleNum << "\n";

  std::cout << "║   • " << std::left << std::setw(23) << "Temperature (alpha)"
            << ": " << std::right << std::setw(6) << temperature << "\n";

  std::cout << "║   • " << std::left << std::setw(23) << "Box Side Length (m)"
            << ": " << std::right << std::setw(6) << boxSide << "\n";

  std::cout << "║   • " << std::left << std::setw(23) << "Number of Iterations"
            << ": " << std::right << std::setw(6) << iterationNum << "\n";

  std::cout << "║   • " << std::left << std::setw(23) << "Parallel Collisions"
            << ": " << std::right << std::setw(6)
            << (simultaneous ? "true" : "false") << "\n";
}

void printStat(const TdStats& stats) {
	double pressure = stats.getPressure();
  double pressFront = stats.getPressure(gasSim::Wall::Front);
  double pressBack = stats.getPressure(gasSim::Wall::Back);
  double pressLeft = stats.getPressure(gasSim::Wall::Left);
  double pressRight = stats.getPressure(gasSim::Wall::Right);
  double pressTop = stats.getPressure(gasSim::Wall::Top);
  double pressBottom = stats.getPressure(gasSim::Wall::Bottom);

  std::cout << "║ ▸ Statistics\n";
	std::cout << "";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure "
						<< ": " << std::right << std::setw(6) << pressure << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on front"
            << ": " << std::right << std::setw(6) << pressFront << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on back"
            << ": " << std::right << std::setw(6) << pressBack << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on left"
            << ": " << std::right << std::setw(6) << pressLeft << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on right"
            << ": " << std::right << std::setw(6) << pressRight << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on top"
            << ": " << std::right << std::setw(6) << pressTop << "\n";
  std::cout << "║   • " << std::left << std::setw(23) << "Pressure on bottom"
            << ": " << std::right << std::setw(6) << pressBottom << "\n";

  /*

    std::cout << "║   • " << std::left << std::setw(23) << "Temperature (K)"
              << ": " << std::right << std::setw(6) << temperature
              << "                ║\n";

    std::cout << "║   • " << std::left << std::setw(23) << "Box Side Length (m)"
              << ": " << std::right << std::setw(6) << boxSide
              << "                ║\n";

    std::cout << "║   • " << std::left << std::setw(23) << "Number of
    Iterations"
              << ": " << std::right << std::setw(6) << iterationNum
              << "                ║\n";

    std::cout << "║   • " << std::left << std::setw(23) << "Parallel Collisions"
              << ": " << std::right << std::setw(6)
              << (simultaneous ? "true" : "false") << "                ║\n";*/
}

void printLastShit() {
  std::cout << "╚═════════════════════════════════════════════════════\n";
}

}  // namespace gasSim

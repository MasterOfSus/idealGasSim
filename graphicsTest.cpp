#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>

#include "gasSim/graphics.hpp"
#include "gasSim/physicsEngine.hpp"
#include "gasSim/statistics.hpp"

double gasSim::Particle::radius = 0.1;
double gasSim::Particle::mass = 10;

inline std::ostream &operator<<(std::ostream &os, const gasSim::PhysVectorD &vec) {
  os << "PhysVector(" << vec.x << ", " << vec.y << ", " << vec.z << ")   ";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const gasSim::Particle &part) {
  os << "Partcle(" << part.position << ", " << part.speed << ")\n";
  return os;
}

int main() {
  gasSim::Gas amogus(8, 1., 10.);
  gasSim::PhysVectorF focus{-23., 31., 12.};
  gasSim::PhysVectorF center{4.5, 4.5, 4.5};
  gasSim::Camera camera(focus, center - focus, 2., 90., 1200, 900);

  sf::RenderTexture photo;
  photo.create(1200, 900);
  gasSim::RenderStyle style{};
  std::string options{"udlrfb"};
  style.setWallsOpts(options);
  gasSim::drawGas(amogus, camera, photo, style);
  sf::Sprite picture;
  picture.setTexture(photo.getTexture());

  sf::ContextSettings settings;
  settings.antialiasingLevel = 8;

  sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window",
                          sf::Style::Default, settings);

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    sf::Event event;
    std::cout << amogus.getParticles()[0];
    std::cout << amogus.getParticles()[1];
    std::cout << amogus.getParticles()[2];
    std::cout << amogus.getParticles()[3];
    std::cout << amogus.getParticles()[4];
    std::cout << amogus.getParticles()[5];
    std::cout << amogus.getParticles()[6];
    std::cout << amogus.getParticles()[7];
	std::cout << "\n\n";
    gasSim::drawGas(amogus, camera, photo, style);

    sleep(1);
    amogus.simulate(1);

    while (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) window.close();
    }
    window.clear(sf::Color::White);
    window.draw(picture);
    window.display();
  }

  return 0;
}

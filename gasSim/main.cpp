#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>
#include <set>

#include "graphics.hpp"
#include "physicsEngine.hpp"

double gasSim::Particle::mass = 10;
double gasSim::Particle::radius = 1.;

std::ostream &operator<<(std::ostream &os, const gasSim::PhysVectorD &vec) {
  os << "PhysVector(" << vec.x << ", " << vec.y << ", " << vec.z << ")   ";
  return os;
}

std::ostream &operator<<(std::ostream &os, const gasSim::Particle &part) {
  os << "Partcle(" << part.position << ", " << part.speed << ")";
  return os;
}
/*
int main() {
  gasSim::Gas scureggione{1, 25, 10};
  gasSim::PhysVectorD focus{-23., 31., 12.};
  gasSim::PhysVectorD center{4.5, 4.5, 4.5};
  gasSim::Camera camera(focus, center - focus, 2., 90., 1200, 900);
  sf::RenderTexture photo;
  photo.create(1200, 900);
  gasSim::RenderStyle style{};

  style.setWallsOpts("udfblr");
  gasSim::drawGas(scureggione, camera, photo, style);
  sf::Sprite picture;
  picture.setTexture(photo.getTexture());

  sf::ContextSettings settings;
  settings.antialiasingLevel = 8;

  sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window",
                          sf::Style::Default, settings);

  // run the program as long as the window is open
  while (window.isOpen()) {
      for (int i{0}; i != 1; ++i) {
    scureggione.gasLoop(1);
  }
    gasSim::drawGas(scureggione, camera, photo, style);

    sleep(1);
    // check all the window's events that were triggered since the last
    // iteration of the loop
    sf::Event event;
    while (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) window.close();
    }
    window.draw(picture);
    window.display();

    sleep(1);
  }


}
*/
int main() {
  gasSim::Gas scureggione{1, 25, 10};
  for (int i{0}; i != 10; ++i) {
    gasSim::Statistic prova = scureggione.gasLoop(1);
    std::cout << scureggione.getLife() << '\n'
              << scureggione.getParticles()[0] << '\n'
              << prova.getPressure(gasSim::Wall::Back) << "  "
              << prova.getPressure(gasSim::Wall::Front) << "  "
              << prova.getPressure(gasSim::Wall::Left) << "  "
              << prova.getPressure(gasSim::Wall::Right) << "  "
              << prova.getPressure(gasSim::Wall::Bottom) << "  "
              << prova.getPressure(gasSim::Wall::Top) << "\n\n";
  }
}
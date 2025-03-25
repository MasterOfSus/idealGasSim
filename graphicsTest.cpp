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

double gasSim::Particle::radius = 1.;
double gasSim::Particle::mass = 10;

int main() {
  gasSim::Gas amogus(2, 1., 10.);
  gasSim::PhysVectorF focus{-23., 31., 12.};
  gasSim::PhysVectorF center{4.5, 4.5, 4.5};
  gasSim::Camera camera(focus, center - focus, 2., 90., 1200, 900);

  sf::RenderTexture photo;
  photo.create(1200, 900);
  gasSim::RenderStyle style{};
  std::string options{"udlrfb"};
  style.setWallsOpts(options);
  sf::Sprite picture;
  picture.setTexture(photo.getTexture());

  sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window",
                          sf::Style::Default);

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    sf::Event event;
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

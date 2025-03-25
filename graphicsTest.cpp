#include <unistd.h>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>

#include "gasSim/graphics.hpp"
#include "gasSim/output.hpp"
#include "gasSim/physicsEngine.hpp"
#include "gasSim/statistics.hpp"

double gasSim::Particle::radius = 0.1;
double gasSim::Particle::mass = 10;

int main() {
  gasSim::Gas amogus(200, 1., 10.);
  gasSim::PhysVectorF focus{23., 24., 26.};
  gasSim::PhysVectorF center{5., 5., 5.};
  gasSim::Camera camera(focus, center - focus, 2., 90., 1200, 900);

  sf::RenderTexture photo;
  photo.create(1200, 900);
  sf::CircleShape shape{1.f};
  shape.setFillColor(sf::Color::Red);
  shape.setOutlineColor(sf::Color::Black);
  shape.setOutlineThickness(10.);
  gasSim::RenderStyle style{shape};
  std::string options{"udlrfb"};
  style.setWallsOpts(options);
  sf::Sprite picture;
  picture.setTexture(photo.getTexture());

  sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window",

                          sf::Style::Default);

  amogus.simulate(8);

  gasSim::drawGas(amogus, camera, photo, style);
  gasSim::TdStats stats(amogus);
  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    sf::Event event;

   
    //sf::sleep(sf::milliseconds(90));
    amogus.simulate(1);
    gasSim::drawGas(amogus, camera, photo, style);

    std::vector<gasSim::Particle> particles = amogus.getParticles();

   // std::cout << "Particles poss and speeds:\n";

    for (const gasSim::Particle& particle : particles) {
      gasSim::PhysVectorD vector = particle.position;
     // std::cout << "(" << vector.x << ", " << vector.y << ", " << vector.z
             //   << "), (";
      vector = particle.speed;
     // std::cout << vector.x << ", " << vector.y << ", " << vector.z << ")\n";
    }

    while (window.pollEvent(event)) {
      // "close requested" event: we close the window
      if (event.type == sf::Event::Closed) window.close();
    }
    window.clear(sf::Color::Yellow);
    window.draw(picture);
    window.display();
  }
  gasSim::printInitData(1, 2, 3, 4, false);
  gasSim::printStat(stats);
  gasSim::printLastShit();

  return 0;
}

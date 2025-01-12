#include "gasSim/physicsEngine.hpp"
#include "gasSim/graphics.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/WindowStyle.hpp>
#include <iostream>

int main() {
	gasSim::Gas amogus(5, 1., 10.);
	gasSim::PhysVector focus {-23., 31., 12.};
	gasSim::PhysVector center {4.5, 4.5, 4.5};
	gasSim::Camera camera(focus, center - focus, 2., 90., 1200, 900);

	sf::RenderTexture photo;
	photo.create(1200, 900);
	gasSim::RenderStyle style {};
	std::string options {""};
	std::cout << "Input walls options: ";
	std::cin >> options;
	style.setWallsOpts(options);
	gasSim::drawGas(amogus, camera, photo, style);
	sf::Sprite picture;
	picture.setTexture(photo.getTexture());

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;

	sf::RenderWindow window(sf::VideoMode(800, 600), "SFML Window", sf::Style::Default, settings);

    // run the program as long as the window is open
	while (window.isOpen()) {
      // check all the window's events that were triggered since the last iteration of the loop
      sf::Event event;
      while (window.pollEvent(event))
      {
            // "close requested" event: we close the window
         if (event.type == sf::Event::Closed)
                window.close();
      }
	window.clear(sf::Color::White);
	window.draw(picture);
	window.display();
  }    

  return 0;
}


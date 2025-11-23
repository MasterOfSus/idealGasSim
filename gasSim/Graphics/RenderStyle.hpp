#ifndef RENDERSTYLE_HPP
#define RENDERSTYLE_HPP

#include <SFML/Graphics/Texture.hpp>

#include "../PhysicsEngine.hpp"

namespace GS {

class RenderStyle {
 public:
  const std::string& getWallsOpts() const { return wallsOpts; };
  void setWallsOpts(const std::string& opts);
  const sf::Color& getWallsColor() const { return wallsColor; };
  void setWallsColor(const sf::Color& color) { wallsColor = color; };
  const sf::Color& getWOutlineColor() const { return wOutlineColor; };
  void setWOutlineColor(const sf::Color& color) { wOutlineColor = color; };

  const sf::Texture& getPartTexture() const { return partTexture; };
  void setPartTexture(const sf::Texture& texture) { partTexture = texture; };

  const sf::Color& getBGColor() const { return background; };
  void setBGColor(const sf::Color& color) { background = color; };

  RenderStyle(const sf::Texture& texture) : partTexture(texture){};

  RenderStyle() = delete;

 private:
  std::string wallsOpts{"udlrfb"};  // up, down, left, right, front, back
                                    // as seen standing on the xy plane and
                                    // looking with direction j
  sf::Color wallsColor{0, 0, 0, 64};
  sf::Color wOutlineColor{sf::Color::Black};

  sf::Texture partTexture;

  sf::Color background{sf::Color::White};
};

}  // namespace GS

#endif

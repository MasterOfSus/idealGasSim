#ifndef RENDERSTYLE_HPP
#define RENDERSTYLE_HPP

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <string>

namespace GS {

class RenderStyle {
 public:
  RenderStyle(sf::Texture const& texture) : partTexture(texture) {}
  RenderStyle() = delete;

  std::string const& getWallsOpts() const { return wallsOpts; }
  void setWallsOpts(std::string const& opts);
  sf::Color getWallsColor() const { return wallsColor; }
  void setWallsColor(sf::Color color) { wallsColor = color; }
  sf::Color getWOutlineColor() const { return wOutlineColor; }
  void setWOutlineColor(sf::Color color) { wOutlineColor = color; }

  sf::Texture const& getPartTexture() const { return partTexture; }
  void setPartTexture(sf::Texture const& texture) { partTexture = texture; }

  sf::Color getBGColor() const { return background; }
  void setBGColor(sf::Color color) { background = color; }

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

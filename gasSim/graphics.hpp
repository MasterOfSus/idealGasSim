#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cmath>
#include <cwctype>
#include <stdexcept>
#include <vector>

#include "physicsEngine.hpp"

namespace gasSim {

class RenderStyle {
	public:
	const std::string& getGridOpts() const { return gridOpts_; };
	void setGridOpts(const std::string& opts);
	const sf::Color& getGridColor() const { return gridColor_; };
	void setGridColor(const sf::Color& color) { gridColor_ = color; };
	float getGridSpacing() const { return gridSpacing_; };
	void setGridSpacing(const float spacing);

	const std::string& getAxesOpts() const { return axesOpts_; };
	void setAxesOpts(const std::string& opts);
	const sf::Color& getAxesColor() const { return axesColor_; };
	void setAxesColor(const sf::Color& color) { axesColor_ = color; };
	float getAxesLength() const { return axesLength_; };
	void setAxesLength(const float length);

	const std::string& getWallsOpts() const { return wallsOpts_; };
	void setWallsOpts(const std::string& opts);
	const sf::Color& getWallsColor() const { return wallsColor_; };
	void setWallsColor(const sf::Color& color) { wallsColor_ = color; };
	const sf::Color& getWOutlineColor() const { return wOutlineColor_; };
	void setWOutlineColor(const sf::Color& color) { wOutlineColor_ = color; };

	const sf::Texture& getPartTexture() const { return partTexture_; };
	void setPartTexture(const sf::Texture& texture) { partTexture_ = texture; };

	const sf::Color& getBGColor() const { return background_; };
	void setBGColor(const sf::Color& color) { background_ = color; };

	RenderStyle(const sf::Texture& texture) : partTexture_(texture) {
	};

	private:

	std::string gridOpts_ {"x"}; // x -> xy plane, y -> yz plane, z -> zx plane
	sf::Color gridColor_ {0, 0, 0, 128};
	float gridSpacing_ {1.};

	std::string axesOpts_ {"xyz"};
	sf::Color axesColor_ {0, 0, 0, 255};
	float axesLength_ {10.};

	std::string wallsOpts_ {"udlrfb"}; // up, down, left, right, front, back
																		 // as seen standing on the xy plane and
																		 // looking along (0., 1., 0.)
	sf::Color wallsColor_ {0, 0, 0, 64};
	sf::Color wOutlineColor_ {sf::Color::Black};

	sf::Texture partTexture_;

	sf::Color background_ {sf::Color::White};
};

class GasData;

class Camera {
 public:

   // parametric constructor
  Camera(const PhysVectorF& focusPosition, const PhysVectorF& sightVector,
         float planeDistance, float fov, int width, int height);

	 // setters and getters
  void setFocus(const PhysVectorF& focusPoint) { focusPoint_ = focusPoint; };
  void setSightVector(const PhysVectorF& sightVector);
  void setAspectRatio(const float ratio);
  void setPlaneDistance(const float distance);
	void setFOV(const float FOV); // field of view setting in degrees
  void setResolution(const int height, const int width);

  PhysVectorF const& getFocus() const { return focusPoint_; };
  PhysVectorF const& getSight() const { return sightVector_; };
	float getAspectRatio() const {

    return static_cast<float>(width_) / static_cast<float>(height_);
  };
  float getPlaneDistance() const { return planeDistance_; };
  float getFOV() const { return fov_; };
  int getHeight() const { return height_; };
  int getWidth() const { return width_; };

	// useful functions
	PhysVectorF getPointProjection(const PhysVectorF& point) const;
	std::vector<PhysVectorF> projectParticles (const std::vector<Particle>& particles, double deltaT = 0.) const;
	std::vector<PhysVectorF> projectParticles(const GasData& data, double deltaT) const;

	// auxiliary member functions
	float getTopSide() const;
	float getPixelSide() const;
	float getNPixels(float length) const;

 private:

  PhysVectorF focusPoint_;
  PhysVectorF sightVector_;
  float planeDistance_;
  float fov_;

  int width_;
  int height_;

};

/*
struct ParticleProjection {
  static sf::CircleShape circle;
  PhysVectorF position;
};
*/

void drawAxes(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);
void drawGrid(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

template<typename GasLike>
void drawWalls(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

// void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {});


void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style, double deltaT = 0.);

void drawParticles(const GasData& data, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style, double deltaT = 0.);
//void drawGas(const Gas& gas, const Camera& camera, sf::RenderTexture& picture, const RenderStyle& style = {});

template<typename GasLike>
void drawGas(const GasLike& gasLike, const Camera& camera, sf::RenderTexture& picture, const RenderStyle& style, double deltaT = 0.);

}  // namespace gasSim

#endif

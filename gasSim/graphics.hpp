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

	const sf::CircleShape& getPartProj() const { return partProj_; };
	void setPartProj(const sf::CircleShape& circle) { partProj_ = circle; };

	const sf::Color& getBGColor() const { return background_; };
	void setBGColor(const sf::Color& color) { background_ = color; };

	RenderStyle() {
		partProj_.setFillColor(partColor_);
	};
	RenderStyle(const sf::CircleShape& defPartProj) : partProj_(defPartProj) {
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

	sf::CircleShape partProj_ {1.f, 20};
	sf::Color partColor_ {sf::Color::Red};

	sf::Color background_ {sf::Color::White};
};

class Camera {
 public:

   // parametric constructor
  Camera(const PhysVector& focusPosition, const PhysVector& sightVector,
         float planeDistance, float fov, int width, int height);

	 // setters and getters
  void setFocus(const PhysVector& focusPoint) { focusPoint_ = focusPoint; };
  void setSightVector(const PhysVector& sightVector) {

    sightVector_ = sightVector / sightVector.norm();
  };
  void setAspectRatio(const float ratio);
  void setPlaneDistance(const float distance);
	void setFOV(const float FOV); // field of view setting in degrees
  void setResolution(const int height, const int width);

  PhysVector const& getFocus() const { return focusPoint_; };
  PhysVector const& getSight() const { return sightVector_; };
	float getAspectRatio() const {

    return static_cast<float>(width_) / static_cast<float>(height_);
  };
  float getPlaneDistance() const { return planeDistance_; };
  float getFOV() const { return fov_; };
  int getHeight() const { return height_; };
  int getWidth() const { return width_; };

	// useful functions
	PhysVector getPointProjection(const PhysVector& point) const;
	// float getSegmentScale(const PhysVector& point) const;
	// PhysVector projectParticle(const Particle& particle) const;
	std::vector<PhysVector> projectParticles (const std::vector<Particle>& particles) const;

	// auxiliary member functions
	float getTopSide() const;
	float getPixelSide() const;
	float getNPixels(float length) const;

 private:

  PhysVector focusPoint_;
  PhysVector sightVector_;
  float planeDistance_;
  float fov_;

  int width_;
  int height_;

};


/*
struct ParticleProjection {
  static sf::CircleShape circle;
  PhysVectorD position;
};
*/


void drawAxes(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);
void drawGrid(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);
void drawWalls(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

void drawGas(const Gas& gas, const Camera& camera, sf::RenderTexture& picture, const RenderStyle& style);

}  // namespace gasSim

#endif

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
	double getGridSpacing() const { return gridSpacing_; };
	void setGridSpacing(const double spacing);

	const std::string& getAxesOpts() const { return axesOpts_; };
	void setAxesOpts(const std::string& opts);
	const sf::Color& getAxesColor() const { return axesColor_; };
	void setAxesColor(const sf::Color& color) { axesColor_ = color; };
	double getAxesLength() const { return axesLength_; };
	void setAxesLength(const double length);

	const std::string& getWallsOpts() const { return wallsOpts_; };
	void setWallsOpts(const std::string& opts);
	const sf::Color& getWallsColor() const { return wallsColor_; };
	void setWallsColor(const sf::Color& color) { wallsColor_ = color; };

	const sf::CircleShape& getPartProj() const { return partProj_; };
	void setPartProj(const sf::CircleShape& circle) { partProj_ = circle; };

	const sf::Color& getBackgroundColor() const { return background_; };
	void setBackgroundColor(const sf::Color& color) { background_ = color; };

	RenderStyle() {};
	RenderStyle(const sf::CircleShape& defPartProj) : partProj_(defPartProj) {}

	private:

	std::string gridOpts_ {"x"}; // x -> xy plane, y -> yz plane, z -> zx plane
	sf::Color gridColor_ {0, 0, 0, 128};
	double gridSpacing_ {1.};

	std::string axesOpts_ {"xyz"};
	sf::Color axesColor_ {0, 0, 0, 255};
	double axesLength_ {10.};

	std::string wallsOpts_ {"udlrfb"}; // top, down, left, right, front, back
	sf::Color wallsColor_ {0, 0, 0, 64};

	sf::CircleShape partProj_ {1.f, 20};
	sf::Color partColor_ {240, 0, 0, 255};

	sf::Color background_ {255, 255, 255, 255};
};

class Camera {
 public:
  // setters and getters
  void setFocus(const PhysVector& focusPoint) { focusPoint_ = focusPoint; };
  void setSightVector(const PhysVector& sightVector) {
    sightVector_ = sightVector / sightVector.norm();
  };
  void setAspectRatio(const double ratio);
  void setPlaneDistance(const double distance);
	void setFOV(const double FOV); // field of view setting in degrees
  void setResolution(const int height, const int width);

  PhysVector const& getFocus() const { return focusPoint_; };
  PhysVector const& getSight() const { return sightVector_; };
  double getAspectRatio() const {
    return static_cast<float>(width_) / static_cast<float>(height_);
  };
  double getPlaneDistance() const { return planeDistance_; };
  double getFOV() const { return fov_; };
  int getHeight() const { return height_; };
  int getWidth() const { return width_; };

  // parametric constructor
  Camera(const PhysVector& focusPosition, const PhysVector& sightVector,
         double planeDistance, double fov, int width, int height);

 private:
  PhysVector focusPoint_;
  PhysVector sightVector_;
  double planeDistance_;
  double fov_;
  int width_;
  int height_;
};	

double getCamTopSide(const Camera& camera);

int getNPixels(double lenght, const Camera& camera);

PhysVector getPointProjection(const PhysVector& point, const Camera& camera);
double getSegmentScale(const PhysVector& point, const Camera& camera);

/*
struct ParticleProjection {
  static sf::CircleShape circle;
  PhysVector position;
};
*/

PhysVector projectParticle(const Particle& particle, const Camera& camera);

std::vector<PhysVector> projectParticles (const std::vector<Particle>& particles);

void drawAxes(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);
void drawGrid(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);
void drawWalls(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture);

sf::RenderTexture drawGas(const Gas& gas);

}  // namespace gasSim

#endif

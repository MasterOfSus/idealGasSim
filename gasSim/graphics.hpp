#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "physicsEngine.hpp"

namespace gasSim {
namespace graphics {

using PhysVector = physics::PhysVector;
using Particle = physics::Particle;
using Gas = physics::Gas;

static sf::CircleShape defPartImage{1., 20};

class Camera {
 public:
  // setters and getters
  void setFocus(const PhysVector& focusPoint) { focusPoint_ = focusPoint; };
  void setSightVector(const PhysVector& sightVector) {
    sightVector_ = sightVector / sightVector.norm();
  };
  void setAspectRatio(const double ratio);
  void setPlaneDistance(const double distance);
  void setFOV(const double FOV);
  void setResolution(const int height, const int width);

  PhysVector const& getFocus() const { return focusPoint_; };
  PhysVector const& getSight() const { return sightVector_; };
  double getAspectRatio() const {
    return static_cast<double>(width_) / static_cast<double>(height_);
  };
  double getPlaneDistance() const { return planeDistance_; };
  double getFOV() const { return fov_; };
  int getHeight() const { return height_; };
  int getWidth() const { return width_; };

  // parametric constructor
  Camera(PhysVector focusPosition, PhysVector sightVector, double aspectRatio,
         double planeDistance, double fov, int height);

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

std::vector<PhysVector> projectParticles(
    const std::vector<Particle>& particles);
void drawAxes(char opt, const Camera& camera, sf::RenderTexture& texture);
// void drawGrid(char opt, const Camera& camera, sf::RenderTexture& image);
void drawWalls(char opt, const Gas& gas, const Camera& camera,
               sf::RenderTexture& texture);

sf::RenderTexture drawGas(const Gas& gas);

}  // namespace graphics
}  // namespace gasSim

#endif

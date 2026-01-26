#ifndef CAMERAHPP
#define CAMERAHPP

#include <array>
#include <vector>

#include <SFML/Graphics/RenderTexture.hpp>

#include "PhysicsEngine/GSVector.hpp"
#include "RenderStyle.hpp"

namespace GS {

class GasData;
class Gas;
struct Particle;

class Camera {
 public:
  // parametric constructor
  Camera(GSVectorF const& focusPosition, GSVectorF const& sightVector,
         float planeDistance = 1.f, float fov = 90.f, unsigned width = 1920,
         unsigned height = 1080);

  GSVectorF getPointProjection(GSVectorF const& point) const;
  std::vector<GSVectorF> projectParticles(
      std::vector<Particle> const& particles, double deltaT = 0.) const;
  std::vector<GSVectorF> projectParticles(GasData const& data,
                                          double deltaT) const;
  // auxiliary member functions
  float getTopSide() const;
  float getPixelSide() const;
  float getNPixels(float length) const;

  // setters and getters
  void setFocus(GSVectorF const& focusPtV);
  void setSightVector(GSVectorF const& sightVector);
  void setAspectRatio(float ratio);
  void setPlaneDistance(float distance);
  void setFOV(float FOV);  // lateral field of view setting (in degrees)
  void setResolution(unsigned height, unsigned width);

  GSVectorF const& getFocus() const { return focusPoint; }
  GSVectorF const& getSight() const { return sightVector; }
  float getAspectRatio() const {
    return static_cast<float>(width) / static_cast<float>(height);
  }
  float getPlaneDistance() const { return planeDistance; }
  float getFOV() const { return fov; }
  unsigned getHeight() const { return height; }
  unsigned getWidth() const { return width; }

 private:
  bool constructed{false};
  void computeCamBase();
  std::array<GSVectorF, 2> cameraBase;

  GSVectorF focusPoint;
  GSVectorF sightVector;
  float planeDistance;
  float fov;

  unsigned width;
  unsigned height;
};

template <typename GasLike>
void drawGas(GasLike const& gasLike, Camera const& camera,
             sf::RenderTexture& picture, RenderStyle const& style,
             double deltaT = 0.);

void drawParticles(Gas const& gas, Camera const& camera,
                   sf::RenderTexture& texture, RenderStyle const& style,
                   double deltaT = 0.);

void drawParticles(GasData const& data, Camera const& camera,
                   sf::RenderTexture& texture, RenderStyle const& style,
                   double deltaT = 0.);

template <typename GasLike>
void drawWalls(GasLike const& gas, Camera const& camera,
               sf::RenderTexture& texture, RenderStyle const& style);

}  // namespace GS

#endif

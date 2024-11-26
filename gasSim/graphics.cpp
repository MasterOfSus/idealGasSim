#include "graphics.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <algorithm>
#include <stdexcept>

namespace gasSim {
namespace graphics {
// Camera member functions
// Getters and setters

void Camera::setPlaneDistance(const double distance) {
  if (distance >= 0.) {
    planeDistance_ = distance;
  } else {
    throw std::runtime_error("Bad distance.");
  }
}

void Camera::setFOV(const double FOV) {  // in degrees
  if (FOV > 0. && FOV < 180.) {
    fov_ = FOV;
  } else {
    throw std::runtime_error("Bad FOV.");
  }
}

void Camera::setResolution(const int height, const int width) {
  if (height <= 0) {
    throw std::runtime_error("Bad height.");
  } else if (width <= 0) {
    throw std::runtime_error("Bad width.");
  } else {
    height_ = height;
    width_ = width;
  }
}

void Camera::setAspectRatio(const double ratio) {
  if (ratio > 0.) {
    height_ = static_cast<int>(width_ / ratio);
  } else {
    throw std::runtime_error("Bad ratio.");
  }
}

Camera::Camera(PhysVector focusPosition, PhysVector sightVector,
               double aspectRatio = 16. / 9., double planeDistance = 1.,
               double FOV = 90, int height = 1080)
    : focusPoint_(focusPosition),
      sightVector_(sightVector / sightVector.norm()) {
  if (aspectRatio < 0.) {
    throw std::runtime_error("Bad aspect ratio.");
  } else if (planeDistance < 0) {
    throw std::runtime_error("Bad plane distance.");
  } else if (FOV < 0. || FOV > 180.) {
    throw std::runtime_error("Bad FOV.");
  } else if (height < 0) {
    throw std::runtime_error("Bad height.");
  } else {
    planeDistance_ = planeDistance;
    fov_ = FOV;
    height_ = height;
  }
}

double getCamTopSide(const Camera& camera) {
  return 2 * camera.getPlaneDistance() * tan(camera.getFOV() / 2);
};

int getNPixels(double length, const Camera& camera) {
  return static_cast<int>(camera.getWidth() * length / getCamTopSide(camera));
}

/*
struct Matrix3x3 {
        PhysVector col1;
        PhysVector col2;
        PhysVector col3;
};

PhysVector operator*(PhysVector vector, Matrix3x3 matrix) {
        return {vector * matrix.col1, vector * matrix.col2, vector *
matrix.col3};
}
*/

PhysVector crossProd(PhysVector v1, PhysVector v2) {
  return {v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
          v1.x * v2.y - v1.y * v2.x};
}

PhysVector getPointProjection(const PhysVector& point, const Camera& camera) {
  PhysVector a{(point - camera.getFocus() /
                            ((point - camera.getFocus()) * camera.getSight())) *
                   camera.getPlaneDistance() +
               camera.getFocus()};

  // making an orthonormal base for the camera, m and o lying on the persp.
  // plane, n for the normal vector
  PhysVector m;
  PhysVector sight{camera.getSight()};
  m = {sight.y, -sight.x, 0.};
  m = m / m.norm();
  PhysVector o{crossProd(m, sight)};
  // returning base-changed vector, with depth field as the third coordinate
  return {m * a, o * a, (point - camera.getFocus()).norm()};
}

double getSegmentScale(const PhysVector& point, const Camera& camera) {
  return (camera.getFocus() - point).norm() /
         (camera.getFocus() - getPointProjection(point, camera)).norm();
}

PhysVector projectParticle(const Particle& particle, const Camera& camera) {
  return {getPointProjection(particle.position, camera)};
}

std::vector<PhysVector> projectParticles(const std::vector<Particle>& particles,
                                         const Camera& camera) {
  std::vector<PhysVector> projections{};
  for (const Particle& particle : particles) {
    projections.emplace_back(projectParticle(particle, camera));
  }
  return projections;
}

void drawAxes(/*char opt,*/ const Camera& camera, const double axisLength,
              sf::RenderTexture texture) {
  static sf::CircleShape tip{1., 3};
  static sf::ConvexShape shaft;
  static bool i{true};
  PhysVector originProj{getPointProjection({0., 0., 0.}, camera)};
  shaft.setPoint(
      1, {static_cast<float>(originProj.x), static_cast<float>(originProj.y)});
  PhysVector arrowTipsPs[3]{{getPointProjection({axisLength, 0., 0.}, camera)},
                            {getPointProjection({0., axisLength, 0.}, camera)},
                            {getPointProjection({0., 0., axisLength}, camera)}};
  for (const PhysVector& position : arrowTipsPs) {
    sf::Vector2f auxPos{static_cast<float>(position.x),
                        static_cast<float>(position.y)};
    shaft.setPoint(2, auxPos);
    tip.setPosition(auxPos);
    texture.draw(shaft);
    texture.draw(tip);
  }
}

/*void drawWalls(char opt, const Gas& gas, const Camera& camera,
sf::RenderTexture& texture) {

}*/

void drawParticles(const Gas& gas, const Camera& camera) {
  static sf::CircleShape circle{1., 10};
  std::vector<PhysVector> projections = projectParticles(gas.getParticles());
  sort(projections.begin(), projections.end(),  // sort by depth fields
       [](const PhysVector& v1, const PhysVector& v2) { return v1.z < v2.z; });
  for (const PhysVector& projection : projections) {
  }
}

sf::RenderTexture drawGas(const Gas& gas);

}  // namespace graphics
}  // namespace gasSim

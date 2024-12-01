#include "graphics.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <algorithm>
#include <stdexcept>

namespace gasSim {

// renderStyle member functions
void RenderStyle::setGridOpts(const char* opts) {
}

void RenderStyle::setAxesOpts(const char* opts) {

}

void RenderStyle::setWallsOpts(const char* opts) {

}

// Camera member functions
// Getters and setters

void Camera::setPlaneDistance(const double distance) {
  if (distance >= 0.) {
    planeDistance_ = distance;
  } else {
    throw std::invalid_argument("Bad distance.");
  }
}

void Camera::setFOV(const double FOV) { // in degrees
  if (FOV > 0. && FOV < 180.) {
    fov_ = FOV;
  } else {
    throw std::invalid_argument("Bad FOV.");
  }
}

void Camera::setResolution(const int height, const int width) {
  if (height <= 0) {
    throw std::invalid_argument("Bad height.");
  } else if (width <= 0) {
    throw std::invalid_argument("Bad width.");
  } else {
    height_ = height;
    width_ = width;
  }
}

void Camera::setAspectRatio(const double ratio) { // ratio set by keeping the image width
  if (ratio > 0.) {
    height_ = static_cast<int>(width_ / ratio);
  } else {
    throw std::invalid_argument("Bad ratio.");
  }
}

Camera::Camera(const PhysVector& focusPosition, const PhysVector& sightVector,
               double planeDistance = 1.,
               double FOV = 90, int width = 1920, int height = 1080)
    : focusPoint_(focusPosition),
      sightVector_(sightVector / sightVector.norm()) {
				setPlaneDistance(planeDistance);
				setFOV(FOV);
				setResolution(height, width);
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

PhysVector crossProd(const PhysVector& v1, const PhysVector& v2) {
  return {v1.y * v2.z - v1.z * v2.y,
					v1.z * v2.x - v1.x * v2.z,
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
  m = {sight.y, - sight.x, 0.};
  m = m / m.norm();
  PhysVector o{crossProd(m, sight)};
  // returning base-changed vector with scaling factor as the third coordinate
  return {
		m * a,
		o * a, 
		(point - camera.getFocus()).norm() /
		(camera.getFocus() - a).norm()
		};
}

/*
double getSegmentScale(const PhysVector& point, const Camera& camera) {
  return (camera.getFocus() - point).norm() /
         (camera.getFocus() - getPointProjection(point, camera)).norm();
} // wrong retard retard retard
*/
/*
PhysVector projectParticle(const Particle& particle, const Camera& camera) {
  return {getPointProjection(particle.position, camera)};
}
*/

std::vector<PhysVector> projectParticles(const std::vector<Particle>& particles,
                                         const Camera& camera) {
  std::vector<PhysVector> projections{};
  for (const Particle& particle : particles) {
    projections.emplace_back(getPointProjection(particle.position, camera));
  }
  return projections;
}

void setDefParticleTexture(const std::string& textureFile) {
	sf::Texture texture;
	if(texture.loadFromFile(textureFile)) {
		defPartProj.setTexture(&texture);
	} else {
		defPartProj.setFillColor(sf::Color::Magenta);
	}
}

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture) {
	float nPixels { static_cast<float>(camera.getHeight() * gas.getParticles()[0].radius / getCamTopSide(camera)) };
  defPartProj.setRadius(nPixels);
	std::vector<PhysVector> projections = projectParticles(gas.getParticles());
  sort(projections.begin(), projections.end(),  // sort by scaling coordinates
       [](const PhysVector& v1, const PhysVector& v2) { return v1.z < v2.z; });
  for (const PhysVector& projection : projections) {
		defPartProj.setScale(1.f/static_cast<float>(projection.z), 1.f/static_cast<float>(projection.z));
		defPartProj.setPosition(projection.x, projection.y);
		texture.draw(defPartProj);
  }
}

void drawAxes(/*char* opt,*/ const Camera& camera, const double axisLength,
              sf::RenderTexture texture) {
  static sf::CircleShape tip{10., 3};
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

void drawGrid(const char* opt, const Camera& camera, sf::RenderTexture& texture) {
	for () {}
}

/*void drawWalls(char opt, const Gas& gas, const Camera& camera,
sf::RenderTexture& texture) {

}*/

sf::RenderTexture drawGas(const Gas& gas);

}  // namespace gasSim

#include "graphics.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <algorithm>
#include <stdexcept>

namespace gasSim {

// renderStyle member functions
void RenderStyle::setGridOpts(const std::string& opts) {
	if (opts.length() > 3) throw std::invalid_argument("Too many grid options.");
	for (char opt: opts) {
		if (opt != 'x' && opt != 'y' && opt != 'z') throw std::invalid_argument("Invalid grid option.");
	}
	gridOpts_.clear();
	if (opts.find('x') != std::string::npos) gridOpts_.push_back('x');
	if (opts.find('y') != std::string::npos) gridOpts_.push_back('y');
	if (opts.find('z') != std::string::npos) gridOpts_.push_back('z');
}

void RenderStyle::setAxesOpts(const std::string& opts) {
	if (opts.length() > 6) throw std::invalid_argument("Too many axes options.");
	for (char opt: opts) {
		if (opt != 'x' && opt != 'y' && opt != 'z') throw std::invalid_argument("Invalid axes option.");
	}
	axesOpts_.clear();
	if (opts.find('x') != std::string::npos) axesOpts_.push_back('x');
	if (opts.find('y') != std::string::npos) axesOpts_.push_back('y');
	if (opts.find('z') != std::string::npos) axesOpts_.push_back('z');
}

void RenderStyle::setAxesLength(const double length) {
	if (length < 0.) throw std::invalid_argument("Invalid axis length.");
	axesLength_ = length;
}

void RenderStyle::setWallsOpts(const std::string& opts) {
	if (opts.length() > 3) throw std::invalid_argument("Too many walls options.");
	for (char opt: opts) {
		if (opt != 'u' && opt != 'd' &&
				opt != 'l' && opt != 'r' &&
				opt != 'f' && opt != 'b')
			throw std::invalid_argument("Invalid walls option.");
	}
	wallsOpts_.clear();
	if (opts.find('u') != std::string::npos) wallsOpts_.push_back('u');
	if (opts.find('d') != std::string::npos) wallsOpts_.push_back('d');
	if (opts.find('l') != std::string::npos) wallsOpts_.push_back('l');
	if (opts.find('r') != std::string::npos) wallsOpts_.push_back('r');
	if (opts.find('f') != std::string::npos) wallsOpts_.push_back('f');
	if (opts.find('b') != std::string::npos) wallsOpts_.push_back('b');
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
		(camera.getFocus() - a).norm() /
		(point - camera.getFocus()).norm()
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

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {}) {
	static sf::CircleShape partProj = style.getPartProj();
	partProj.setRadius(getNPixels(gas.getParticles().begin()->radius, camera));
	std::vector<PhysVector> projections = projectParticles(gas.getParticles());
	std::sort(projections.begin(), projections.end(), 
						[](const PhysVector& a, const PhysVector& b) {
							return a.z > b.z;});
	for (const PhysVector& projection: projections) {
		partProj.setPosition(projection.x, projection.y); // wrong, but heart's in the righ place
		partProj.setScale(projection.z, projection.z);
		texture.draw(partProj);
	}
}

void drawAxes(const Camera& camera, sf::RenderTexture texture, const RenderStyle& style = {}) {
// imagine there was a functioning arrow here
	sf::CircleShape arrow {1.f, 3};
	float axesLength = style.getAxesLength();

	for (char opt: style.getAxesOpts()) {
		switch (opt) {
			case 'x':
				{
					PhysVector proj = getPointProjection({axesLength, 0., 0.}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
			case 'y':
				{
					PhysVector proj = getPointProjection({0., axesLength, 0.}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
			case 'z':
				{
					PhysVector proj = getPointProjection({0., 0., axesLength}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
		}
	}
}

void drawGrid(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {}) {
	sf::Vertex line[2];
	line[0].color = style.getGridColor();
	line[1].color = style.getGridColor();
	for (char opt: style.getGridOpts()) {
		switch (opt) {
			case 'x':
				break;
			case 'y':
				break;
			case 'z':
				break;
		}
	}
}

void drawWalls(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& = {}) {

}

sf::RenderTexture drawGas(const Gas& gas);

}  // namespace gasSim

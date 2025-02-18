#include "graphics.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <cassert>
#include <iostream>

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

void RenderStyle::setAxesLength(const float length) {
	if (length < 0.) throw std::invalid_argument("Invalid axis length.");
	axesLength_ = length;
}

void RenderStyle::setWallsOpts(const std::string& opts) {
	if (opts.length() > 6) throw std::invalid_argument("Too many walls options.");
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

void Camera::setSightVector(const PhysVectorF& sightVector) {
	if (sightVector.norm() > 0.) {
		sightVector_ = sightVector / sightVector.norm();
	} else throw std::invalid_argument("Null vector provided.");
}

void Camera::setPlaneDistance(const float distance) {
  if (distance > 0.) {
    planeDistance_ = distance;
  } else {
    throw std::invalid_argument("Bad distance.");
  }
}

void Camera::setFOV(const float FOV) { // in degrees
  if (FOV > 0. && FOV < 180.) {
    fov_ = FOV;
  } else {
    throw std::invalid_argument("Bad FOV.");
  }
}

void Camera::setResolution(const int height, const int width) {
  if (height > 0. && width > 0.) {
    height_ = height;
    width_ = width;
  } else {
		throw std::invalid_argument("Bad resolution");
	}
}

void Camera::setAspectRatio(const float ratio) { // ratio set by keeping the image width
  if (ratio > 0.) {
    height_ = static_cast<int>(width_ / ratio);
  } else {
    throw std::invalid_argument("Bad ratio.");
  }
}

Camera::Camera(const PhysVectorF& focusPosition, const PhysVectorF& sightVector,
               float planeDistance = 1.,
               float FOV = 90, int width = 1920, int height = 1080)
    : focusPoint_(focusPosition),
      sightVector_(sightVector / sightVector.norm()) {
				setPlaneDistance(planeDistance);
				setFOV(FOV);
				setResolution(height, width);
}

float Camera::getTopSide() const {
  return 2 * getPlaneDistance() * tan(getFOV() * (M_PI / 180.) / 2.);
};

float Camera::getPixelSide() const {
	return getTopSide()/getWidth();
}

float Camera::getNPixels(float length) const {
  return std::abs(length) / getPixelSide();
}

PhysVectorF crossProd(const PhysVectorF& v1, const PhysVectorF& v2) {
  return {v1.y * v2.z - v1.z * v2.y,
					v1.z * v2.x - v1.x * v2.z,
          v1.x * v2.y - v1.y * v2.x};
}

PhysVectorF Camera::getPointProjection(const PhysVectorF& point) const {
	PhysVectorF focus {getFocus()};
	PhysVectorF sight {getSight()};
	PhysVectorF a{focus + 
							 (focus - point) /
               ((focus - point) * sight) *
               getPlaneDistance()};

	PhysVectorF b {a - (focus + sight*getPlaneDistance())};

  // making an orthonormal base for the camera, m and o lying on the persp.
  // plane, sight for the normal vector
  PhysVectorF m;
  m = {sight.y, - sight.x, 0.};
  m = m / m.norm();
  PhysVector o{crossProd(m, sight)};
	m = m / getPixelSide();
	o = o / getPixelSide();

  // returning base-changed vector with scaling factor, with sign for positional information
	// as the third coordinate
	return {
		m * b + getWidth()/2.f,
		o * b + getHeight()/2.f,
		(a - focus)*(a - focus) / ((a - focus)*(point - focus)) // scaling factor, degen. if > 1 V < 0
		};
}

std::vector<PhysVectorF> Camera::projectParticles(const std::vector<Particle>& particles) const {
  std::vector<PhysVectorF> projections {};
  PhysVectorF proj{};

	for (const Particle& particle : particles) {
		proj = getPointProjection(static_cast<PhysVectorF>(particle.position));
		if (proj.z <= 1 && proj.z > 0) {
    	projections.emplace_back(proj);
		}
  }
  return projections;
}

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {}) {
	static sf::CircleShape partProj = style.getPartProj();
	float r {camera.getNPixels(gas.getParticles().begin()->radius)};
	partProj.setRadius(r);
	partProj.setOrigin({r, -r});
	std::vector<PhysVectorF> projections = camera.projectParticles(gas.getParticles());

	std::sort(projections.begin(), projections.end(), 
						[](const PhysVectorF& a, const PhysVectorF& b) {
							return a.z > b.z;});
	for (const PhysVectorF& proj: projections) {
		partProj.setPosition({proj.x, proj.y});
		partProj.setScale({proj.z, proj.z});

		texture.draw(partProj);
	}
}

/*
void drawAxes(const Camera& camera, sf::RenderTexture texture, const RenderStyle& style = {}) {
// imagine there was a functioning arrow here
	sf::CircleShape arrow {1.f, 3};
	float axesLength = style.getAxesLength();

	for (char opt: style.getAxesOpts()) {
		switch (opt) {
			case 'x':
				{
					PhysVectorF proj = getPointProjection({axesLength, 0., 0.}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
			case 'y':
				{
					PhysVectorF proj = getPointProjection({0., axesLength, 0.}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
			case 'z':
				{
					PhysVectorF proj = getPointProjection({0., 0., axesLength}, camera);
					arrow.setPosition(proj.x, proj.y);
					texture.draw(arrow);
					break;
				}
		}
	}
}
*/
/*
void project(PhysVectorF& origin, PhysVectorF& direction, const Camera& camera) {
	origin = getPointProjection(origin, camera);
	direction = getPointProjection(origin + direction, camera);
}

void drawGrid(const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {}) {
	sf::Vertex line[2];
	line[0].color = style.getGridColor();
	line[1].color = style.getGridColor();
	double spacing = style.getGridSpacing();
	PhysVectorF p1 {};
	PhysVectorF p2 {};
	sf::VertexArray line{sf::Line};
	for (char opt: style.getGridOpts()) {
		switch (opt) {
			case 'x':
double spacing = style.getGridSpacing();
p2 = {1., 0., 0.};
for (double i {0}; true; ++i) {
	p1 = {0, i, 0};
	p1 = getPointProjection(p1, camera);
	p2 = getPointProjection(p1 + p2, camera);
	
}
				break;
			case 'y':
				break;
			case 'z':
				break;
		}
	}
}
*/

std::vector<PhysVectorF> gasWallVerts(const Gas& gas, char wall) {
	float side {static_cast<float>(gas.getBoxSide())};
	switch (wall) {
		case 'u':
			return {
				{0., 0., side},
				{side, 0., side},
				{side, side, side},
				{0., side, side}
			}; // ok
			break;
		case 'd':
			return {
				{0., 0., 0.},
				{side, 0., 0.},
				{side, side, 0.},
				{0., side, 0.}
			}; // ok
			break;
		case 'l':
			return {
				{0., 0., 0.},
				{0., side, 0.},
				{0., side, side},
				{0., 0., side}
			}; // ok
			break;
		case 'r':
			return {
				{side, 0., 0.},
				{side, side, 0.},
				{side, side, side},
				{side, 0., side}
			}; // ok
			break;
		case 'f':
			return {
				{0., 0., 0.},
				{side, 0., 0.},
				{side, 0., side},
				{0., 0., side}
			}; // ok
			break;
		case 'b':
			return {
				{0., side, 0.},
				{side, side, 0.},
				{side, side, side},
				{0., side, side}
			}; // ok
			break;
	};
	return {};
}; // is good

void drawWalls(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style = {}) {
	PhysVectorF wallN {};
	PhysVectorF wallCenter {};
	float side {static_cast<float>(gas.getBoxSide())};
	
	std::vector<PhysVectorF> wallVerts {};
	sf::ConvexShape wallProj;
	wallProj.setPointCount(4);
	wallProj.setFillColor(style.getWallsColor());
	wallProj.setOutlineThickness(1);
	wallProj.setOutlineColor(style.getWOutlineColor());
	std::vector<sf::ConvexShape> frontWallPrjs {};
	std::vector<sf::ConvexShape> backWallPrjs {};
	for (char wall: style.getWallsOpts()) {
		switch (wall) {
			case 'u':
				wallN = {0., 0., 1.};
				wallCenter = {side/2.f, side/2.f, side};
				break;
			case 'd':
				wallN = {0., 0., -1.};
				wallCenter = {side/2.f, side/2.f, 0.};
				break;
			case 'l':
				wallN = {-1., 0., 0.};
				wallCenter = {0., side/2.f, side/2.f};
				break;
			case 'r':
				wallN = {1., 0., 0.};
				wallCenter = {side, side/2.f, side/2.f};
				break;
			case 'f':
				wallN = {0., -1., 0.};
				wallCenter = {side/2.f, 0.f, side/2.f};
				break;
			case 'b':
				wallN = {0., 1., 0.};
				wallCenter = {side/2.f, side, side/2.f};
				break;
		} // is good
		wallVerts = gasWallVerts(gas, wall); // should be good
		int i {};
		for (const PhysVectorF& vertex: wallVerts) {
			PhysVectorF proj = camera.getPointProjection(vertex);
			wallProj.setPoint(i, {static_cast<float>(proj.x), static_cast<float>(proj.y)});
			++i;
		} // all good
		if (wallN * (wallCenter - camera.getFocus()) < 0.) {
			frontWallPrjs.emplace_back(wallProj);
		}
		else {
			backWallPrjs.emplace_back(wallProj);
		}
	} // all seems to be good except for emplace_back?
	sf::RenderTexture backWalls;
	backWalls.create(camera.getWidth(), camera.getHeight());
	backWalls.clear(sf::Color::White);
	sf::RenderTexture frontWalls;
	frontWalls.create(camera.getWidth(), camera.getHeight());
	for (const sf::ConvexShape& wallPrj: backWallPrjs) {
		backWalls.draw(wallPrj);
	}
	for (const sf::ConvexShape& wallPrj: frontWallPrjs) {
		frontWalls.draw(wallPrj);
	} // all good up to here?
	
	sf::Sprite auxSprite;
	auxSprite.setScale(1., -1.);
	auxSprite.setPosition(0., backWalls.getSize().y);

	auxSprite.setTexture(texture.getTexture(), true);
	backWalls.draw(auxSprite);

	auxSprite.setTexture(frontWalls.getTexture(), true);
	backWalls.draw(auxSprite);

	auxSprite.setTexture(backWalls.getTexture(), true);
	texture.clear(sf::Color::Transparent);
	texture.draw(auxSprite);
}

void drawGas(const Gas& gas, const Camera& camera, sf::RenderTexture& picture, const RenderStyle& style) {
	picture.create(camera.getWidth(), camera.getHeight());
	picture.clear(sf::Color::Transparent);
	drawParticles(gas, camera, picture, style);
	drawWalls(gas, camera, picture, style);
};

}  // namespace gasSim

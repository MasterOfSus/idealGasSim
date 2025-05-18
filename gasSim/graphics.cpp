#include "graphics.hpp"

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <execution>
#include <tuple>
#include <chrono>

#include "physicsEngine.hpp"
#include "statistics.hpp"

namespace gasSim {

// renderStyle member functions
void RenderStyle::setGridOpts(const std::string& opts) {
	bool isGood {true};
	if (opts.length() > 3) {
		throw std::invalid_argument("Too many grid options.");
		isGood = false;
	}
	for (char opt: opts) {
		if (opt != 'x' && opt != 'y' && opt != 'z') {
			throw std::invalid_argument("Invalid grid option. ");
			isGood = false;
		}
	}
	if (isGood) {
		gridOpts_.clear();
		if (opts.find('x') != std::string::npos) gridOpts_.push_back('x');
		if (opts.find('y') != std::string::npos) gridOpts_.push_back('y');
		if (opts.find('z') != std::string::npos) gridOpts_.push_back('z');
	}
}

void RenderStyle::setAxesOpts(const std::string& opts) {
	bool isGood {true};
	if (opts.length() > 6) {
		throw std::invalid_argument("Too many axis options.");
		isGood = false;
	}
	for (char opt: opts) {
		if (opt != 'x' && opt != 'y' && opt != 'z') {
			throw std::invalid_argument("Invalid axis option.");
			isGood = false;
		}
	}
	if (isGood) {
		axesOpts_.clear();
		if (opts.find('x') != std::string::npos) axesOpts_.push_back('x');
		if (opts.find('y') != std::string::npos) axesOpts_.push_back('y');
		if (opts.find('z') != std::string::npos) axesOpts_.push_back('z');
	}
}

void RenderStyle::setAxesLength(const float length) {
	if (length < 0.) throw std::invalid_argument("Invalid axes length.");
	else axesLength_ = length;
}

void RenderStyle::setWallsOpts(const std::string& opts) {
	bool isGood {true};
	if (opts.length() > 6) {
		throw std::invalid_argument("Too many wall options.");
		isGood = false;
	}
	for (char opt: opts) {
		if (opt != 'u' && opt != 'd' &&
				opt != 'l' && opt != 'r' &&
				opt != 'f' && opt != 'b') {
			throw std::invalid_argument("Invalid wall option.");
			isGood = false;
		}
	}
	if (isGood) {
		wallsOpts_.clear();
		if (opts.find('u') != std::string::npos) wallsOpts_.push_back('u');
		if (opts.find('d') != std::string::npos) wallsOpts_.push_back('d');
		if (opts.find('l') != std::string::npos) wallsOpts_.push_back('l');
		if (opts.find('r') != std::string::npos) wallsOpts_.push_back('r');
		if (opts.find('f') != std::string::npos) wallsOpts_.push_back('f');
		if (opts.find('b') != std::string::npos) wallsOpts_.push_back('b');
	}
}

// Camera member functions
// Getters and setters

void Camera::setSightVector(const PhysVectorF& sightVector) {
	if (sightVector.norm() > 0.f) {
		sightVector_ = sightVector / sightVector.norm();
	} else throw std::invalid_argument("Null vector cannot be normalized.");
}

void Camera::setPlaneDistance(const float distance) {
  if (distance > 0.f) {
    planeDistance_ = distance;
  } else {
    throw std::invalid_argument("Non positive distance.");
  }
}

void Camera::setFOV(const float FOV) { // in degrees
  if (FOV > 0.f && FOV < 180.f) {
    fov_ = FOV;
  } else {
    throw std::invalid_argument("Bad FOV.");
  }
}

void Camera::setResolution(const int height, const int width) {
  if (height > 0.f && width > 0.f) {
    height_ = height;
    width_ = width;
  } else {
		throw std::invalid_argument("Bad resolution.");
	}
}

void Camera::setAspectRatio(const float ratio) { // ratio set by keeping the image width
  if (ratio > 0.f) {
    height_ = static_cast<int>(width_ / ratio);
  } else {
    throw std::invalid_argument("Bad ratio.");
  }
}

Camera::Camera(const PhysVectorF& focusPosition, const PhysVectorF& sightVector,
               float planeDistance = 1.f,
               float FOV = 90.f, int width = 1920, int height = 1080)
    : focusPoint_(focusPosition) {
				setSightVector(sightVector);
				setPlaneDistance(planeDistance);
				setFOV(FOV);
				setResolution(height, width);
}

float Camera::getTopSide() const {
	// finding top side through 2 * tan(FOV/2) * distance
  return 2.f * getPlaneDistance() * tan(getFOV() * (M_PI / 180.f) / 2.f);
};

float Camera::getPixelSide() const {
	return getTopSide()/getWidth();
}

float Camera::getNPixels(float length) const {
  return std::abs(length) / getPixelSide();
}

PhysVectorF Camera::getPointProjection(const PhysVectorF& point) const {
	PhysVectorF focus {getFocus()};
	PhysVectorF sight {getSight()};
	PhysVectorF a{focus + 
							 (focus - point) /
               ((focus - point) * sight) *
               getPlaneDistance()};
	// a = intersection of line passing through point and focus and the perspective plane

	PhysVectorF b {a - (focus + sight*getPlaneDistance())};
	// b = a relative to perspective plane ref. sys.

  // making an orthonormal base for the camera, m and o lying on the persp.
  // plane, sight for the normal vector
  PhysVectorF m;
	if (sight.x == 0. && sight.y == 0.)
		m = {0., sight.z, 0.};
	else {
  	m = {sight.y, - sight.x, 0.f};
  	m = m / m.norm();
	}
  PhysVector o {m.cross(sight)};
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

std::vector<PhysVectorF> Camera::projectParticles(const std::vector<Particle>& particles, double deltaT) const {
  std::vector<PhysVectorF> projections {};
  PhysVectorF proj{};

	for (const Particle& particle : particles) {
		proj = getPointProjection(static_cast<PhysVectorF>(particle.position + particle.speed*deltaT));
		// selecting scaling factor so that the particle is in front of the camera
		if (proj.z <= 1.f && proj.z > 0.f) {
    	projections.emplace_back(proj);
		}
  }
  return projections;
}

inline PhysVectorD& preCollSpeed(PhysVectorD& v, Wall wall) {
	switch (wall) {
		case Wall::Left:
		case Wall::Right:
			v.x = -v.x;
			break;
		case Wall::Front:
		case Wall::Back:
			v.y = -v.y;
			break;
		case Wall::Top:
		case Wall::Bottom:
			v.z = -v.z;
			break;
	}
	return v;
}

inline void preCollSpeed(PhysVectorD& v1, PhysVectorD& v2, const PhysVectorD& n) {
	PhysVectorD v10 {v1};
	v1 -= n*(n*(v1-v2));
	v2 -= n*(n*(v2-v10));
}

std::vector<PhysVectorF> Camera::projectParticles(const GasData& data, double deltaT) const {
  std::vector<PhysVectorF> projections {};
  PhysVectorF proj{};

	const std::vector<gasSim::Particle>& particles {data.getParticles()};

	if (data.getCollType() == 'w') {

		int p1I {data.getP1Index()};

		for (int i {0}; i < p1I; ++i) {
			proj = getPointProjection(static_cast<PhysVectorF>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
			if (proj.z <= 1.f && proj.z > 0.f)
    		projections.emplace_back(proj);
  	}

		{

		PhysVectorD speed {particles[p1I].speed};

		proj = getPointProjection(static_cast<PhysVectorF>(particles[p1I].position + 
					preCollSpeed(speed, data.getWall())*deltaT
					));
		if (proj.z <= 1.f && proj.z > 0.f)
    	projections.emplace_back(proj);

		}

		for (int i {p1I + 1}; i < (int) particles.size(); ++i) {
			proj = getPointProjection(static_cast<PhysVectorF>(particles[i].position + particles[i].speed*deltaT));
			if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
		}

	} else {

		int p1I {data.getP1Index()};
		int p2I {data.getP2Index()};

		PhysVectorD v1 {data.getP1().speed};
		PhysVectorD v2 {data.getP2().speed};

		{
		PhysVectorD n {data.getP1().position - data.getP2().position};
		n = n/n.norm();
		preCollSpeed(v1, v2, n);
		}

		int pIs[2] {p1I, p2I};
		PhysVectorD vs[2] {v1, v2};

		if (p2I < p1I) {
			pIs[0] = p2I;
			vs[0] = v2;
			pIs[1] = p1I;
			vs[1] = v1;
		}

		for (int i {0}; i < pIs[0]; ++i) {
			proj = getPointProjection(static_cast<PhysVectorF>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
  	}

		proj = getPointProjection(static_cast<PhysVectorF>(
					particles[pIs[0]].position + vs[0]*deltaT)
				);
		if (proj.z <= 1.f && proj.z > 0.f)
			projections.emplace_back(proj);

		for (int i {pIs[0] + 1}; i < pIs[1]; ++i) {
			proj = getPointProjection(static_cast<PhysVectorF>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
  	}

		proj = getPointProjection(static_cast<PhysVectorF>(
					particles[pIs[1]].position + vs[1]*deltaT)
				);
		if (proj.z <= 1.f && proj.z > 0.f)
			projections.emplace_back(proj);

		for (int i {pIs[1] + 1}; i < (int) particles.size(); ++i) {
			proj = getPointProjection(static_cast<PhysVectorF>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f) {
				projections.emplace_back(proj);
			}
  	}
	}
  return projections;
}

void drawParticles(const Gas& gas, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style, double deltaT) {
	sf::VertexArray particles(sf::Quads, 4*gas.getParticles().size());
	sf::Vector2u tSize {style.getPartTexture().getSize()};
	//auto projStart {std::chrono::high_resolution_clock::now()};
	std::vector<PhysVectorF> projections = camera.projectParticles(gas.getParticles(), deltaT);
	//std::chrono::duration<double> projTime {std::chrono::high_resolution_clock::now() - projStart};
	//std::cout << "Time taken to project particles: " << projTime.count() << std::endl;

	//auto sortStart {std::chrono::high_resolution_clock::now()};
	std::sort(std::execution::par, projections.begin(), projections.end(),
						[](const PhysVectorF& a, const PhysVectorF& b){
						return a.z < b.z; });
	//std::chrono::duration<double> sortTime {std::chrono::high_resolution_clock::now() - sortStart};
	//std::cout << "Time taken to sort: " << sortTime.count() << std::endl;

	// sorted projections so as to draw the closest particles over the farthest
	//auto compStart {std::chrono::high_resolution_clock::now()};
	for (const PhysVectorF& proj: projections) {
		float r {camera.getNPixels(Particle::radius) * proj.z};
		sf::Vector2f vertexes[4] {
			{proj.x - r, proj.y + r},
			{proj.x + r, proj.y + r},
			{proj.x + r, proj.y - r},
			{proj.x - r, proj.y - r}
		};
		sf::Vector2f texVertexes[4] {
			{0.f, 0.f},
			{float(tSize.x), 0.f},
			{float(tSize.x), float(tSize.y)},
			{0.f, float(tSize.y)}
		};
		for (int i {0}; i < 4; ++i)
			particles.append(sf::Vertex(vertexes[i], texVertexes[i]));
	}
	//std::chrono::duration<double> compTime {std::chrono::high_resolution_clock::now() - compStart};
	//std::cout << "Time taken to compose shapes: " << compTime.count() << std::endl;
	//auto drawStart {std::chrono::high_resolution_clock::now()};
	texture.draw(particles, &style.getPartTexture());
	//std::chrono::duration<double> drawTime {std::chrono::high_resolution_clock::now() - drawStart};
	//std::cout << "Time taken to call the draw function: " << drawTime.count() << std::endl;
}

void drawParticles(const GasData& data, const Camera &camera, sf::RenderTexture &texture, const RenderStyle& style, double deltaT) {
	sf::VertexArray particles(sf::Quads, 4*data.getParticles().size());
	sf::Vector2u tSize {style.getPartTexture().getSize()};
	
	//auto projStart {std::chrono::high_resolution_clock::now()};
	std::vector<PhysVectorF> projections = camera.projectParticles(data, deltaT);
	//std::chrono::duration<double> projTime {std::chrono::high_resolution_clock::now() - projStart};
	//std::cout << "Time taken to project particles: " << projTime.count() << std::endl;

	//auto sortStart {std::chrono::high_resolution_clock::now()};
	std::sort(std::execution::par, projections.begin(), projections.end(),
						[](const PhysVectorF& a, const PhysVectorF& b){
						return a.z < b.z; });
	//std::chrono::duration<double> sortTime {std::chrono::high_resolution_clock::now() - sortStart};
	//std::cout << "Time taken to sort: " << sortTime.count() << std::endl;

	// sorted projections so as to draw the closest particles over the farthest
	//auto compStart {std::chrono::high_resolution_clock::now()};
	for (const PhysVectorF& proj: projections) {
		float r {camera.getNPixels(Particle::radius) * proj.z};
		sf::Vector2f vertexes[4] {
			{proj.x - r, proj.y + r},
			{proj.x + r, proj.y + r},
			{proj.x + r, proj.y - r},
			{proj.x - r, proj.y - r}
		};
		sf::Vector2f texVertexes[4] {
			{0.f, 0.f},
			{float(tSize.x), 0.f},
			{float(tSize.x), float(tSize.y)},
			{0.f, float(tSize.y)}
		};
		for (int i {0}; i < 4; ++i)
			particles.append(sf::Vertex(vertexes[i], texVertexes[i]));
	}
	//std::chrono::duration<double> compTime {std::chrono::high_resolution_clock::now() - compStart};
	//std::cout << "Time taken to compose shapes: " << compTime.count() << std::endl;
	//auto drawStart {std::chrono::high_resolution_clock::now()};
	texture.draw(particles, &style.getPartTexture());
	//std::chrono::duration<double> drawTime {std::chrono::high_resolution_clock::now() - drawStart};
	//std::cout << "Time taken to call the draw function: " << drawTime.count() << std::endl;
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

template<typename GasLike>
std::array<PhysVectorF, 6> gasWallData(const GasLike& gasLike, char wall) {
	float side {static_cast<float>(gasLike.getBoxSide())};
	// brute forcing the cubical gas container face, with its center and normal vector
	switch (wall) {
		case 'u':
			return { 
				PhysVectorF(0.f, 0.f, side),
				{side, 0.f, side},
				{side, side, side},
				{0.f, side, side},
				{0.f, 0.f, 1.f},
				{side/2.f, side/2.f, side}
			};
			break;
		case 'd':
			return {
				PhysVectorF(0.f, 0.f, 0.f),
				{side, 0.f, 0.f},
				{side, side, 0.f},
				{0.f, side, 0.f},
				{0.f, 0.f, -1.f},
				{side/2.f, side/2.f, 0.f}
			};
			break;
		case 'l':
			return {
				PhysVectorF(0.f, 0.f, 0.f),
				{0.f, side, 0.f},
				{0.f, side, side},
				{0.f, 0.f, side},
				{-1.f, 0.f, 0.f},
				{0.f, side/2.f, side/2.f}
			};
			break;
		case 'r':
			return {
				PhysVectorF(side, 0.f, 0.f),
				{side, side, 0.f},
				{side, side, side},
				{side, 0.f, side},
				{1.f, 0.f, 0.f},
				{side, side/2.f, side/2.f}
			};
			break;
		case 'f':
			return {
				PhysVectorF(0.f, 0.f, 0.f),
				{side, 0.f, 0.f},
				{side, 0.f, side},
				{0.f, 0.f, side},
				{0.f, -1.f, 0.f},
				{side/2.f, 0.f, side/2.f}
			};
			break;
		case 'b':
			return {
				PhysVectorF(0.f, side, 0.f),
				{side, side, 0.f},
				{side, side, side},
				{0.f, side, side},
				{0.f, 1.f, 0.f},
				{side/2.f, side, side/2.f}
			};
			break;
	};
	return {};
}

template std::array<PhysVectorF, 6> gasWallData<Gas>(const Gas& gas, char wall);
template std::array<PhysVectorF, 6> gasWallData<GasData>(const GasData& gasData, char wall);

// the speeds after the collision are reversed, so a particle who just had a collision needs to be drawn with the speed opposite to the one it has in the moment, otherwise it is drawn shifted opposite to what it should have been

template<typename GasLike>
void drawWalls(const GasLike& gasLike, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style) {
	std::array<PhysVectorF, 6> wallData {};
	PhysVectorF wallN {};
	PhysVectorF wallCenter {};
	std::array<PhysVectorF, 4> wallVerts {};

	sf::ConvexShape wallProj;
	wallProj.setFillColor(style.getWallsColor());
	wallProj.setOutlineThickness(1);
	wallProj.setOutlineColor(style.getWOutlineColor());

	std::vector<sf::ConvexShape> frontWallPrjs {};
	std::vector<sf::ConvexShape> backWallPrjs {};

	for (char wall: style.getWallsOpts()) {
		wallData = gasWallData(gasLike, wall);
		wallVerts = {wallData[0], wallData[1], wallData[2], wallData[3]};
		wallN = wallData[4];
		wallCenter = wallData[5];
		{ // i scope
		int i {};
		// add all accepted point projections to proj (the wall's projection)
		for (const PhysVectorF& vertex: wallVerts) {
			wallProj.setPointCount(i + 1);
			PhysVectorF proj = camera.getPointProjection(vertex);
			if (proj.z < 1.f && proj.z > 0.f) {
				wallProj.setPoint(i, {proj.x, proj.y});
				++i;
			}
		} // end of i scope
		}
		// since particles lie inside the container, walls facing the Camera
		// must be drawn over them, others under
		if (wallProj.getPointCount() > 2) { // select only triangular or square projections
			if (wallN * (wallCenter - camera.getFocus()) < 0.f) {
				frontWallPrjs.emplace_back(wallProj);
			}
			else {
				backWallPrjs.emplace_back(wallProj);
			}
		}
	}

	sf::RenderTexture backWalls;
	backWalls.create(camera.getWidth(), camera.getHeight());
	backWalls.clear(style.getBGColor());
	sf::RenderTexture frontWalls;
	frontWalls.create(camera.getWidth(), camera.getHeight());
	frontWalls.clear(sf::Color::Transparent);
	// draw walls projections
	for (const sf::ConvexShape& wallPrj: backWallPrjs) {
		backWalls.draw(wallPrj);
	}
	for (const sf::ConvexShape& wallPrj: frontWallPrjs) {
		frontWalls.draw(wallPrj);
	}
	
	sf::Sprite auxSprite;
	auxSprite.setScale(1.f, -1.f);
	auxSprite.setPosition(0.f, backWalls.getSize().y);

	auxSprite.setTexture(texture.getTexture(), true);
	backWalls.draw(auxSprite); // draw particles over backWalls

	auxSprite.setTexture(frontWalls.getTexture(), true);
	backWalls.draw(auxSprite); // draw frontWalls over everything

	auxSprite.setTexture(backWalls.getTexture(), true);
	texture.clear(sf::Color::Transparent);
	texture.draw(auxSprite);
}

template void drawGas<Gas>(const Gas& gas, const Camera &camera, sf::RenderTexture &picture, const RenderStyle& style, double deltaT);
template void drawWalls<GasData>(const GasData& data, const Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

template<typename GasLike> void drawGas(const GasLike& gasLike, const Camera& camera, sf::RenderTexture& picture, const RenderStyle& style, double deltaT) {
	picture.create(camera.getWidth(), camera.getHeight());
	picture.clear(sf::Color::Transparent);
	drawParticles(gasLike, camera, picture, style, deltaT);
	drawWalls(gasLike, camera, picture, style);
}

template void drawGas<GasData>(const GasData& data, const Camera &camera, sf::RenderTexture &picture, const RenderStyle& style, double deltaT);

}  // namespace gasSim

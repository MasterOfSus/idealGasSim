#include <stdexcept>
#include <execution>
#include <thread>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "Camera.hpp"

// Getters and setters

void GS::Camera::setSightVector(Vector3f const& sightVector) {
	if (sightVector.norm() > 0.f) {
		this->sightVector = sightVector / sightVector.norm();
	} else throw std::invalid_argument("O vector cannot be normalized");
}

void GS::Camera::setPlaneDistance(const float distance) {
  if (distance > 0.f) {
    planeDistance = distance;
  } else {
    throw std::invalid_argument("Non positive distance.");
  }
}

void GS::Camera::setFOV(const float FOV) { // in degrees
  if (FOV > 0.f && FOV < 180.f) {
    fov = FOV;
  } else {
    throw std::invalid_argument("Bad FOV provided");
  }
}

void GS::Camera::setResolution(unsigned height, unsigned width) {
  this->height = height;
  this->width = width;
}

void GS::Camera::setAspectRatio(const float ratio) { // ratio set by keeping the image width
  if (ratio > 0.f) {
    height = static_cast<unsigned>(width / ratio);
  } else {
    throw std::invalid_argument("Non-positive ratio provided");
  }
}

GS::Camera::Camera(Vector3f const& focusPosition, Vector3f const& sightVector,
               float planeDistance,
               float FOV, unsigned width, unsigned height)
    : focusPoint(focusPosition) {
				setSightVector(sightVector);
				setPlaneDistance(planeDistance);
				setFOV(FOV);
				setResolution(height, width);
}

float GS::Camera::getTopSide() const {
	// finding top side through 2 * tan(FOV/2) * distance
  return 2.f * getPlaneDistance() * tan(getFOV() * (M_PI / 180.f) / 2.f);
};

float GS::Camera::getPixelSide() const {
	return getTopSide()/getWidth();
}

float GS::Camera::getNPixels(float length) const {
  return std::abs(length) / getPixelSide();
}

GS::Vector3f GS::Camera::getPointProjection(Vector3f const& point) const {
	Vector3f focus {getFocus()};
	Vector3f sight {getSight()};
	Vector3f a{focus + 
							 (focus - point) /
               ((focus - point) * sight) *
               getPlaneDistance()};
	// a = intersection of line passing through point and focus and the perspective plane

	Vector3f b {a - (focus + sight*getPlaneDistance())};
	// b = a relative to perspective plane ref. sys.

  // making an orthonormal base for the camera, m and o lying on the persp.
  // plane, sight for the normal vector
  Vector3f m;
	if (sight.x == 0. && sight.y == 0.)
		m = {0., sight.z, 0.};
	else {
  	m = {sight.y, - sight.x, 0.f};
  	m = m / m.norm();
	}
  Vector3f o {cross(m, sight)};
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

std::vector<GS::Vector3f> GS::Camera::projectParticles(std::vector<Particle> const& particles, double deltaT) const {
  std::vector<Vector3f> projections {};
  Vector3f proj{};

	for (const Particle& particle : particles) {
		proj = getPointProjection(static_cast<Vector3f>(particle.position + particle.speed*deltaT));
		// selecting scaling factor so that the particle is in front of the camera
		if (proj.z <= 1.f && proj.z > 0.f) {
    	projections.emplace_back(proj);
		}
  }
  return projections;
}

inline GS::Vector3d preCollSpeed(GS::Vector3d& v, GS::Wall wall) {
	switch (wall) {
		case GS::Wall::Left:
		case GS::Wall::Right:
			v.x = -v.x;
			break;
		case GS::Wall::Front:
		case GS::Wall::Back:
			v.y = -v.y;
			break;
		case GS::Wall::Top:
		case GS::Wall::Bottom:
			v.z = -v.z;
			break;
	}
	return v;
}

inline void preCollSpeed(GS::Vector3d& v1, GS::Vector3d& v2, const GS::Vector3d& n) {
	GS::Vector3d v10 {v1};
	v1 -= n*(n*(v1-v2));
	v2 -= n*(n*(v2-v10));
}

std::vector<GS::Vector3f> GS::Camera::projectParticles(const GasData& data, double deltaT) const {
  std::vector<Vector3f> projections {};
  Vector3f proj{};

	const std::vector<GS::Particle>& particles {data.getParticles()};

	if (data.getCollType() == 'w') {

		int p1I {data.getP1Index()};

		for (int i {0}; i < p1I; ++i) {
			proj = getPointProjection(static_cast<Vector3f>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
			if (proj.z <= 1.f && proj.z > 0.f)
    		projections.emplace_back(proj);
  	}

		{

		Vector3d speed {particles[p1I].speed};

		proj = getPointProjection(static_cast<Vector3f>(particles[p1I].position + 
					preCollSpeed(speed, data.getWall())*deltaT
					));
		if (proj.z <= 1.f && proj.z > 0.f)
    	projections.emplace_back(proj);

		}

		for (int i {p1I + 1}; i < (int) particles.size(); ++i) {
			proj = getPointProjection(static_cast<Vector3f>(particles[i].position + particles[i].speed*deltaT));
			if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
		}

	} else {

		int p1I {data.getP1Index()};
		int p2I {data.getP2Index()};

		Vector3d v1 {data.getP1().speed};
		Vector3d v2 {data.getP2().speed};

		{
		Vector3d n {data.getP1().position - data.getP2().position};
		n = n/n.norm();
		preCollSpeed(v1, v2, n);
		}

		int pIs[2] {p1I, p2I};
		Vector3d vs[2] {v1, v2};

		if (p2I < p1I) {
			pIs[0] = p2I;
			vs[0] = v2;
			pIs[1] = p1I;
			vs[1] = v1;
		}

		for (int i {0}; i < pIs[0]; ++i) {
			proj = getPointProjection(static_cast<Vector3f>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
  	}

		proj = getPointProjection(static_cast<Vector3f>(
					particles[pIs[0]].position + vs[0]*deltaT)
				);
		if (proj.z <= 1.f && proj.z > 0.f)
			projections.emplace_back(proj);

		for (int i {pIs[0] + 1}; i < pIs[1]; ++i) {
			proj = getPointProjection(static_cast<Vector3f>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f)
				projections.emplace_back(proj);
  	}

		proj = getPointProjection(static_cast<Vector3f>(
					particles[pIs[1]].position + vs[1]*deltaT)
				);
		if (proj.z <= 1.f && proj.z > 0.f)
			projections.emplace_back(proj);

		for (int i {pIs[1] + 1}; i < (int) particles.size(); ++i) {
			proj = getPointProjection(static_cast<Vector3f>(particles[i].position + particles[i].speed*deltaT));
			// selecting scaling factor so that the particle is in front of the camera
    	if (proj.z <= 1.f && proj.z > 0.f) {
				projections.emplace_back(proj);
			}
  	}
	}
  return projections;
}

void GS::drawParticles(Gas const& gas, Camera const& camera, sf::RenderTexture& texture, RenderStyle const& style, double deltaT) {
	sf::VertexArray particles(sf::Quads, 4*gas.getParticles().size());
	sf::Vector2u tSize {style.getPartTexture().getSize()};
	std::vector<Vector3f> projections = camera.projectParticles(gas.getParticles(), deltaT);
	std::sort(std::execution::par, projections.begin(), projections.end(),
						[](Vector3f const& a, const Vector3f& b){
						return a.z < b.z; });
	for (const Vector3f& proj: projections) {
		float r {camera.getNPixels(Particle::getRadius()) * proj.z};
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
	texture.draw(particles, &style.getPartTexture());
}

void GS::drawParticles(GasData const& data, Camera const& camera, sf::RenderTexture& texture, RenderStyle const& style, double deltaT) {
	sf::VertexArray particles(sf::Quads, 4*data.getParticles().size());
	sf::Vector2u tSize {style.getPartTexture().getSize()};
	
	std::vector<Vector3f> projections = camera.projectParticles(data, deltaT);
	std::sort(std::execution::par, projections.begin(), projections.end(),
						[](const Vector3f& a, const Vector3f& b){
						return a.z < b.z; });
	for (const Vector3f& proj: projections) {
		float r {camera.getNPixels(Particle::getRadius()) * proj.z};
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
	texture.draw(particles, &style.getPartTexture());
}

template<typename GasLike>
std::array<GS::Vector3f, 6> gasWallData(const GasLike& gasLike, char wall) {
	float side {static_cast<float>(gasLike.getBoxSide())};
	// brute forcing the cubical gas container face, with its center and normal vector
	switch (wall) {
		case 'u':
			return { 
				GS::Vector3f(0.f, 0.f, side),
				{side, 0.f, side},
				{side, side, side},
				{0.f, side, side},
				{0.f, 0.f, 1.f},
				{side/2.f, side/2.f, side}
			};
			break;
		case 'd':
			return {
				GS::Vector3f(0.f, 0.f, 0.f),
				{side, 0.f, 0.f},
				{side, side, 0.f},
				{0.f, side, 0.f},
				{0.f, 0.f, -1.f},
				{side/2.f, side/2.f, 0.f}
			};
			break;
		case 'l':
			return {
				GS::Vector3f(0.f, 0.f, 0.f),
				{0.f, side, 0.f},
				{0.f, side, side},
				{0.f, 0.f, side},
				{-1.f, 0.f, 0.f},
				{0.f, side/2.f, side/2.f}
			};
			break;
		case 'r':
			return {
				GS::Vector3f(side, 0.f, 0.f),
				{side, side, 0.f},
				{side, side, side},
				{side, 0.f, side},
				{1.f, 0.f, 0.f},
				{side, side/2.f, side/2.f}
			};
			break;
		case 'f':
			return {
				GS::Vector3f(0.f, 0.f, 0.f),
				{side, 0.f, 0.f},
				{side, 0.f, side},
				{0.f, 0.f, side},
				{0.f, -1.f, 0.f},
				{side/2.f, 0.f, side/2.f}
			};
			break;
		case 'b':
			return {
				GS::Vector3f(0.f, side, 0.f),
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

template std::array<GS::Vector3f, 6> gasWallData<GS::Gas>(const GS::Gas& gas, char wall);
template std::array<GS::Vector3f, 6> gasWallData<GS::GasData>(const GS::GasData& gasData, char wall);

// the speeds after the collision are reversed, so a particle who just had a collision needs to be drawn with the speed opposite to the one it has in the moment, otherwise it is drawn shifted opposite to what it should have been

template<typename GasLike>
void GS::drawWalls(GasLike const& gas, const GS::Camera& camera, sf::RenderTexture& texture, const GS::RenderStyle& style) {
	std::array<GS::Vector3f, 6> wallData {};
	GS::Vector3f wallN {};
	GS::Vector3f wallCenter {};
	std::array<GS::Vector3f, 4> wallVerts {};

	sf::ConvexShape wallProj;
	wallProj.setFillColor(style.getWallsColor());
	wallProj.setOutlineThickness(1);
	wallProj.setOutlineColor(style.getWOutlineColor());

	std::vector<sf::ConvexShape> frontWallPrjs {};
	std::vector<sf::ConvexShape> backWallPrjs {};

	for (char wall: style.getWallsOpts()) {
		wallData = gasWallData(gas, wall);
		wallVerts = {wallData[0], wallData[1], wallData[2], wallData[3]};
		wallN = wallData[4];
		wallCenter = wallData[5];
		{ // i scope
		int i {};
		// add all accepted point projections to proj (the wall's projection)
		for (GS::Vector3f const& vertex: wallVerts) {
			wallProj.setPointCount(i + 1);
			GS::Vector3f proj = camera.getPointProjection(vertex);
			if (proj.z < 1.f && proj.z > 0.f) {
				wallProj.setPoint(i, {proj.x, proj.y});
				++i;
			}
		} // end of i scope
		}
		// since particles lie inside the container, walls facing the GS::Camera
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

template void GS::drawWalls<GS::GasData>(const GasData& data, const GS::Camera& camera, sf::RenderTexture& texture, const RenderStyle& style);

template<typename GasLike> void GS::drawGas(const GasLike& gasLike, const GS::Camera& camera, sf::RenderTexture& picture, const RenderStyle& style, double deltaT) {
	picture.create(camera.getWidth(), camera.getHeight());
	picture.clear(sf::Color::Transparent);
	drawParticles(gasLike, camera, picture, style, deltaT);
	drawWalls(gasLike, camera, picture, style);
}

template void GS::drawGas<GS::Gas>(const Gas& gas, const GS::Camera &camera, sf::RenderTexture &picture, const RenderStyle& style, double deltaT);

template void GS::drawGas<GS::GasData>(const GasData& data, const GS::Camera &camera, sf::RenderTexture &picture, const RenderStyle& style, double deltaT);


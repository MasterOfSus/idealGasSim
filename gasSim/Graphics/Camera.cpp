#include "Camera.hpp"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <cmath>
#include <execution>
#include <stdexcept>

#include "../DataProcessing/GasData.hpp"

namespace GS {

// Getters and setters

void Camera::setSightVector(GSVectorF const& sightVectorV) {
  if (sightVectorV.norm() > 0.f) {
    sightVector = sightVectorV / sightVectorV.norm();
  } else {
    throw std::invalid_argument(
        "setSightVector error: O vector doesn't have direction information");
  }
}

void Camera::setPlaneDistance(const float distance) {
  if (distance > 0.f) {
    planeDistance = distance;
  } else {
    throw std::invalid_argument(
        "setPlaneDistance error: provided non positive distance");
  }
}

void Camera::setFOV(const float FOV) {  // in degrees
  if (FOV > 0.f && FOV < 180.f) {
    fov = FOV;
  } else {
    throw std::invalid_argument(
        "setFOV error: provided bad FOV (accepted range: 0.f, 180.f)");
  }
}

void Camera::setResolution(unsigned heightV, unsigned widthV) {
  if (!heightV || !widthV) {
    throw std::invalid_argument(
        "setResolution error: provided null height or width");
  }
  height = heightV;
  width = widthV;
}

void Camera::setAspectRatio(
    const float ratio) {  // ratio set by keeping the image width
  if (ratio > 0.f) {
    height = static_cast<unsigned>(static_cast<float>(width) / ratio);
  } else {
    throw std::invalid_argument(
        "setAspectRatio error: non-positive ratio provided");
  }
}

Camera::Camera(GSVectorF const& focusPosition, GSVectorF const& sightVectorV,
               float planeDistanceV, float FOV, unsigned widthV, unsigned heightV)
    : focusPoint(focusPosition) {
  setSightVector(sightVectorV);
  setPlaneDistance(planeDistanceV);
  setFOV(FOV);
  setResolution(heightV, widthV);
}

float Camera::getTopSide() const {
  return 2.f * getPlaneDistance() * tanf(getFOV() * (M_PIf / 180.f) / 2.f);
};

float Camera::getPixelSide() const { return getTopSide() / static_cast<float>(getWidth()); }

float Camera::getNPixels(float length) const {
  return std::abs(length) / getPixelSide();
}

GSVectorF Camera::getPointProjection(GSVectorF const& point) const {
  GSVectorF focus{getFocus()};
  GSVectorF sight{getSight()};
  GSVectorF a{focus +
              (focus - point) / ((focus - point) * sight) * getPlaneDistance()};
  // a = intersection of line passing through point and focus and the
  // perspective plane

  GSVectorF b{a - (focus + sight * getPlaneDistance())};
  // b = a relative to perspective plane ref. sys.

  // making an orthonormal base for the camera, m and o lying on the persp.
  // plane, sight for the normal vector
  GSVectorF m;
  if (sight.x == 0. && sight.y == 0.)
    m = {0., sight.z, 0.};
  else {
    m = {sight.y, -sight.x, 0.f};
    m = m / m.norm();
  }
  GSVectorF o{cross(m, sight)};
  m = m / getPixelSide();
  o = o / getPixelSide();

  // returning base-changed vector with scaling factor, with sign for positional
  // information as the third coordinate
  return {
      m * b + static_cast<float>(getWidth()) / 2.f, o * b + static_cast<float>(getHeight()) / 2.f,
      (a - focus) * (a - focus) /
          ((a - focus) *
           (point - focus))  // scaling factor, degenerate if > 1 V < 0
  };
}

std::vector<GSVectorF> Camera::projectParticles(
    std::vector<Particle> const& particles, double deltaT) const {
  std::vector<GSVectorF> projections{};
  GSVectorF proj{};

  for (Particle const& particle : particles) {
    proj = getPointProjection(
        static_cast<GSVectorF>(particle.position + particle.speed * deltaT));
    // selecting scaling factor so that the particle is in front of the camera
    if (proj.z <= 1.f && proj.z > 0.f) {
      projections.emplace_back(proj);
    }
  }
  return projections;
}

inline GSVectorD preCollSpeed(GSVectorD& v, Wall wall) {
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
    default:
      throw std::runtime_error("preCollSpeed error: VOID wall provided");
  }
  return v;
}

inline void preCollSpeed(GSVectorD& v1, GSVectorD& v2, GSVectorD const& n) {
  GSVectorD v10{v1};
  v1 -= n * (n * (v1 - v2));
  v2 -= n * (n * (v2 - v10));
}

std::vector<GSVectorF> Camera::projectParticles(GasData const& data,
                                                double deltaT) const {
  std::vector<GSVectorF> projections{};
  GSVectorF proj{};

  std::vector<Particle> const& particles{data.getParticles()};

  if (data.getCollType() == 'w') {
    size_t p1I{data.getP1Index()};

    for (size_t i{0}; i < p1I; ++i) {
      proj = getPointProjection(static_cast<GSVectorF>(
          particles[i].position + particles[i].speed * deltaT));
      // selecting scaling factor so that the particle is in front of the camera
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }

    {
      GSVectorD speed{particles[p1I].speed};

      proj = getPointProjection(
          static_cast<GSVectorF>(particles[p1I].position +
                                 preCollSpeed(speed, data.getWall()) * deltaT));
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }

    for (size_t i{p1I + 1}; i < particles.size(); ++i) {
      proj = getPointProjection(static_cast<GSVectorF>(
          particles[i].position + particles[i].speed * deltaT));
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }

  } else {
    size_t p1I{data.getP1Index()};
    size_t p2I{data.getP2Index()};

    GSVectorD v1{data.getP1().speed};
    GSVectorD v2{data.getP2().speed};

    {
      GSVectorD n{data.getP1().position - data.getP2().position};
      n = n / n.norm();
      preCollSpeed(v1, v2, n);
    }

    size_t pIs[2]{p1I, p2I};
    GSVectorD vs[2]{v1, v2};

    if (p2I < p1I) {
      pIs[0] = p2I;
      vs[0] = v2;
      pIs[1] = p1I;
      vs[1] = v1;
    }

    for (size_t i{0}; i < pIs[0]; ++i) {
      proj = getPointProjection(static_cast<GSVectorF>(
          particles[i].position + particles[i].speed * deltaT));
      // selecting scaling factor so that the particle is in front of the camera
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }

    proj = getPointProjection(
        static_cast<GSVectorF>(particles[pIs[0]].position + vs[0] * deltaT));
    if (proj.z <= 1.f && proj.z > 0.f) {
      projections.emplace_back(proj);
    }

    for (size_t i{pIs[0] + 1}; i < pIs[1]; ++i) {
      proj = getPointProjection(static_cast<GSVectorF>(
          particles[i].position + particles[i].speed * deltaT));
      // selecting scaling factor so that the particle is in front of the camera
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }

    proj = getPointProjection(
        static_cast<GSVectorF>(particles[pIs[1]].position + vs[1] * deltaT));
    if (proj.z <= 1.f && proj.z > 0.f) {
      projections.emplace_back(proj);
    }

    for (size_t i{pIs[1] + 1}; i < particles.size(); ++i) {
      proj = getPointProjection(static_cast<GSVectorF>(
          particles[i].position + particles[i].speed * deltaT));
      // selecting scaling factor so that the particle is in front of the camera
      if (proj.z <= 1.f && proj.z > 0.f) {
        projections.emplace_back(proj);
      }
    }
  }
  return projections;
}

void drawParticles(Gas const& gas, Camera const& camera,
                   sf::RenderTexture& texture, RenderStyle const& style,
                   double deltaT) {
  sf::VertexArray particles(sf::Quads, 4 * gas.getParticles().size());
  sf::Vector2u tSize{style.getPartTexture().getSize()};
  std::vector<GSVectorF> projections =
      camera.projectParticles(gas.getParticles(), deltaT);
  std::sort(std::execution::par, projections.begin(), projections.end(),
            [](GSVectorF const& a, GSVectorF const& b) { return a.z < b.z; });
  for (GSVectorF const& proj : projections) {
    float r{camera.getNPixels(static_cast<float>(Particle::getRadius())) * proj.z};
    sf::Vector2f vertexes[4]{{proj.x - r, proj.y + r},
                             {proj.x + r, proj.y + r},
                             {proj.x + r, proj.y - r},
                             {proj.x - r, proj.y - r}};
    sf::Vector2f texVertexes[4]{{0.f, 0.f},
                                {float(tSize.x), 0.f},
                                {float(tSize.x), float(tSize.y)},
                                {0.f, float(tSize.y)}};
    for (size_t i{0}; i < 4; ++i)
      particles.append(sf::Vertex(vertexes[i], texVertexes[i]));
  }
	texture.setActive();
  texture.draw(particles, &style.getPartTexture());
}

void drawParticles(GasData const& data, Camera const& camera,
                   sf::RenderTexture& texture, RenderStyle const& style,
                   double deltaT) {
  sf::VertexArray particles(sf::Quads, 4 * data.getParticles().size());
  sf::Vector2u tSize{style.getPartTexture().getSize()};

  std::vector<GSVectorF> projections = camera.projectParticles(data, deltaT);
  std::sort(std::execution::par, projections.begin(), projections.end(),
            [](GSVectorF const& a, GSVectorF const& b) { return a.z < b.z; });
  for (GSVectorF const& proj : projections) {
    float r{camera.getNPixels(static_cast<float>(Particle::getRadius())) * proj.z};
    sf::Vector2f vertexes[4]{{proj.x - r, proj.y + r},
                             {proj.x + r, proj.y + r},
                             {proj.x + r, proj.y - r},
                             {proj.x - r, proj.y - r}};
    sf::Vector2f texVertexes[4]{{0.f, 0.f},
                                {float(tSize.x), 0.f},
                                {float(tSize.x), float(tSize.y)},
                                {0.f, float(tSize.y)}};
    for (size_t i{0}; i < 4; ++i)
      particles.append(sf::Vertex(vertexes[i], texVertexes[i]));
  }
  texture.draw(particles, &style.getPartTexture());
}

template <typename GasLike>
std::array<GSVectorF, 6> gasWallData(GasLike const& gasLike, char wall) {
  float side{static_cast<float>(gasLike.getBoxSide())};
  // brute forcing the cubical gas container face, with its center and normal
  // vector
  switch (wall) {
    case 'u':
      return {GSVectorF(0.f, 0.f, side), {side, 0.f, side},
              {side, side, side},        {0.f, side, side},
              {0.f, 0.f, 1.f},           {side / 2.f, side / 2.f, side}};
      break;
    case 'd':
      return {GSVectorF(0.f, 0.f, 0.f), {side, 0.f, 0.f},
              {side, side, 0.f},        {0.f, side, 0.f},
              {0.f, 0.f, -1.f},         {side / 2.f, side / 2.f, 0.f}};
      break;
    case 'l':
      return {GSVectorF(0.f, 0.f, 0.f), {0.f, side, 0.f},
              {0.f, side, side},        {0.f, 0.f, side},
              {-1.f, 0.f, 0.f},         {0.f, side / 2.f, side / 2.f}};
      break;
    case 'r':
      return {GSVectorF(side, 0.f, 0.f), {side, side, 0.f},
              {side, side, side},        {side, 0.f, side},
              {1.f, 0.f, 0.f},           {side, side / 2.f, side / 2.f}};
      break;
    case 'f':
      return {GSVectorF(0.f, 0.f, 0.f), {side, 0.f, 0.f},
              {side, 0.f, side},        {0.f, 0.f, side},
              {0.f, -1.f, 0.f},         {side / 2.f, 0.f, side / 2.f}};
      break;
    case 'b':
      return {GSVectorF(0.f, side, 0.f), {side, side, 0.f},
              {side, side, side},        {0.f, side, side},
              {0.f, 1.f, 0.f},           {side / 2.f, side, side / 2.f}};
      break;
  };
  return {};
}

template std::array<GSVectorF, 6> gasWallData<Gas>(Gas const& gas, char wall);
template std::array<GSVectorF, 6> gasWallData<GasData>(GasData const& gasData,
                                                       char wall);

// the speeds after the collision are reversed, so a particle who just had a
// collision needs to be drawn with the speed opposite to the one it has in the
// moment, otherwise it is drawn shifted opposite to what it should have been

template <typename GasLike>
void drawWalls(GasLike const& gas, const Camera& camera,
               sf::RenderTexture& texture, RenderStyle const& style) {
  std::array<GSVectorF, 6> wallData{};
  GSVectorF wallN{};
  GSVectorF wallCenter{};
  std::array<GSVectorF, 4> wallVerts{};

  sf::ConvexShape wallProj;
  wallProj.setFillColor(style.getWallsColor());
  wallProj.setOutlineThickness(1);
  wallProj.setOutlineColor(style.getWOutlineColor());

  std::vector<sf::ConvexShape> frontWallPrjs{};
  std::vector<sf::ConvexShape> backWallPrjs{};

  for (char wall : style.getWallsOpts()) {
    wallData = gasWallData(gas, wall);
    wallVerts = {wallData[0], wallData[1], wallData[2], wallData[3]};
    wallN = wallData[4];
    wallCenter = wallData[5];
    {  // i scope
      size_t i{};
      // add all accepted point projections to proj (the wall's projection)
      for (GSVectorF const& vertex : wallVerts) {
        wallProj.setPointCount(i + 1);
        GSVectorF proj = camera.getPointProjection(vertex);
        if (proj.z < 1.f && proj.z > 0.f) {
          wallProj.setPoint(i, {proj.x, proj.y});
          ++i;
        }
      }  // end of i scope
    }
    // since particles lie inside the container, walls facing the Camera
    // must be drawn over them, others under
    if (wallProj.getPointCount() >
        2) {  // select only triangular or square projections
      if (wallN * (wallCenter - camera.getFocus()) < 0.f) {
        frontWallPrjs.emplace_back(wallProj);
      } else {
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
  for (sf::ConvexShape const& wallPrj : backWallPrjs) {
    backWalls.draw(wallPrj);
  }
  for (sf::ConvexShape const& wallPrj : frontWallPrjs) {
    frontWalls.draw(wallPrj);
  }

  sf::Sprite auxSprite;
  auxSprite.setScale(1.f, -1.f);
  auxSprite.setPosition(0.f, static_cast<float>(backWalls.getSize().y));

  auxSprite.setTexture(texture.getTexture(), true);
  backWalls.draw(auxSprite);  // draw particles over backWalls

  auxSprite.setTexture(frontWalls.getTexture(), true);
  backWalls.draw(auxSprite);  // draw frontWalls over everything

  auxSprite.setTexture(backWalls.getTexture(), true);
  texture.clear(sf::Color::Transparent);
  texture.draw(auxSprite);
}

template void drawWalls<GasData>(GasData const& data, Camera const& camera,
                                 sf::RenderTexture& texture,
                                 RenderStyle const& style);

template <typename GasLike>
void drawGas(GasLike const& gasLike, Camera const& camera,
             sf::RenderTexture& picture, RenderStyle const& style,
             double deltaT) {
  picture.create(camera.getWidth(), camera.getHeight());
  picture.clear(sf::Color::Transparent);
  drawParticles(gasLike, camera, picture, style, deltaT);
  drawWalls(gasLike, camera, picture, style);
}

template void drawGas<Gas>(Gas const& gas, Camera const& camera,
                           sf::RenderTexture& picture, RenderStyle const& style,
                           double deltaT);

template void drawGas<GasData>(GasData const& data, Camera const& camera,
                               sf::RenderTexture& picture,
                               RenderStyle const& style, double deltaT);
}  // namespace GS

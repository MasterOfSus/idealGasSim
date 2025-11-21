#ifndef CAMERAHPP
#define CAMERAHPP

#include "RenderStyle.hpp"

namespace GS {
class GasData;

	class Camera {
	public:

		 // parametric constructor
		Camera(Vector3f const& focusPosition, Vector3f const& sightVector,
					 float planeDistance = 1.f, float fov = 90.f,
					 unsigned width = 1920, unsigned height = 1080);

		 // setters and getters
		void setFocus(Vector3f const& focusPoint) { this->focusPoint = focusPoint; };
		void setSightVector(Vector3f const& sightVector);
		void setAspectRatio(float ratio);
		void setPlaneDistance(float distance);
		void setFOV(float FOV); // field of view setting (in degrees)
		void setResolution(unsigned height, unsigned width);

		Vector3f const& getFocus() const { return focusPoint; };
		Vector3f const& getSight() const { return sightVector; };
		float getAspectRatio() const {

			return static_cast<float>(width) / static_cast<float>(height);
		};
		float getPlaneDistance() const { return planeDistance; };
		float getFOV() const { return fov; };
		int getHeight() const { return height; };
		int getWidth() const { return width; };

		// useful functions
		Vector3f getPointProjection(Vector3f const& point) const;
		std::vector<Vector3f> projectParticles (std::vector<Particle> const& particles, double deltaT = 0.) const;
		std::vector<Vector3f> projectParticles(GasData const& data, double deltaT) const;

		// auxiliary member functions
		float getTopSide() const;
		float getPixelSide() const;
		float getNPixels(float length) const;

	 private:

		Vector3f focusPoint;
		Vector3f sightVector;
		float planeDistance;
		float fov;

		unsigned width;
		unsigned height;

	};

	template<typename GasLike>
	void drawWalls(GasLike const& gas, Camera const& camera, sf::RenderTexture& texture, RenderStyle const& style);

	void drawParticles(Gas const& gas, Camera const& camera, sf::RenderTexture& texture, RenderStyle const& style, double deltaT = 0.);

	void drawParticles(GasData const& data, Camera const& camera, sf::RenderTexture& texture, RenderStyle const& style, double deltaT = 0.);

	template<typename GasLike>
	void drawGas(GasLike const& gasLike, Camera const& camera, sf::RenderTexture& picture, RenderStyle const& style, double deltaT = 0.);

}

#endif

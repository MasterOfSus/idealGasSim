#ifndef GASDATA_HPP
#define GASDATA_HPP

#include "PhysicsEngine.hpp"

namespace GS {
class GasData {
	public:
		GasData(const Gas& gas, const Collision* collision);

		const std::vector<Particle>& getParticles() const { return particles; };
		const Particle& getP1() const { return particles[p1Index]; };
		size_t getP1Index() const { return p1Index; };
		const Particle& getP2() const;
		size_t getP2Index() const;
		double getT0() const { return t0; }
		double getTime() const { return time; };
		double getBoxSide() const { return boxSide; };
		Wall getWall() const;
		char getCollType() const;

		bool operator==(const GasData& data) const;

 	private:
		std::vector<Particle> particles;
		double t0;
		double time;
		double boxSide;
		size_t p1Index;
		size_t p2Index;
		Wall wall;
	};
}

#endif

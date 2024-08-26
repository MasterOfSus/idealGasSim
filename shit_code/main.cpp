#include <iostream>
#include <random>
#include <algorithm>
#include <iterator>
#include <cassert>
#include "namespacer.hpp"

namespace thermo {

// static variables initalization

double Particle::mass_{1.66 * 4};
double Particle::radius_{0.143};

double total_time {0.};

// a set of three auxiliary functions returning parameters for a reduced
// quadratic formula
double get_a(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return std::pow(((*P1).speed_.x_ - (*P2).speed_.x_), 2) +
         std::pow(((*P1).speed_.y_ - (*P2).speed_.y_), 2) +
         std::pow(((*P1).speed_.z_ - (*P2).speed_.z_), 2);
}

double get_b(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return ((*P1).position_.x_ - (*P2).position_.x_) *
             ((*P1).speed_.x_ - (*P2).speed_.x_) +
         ((*P1).position_.y_ - (*P2).position_.y_) *
             ((*P1).speed_.y_ - (*P2).speed_.y_) +
         ((*P1).position_.z_ - (*P2).position_.z_) *
             ((*P1).speed_.z_ - (*P2).speed_.z_);
}

double get_c(const std::vector<Particle>::iterator P1,
             const std::vector<Particle>::iterator P2) {
  return std::pow(((*P1).position_.x_ - (*P2).position_.x_), 2) +
         std::pow(((*P1).position_.y_ - (*P2).position_.y_), 2) +
         std::pow(((*P1).position_.z_ - (*P2).position_.z_), 2) -
         4 * pow((*P1).radius_, 2);
}

// all class member functions

// physvector implementation
double norm(const PhysVector& vector) {
  return std::sqrt(vector.x_ * vector.x_ + vector.y_ * vector.y_ +
                   vector.z_ * vector.z_);
}

PhysVector operator+(const PhysVector& v1, const PhysVector& v2) {
	return {v1.x_ + v2.x_, v1.y_ + v2.y_, v1.z_ + v2.z_};
}

PhysVector operator*(const PhysVector& vector, double factor) {
  return {factor * vector.x_, factor * vector.y_, factor * vector.z_};
}

double operator*(const PhysVector& v1, const PhysVector& v2) {
  return v1.x_ * v2.x_ + v1.y_ * v2.y_ + v1.z_ * v2.z_;
}

PhysVector operator-(const PhysVector& v1, const PhysVector& v2) {
	return v1 + v2*-1;
}

PhysVector operator/(const PhysVector& vector, double factor) {
  return {vector.x_/factor, vector.y_/factor, vector.z_/factor};
}

void operator+=(PhysVector& v1, const PhysVector& v2) {
	v1 = v1 + v2;
}

void operator-=(PhysVector& v1, const PhysVector& v2) {
	v1 = v1 - v2;
}

// wall implementation

bool operator==(Wall wall1, Wall wall2) {
	return wall1.wall_type_ == wall2.wall_type_;
}

// collision implementation

void Collision::wipe() {
	clusters_.clear();
	time_ = 100000.;
}

double Collision::get_time() const {
	return time_;
}

// gas implementation
Gas::Gas(const Gas& gas)
    : particles_(gas.particles_.begin(), gas.particles_.end()),
      side_{gas.side_} {}

Gas::Gas(int n, double l, double temperature) : side_{l} {
  std::default_random_engine eng(std::time(nullptr));
  std::uniform_real_distribution<double> dist(
      -sqrt(2 * temperature / (3 * Particle::mass_)), sqrt(2 * temperature / (3 * Particle::mass_)));

  int num = static_cast<int>(std::ceil(pow(n, 1./3.)));

	if (l/static_cast<double>(num) < 2*Particle::radius_) {
		throw std::runtime_error("Too many particles to fit in a square grid!!");
	}

  for (int i{1}; i != (num + 1); ++i) {
    for (int j{1}; j != (num + 1); ++j) {
      for (int k{1}; k != (num + 1) && static_cast<int>(particles_.size()) != n; ++k) {
        particles_.push_back({{i * side_ / (num + 1), j * side_ / (num + 1),
                               k * side_ / (num + 1)},
                              {dist(eng), dist(eng), dist(eng)}});
				// std::cout << "Created a particle at " << "(" << i * side_ / (num + 1) << ", " << j * side_ / (num + 1) << ", " << k * side_ / (num + 1) << ")\n";
      }
    }
  }
}

double collision_time(const std::vector<Particle>::iterator P1,
                      const std::vector<Particle>::iterator P2) {
  double a{get_a(P1, P2)};
  double b{get_b(P1, P2)};
  double c{get_c(P1, P2)};
  double result{100000.};

  double delta{std::pow(b, 2) - a * c};

  if (delta > 0) {
    delta = sqrt(delta);
    double t1{(-b - delta) / a};
    double t2{(-b + delta) / a};
    if (t1 > 0) {
      result = t1;
    } else if (t2 > 0) {
      result = t2;
    }
  }
	assert(result > 0);
	/*if (result < 100000)*/ std::cout << "Calculated time for collision between two particles: " << result << "\n";
  return result;
}

double collision_time(const std::vector<Particle>::iterator P1,
                      const Wall wall, double side) {
  double t {100000.};
  switch (wall.wall_type_) {
    case 'x':
      if ((*P1).speed_.x_ < 0) {
        t = -(*P1).position_.x_ / (*P1).speed_.x_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.x_) / (*P1).speed_.x_;
      }
      break;
    case 'y':
      if ((*P1).speed_.y_ < 0) {
        t = -(*P1).position_.y_ / (*P1).speed_.y_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.y_) / (*P1).speed_.y_;
      }
      break;
    case 'z':
      if ((*P1).speed_.z_ < 0) {
        t = -(*P1).position_.z_ / (*P1).speed_.z_;
      } else {
        t = (side - (2 * (*P1).radius_) - (*P1).position_.z_) / (*P1).speed_.z_;
      }
      break;
  }
	std::cout << "Calculated time for collision between " << wall.wall_type_ << " wall and particle: " << t << "\n";
	assert(t > 0);
	return t;
}

void Collision::insert_Cluster(const Cluster& input_cluster, double side) {
	bool particle_touches {true};
	bool wall_touches {true};
	bool are_together {false};
	for (std::vector<Particle>::iterator input_particle_it: input_cluster.particles_iterators_) {
		for (Cluster cluster: clusters_) {
			for (std::vector<Particle>::iterator cluster_particle_it: cluster.particles_iterators_) {
				particle_touches &= (time_ == collision_time(input_particle_it, cluster_particle_it));
				if (!particle_touches) break;
			}
			if (particle_touches) {
				cluster.particles_iterators_.push_back(input_particle_it);
			}
		}
	}
	for (Wall wall: input_cluster.walls_) {
		for (Cluster cluster: clusters_) {
			for (std::vector<Particle>::iterator cluster_particle_it: cluster.particles_iterators_) {
				wall_touches &= (time_ == collision_time(cluster_particle_it, wall, side));
				if (!wall_touches) break;
			}
			if (wall_touches && (std::find(cluster.walls_.begin(), cluster.walls_.end(), wall) == cluster.walls_.end())) {
				cluster.walls_.push_back(wall);
			}
		}
	}
	for (Cluster cluster: clusters_) {
		for (Wall wall: input_cluster.walls_) {
			are_together &= (std::find(cluster.walls_.begin(), cluster.walls_.end(), wall) != cluster.walls_.end());
		}
		for (std::vector<Particle>::iterator cluster_particle_it: input_cluster.particles_iterators_) {
			are_together &= (std::find(cluster.particles_iterators_.begin(), cluster.particles_iterators_.end(), cluster_particle_it) != cluster.particles_iterators_.end());
		}
		if (are_together) break;
	}
	if (!are_together) {
		 clusters_.push_back(input_cluster);
	}
}

void Collision::set_time(double side) {
	if ((*(clusters_.begin())).particles_iterators_.size() == 2)
		time_ = collision_time(*((*(clusters_.begin())).particles_iterators_.begin()), *((*(clusters_.begin())).particles_iterators_.begin() + 1));
	else
		time_ = collision_time(*((*(clusters_.begin())).particles_iterators_.begin()), *((*(clusters_.begin())).walls_.begin()), side);
	// std::cout << "Set time for collision at: " << time_ << "\n";
	assert(time_ > 0 && time_ != NAN);
}

void Collision::set_time(double side, double time) {
	time_ = time*side;
}

Collision Gas::get_next_collision() {
  double aux_time{0.};

	Collision collision;

	collision.set_time(1., 100000.);

	for (auto it1 {particles_.begin()}; it1 != particles_.end(); ++it1) {

		for	(Wall wall: std::vector<Wall> {{'x'},{'y'},{'z'}}) {
			aux_time = collision_time(it1, wall, side_);
			if (aux_time > 0.0000001 && aux_time < collision.get_time()) {
				collision.wipe();
				collision.insert_Cluster({{it1}, {wall}}, side_);
				collision.set_time(side_);
			}
			else if (aux_time == collision.get_time()) {
				collision.insert_Cluster({{it1}, {wall}}, side_);
				collision.set_time(side_);
			}
		}

		for (auto it2 {it1 + 1}; it2 != particles_.end(); ++it2) {
			aux_time = collision_time(it1, it2);
			if (aux_time > 0.0000001 && aux_time < collision.get_time()) {
				collision.wipe();
				collision.insert_Cluster({{it1, it2}, {}}, side_);
				collision.set_time(side_);
			}
			else if (aux_time == collision.get_time()) {
				collision.insert_Cluster({{it1, it2}, {}}, side_);
				collision.set_time(side_);
			}
		}
	}
	assert(collision.get_time() > 0);
	return collision;
}

void solve_cluster(Cluster& cluster) {
	if (cluster.particles_iterators_.size() == 2) {
		std::cout << "Solving two-particle cluster with results: \n";
		PhysVector n {(*(*(cluster.particles_iterators_.begin()))).position_ - (*(*(cluster.particles_iterators_.begin()+1))).position_};
		std::cout << "n norm before normalization = " << norm(n) << "\n";
		n = n / norm(n);
		std::cout << "n norm = " << norm(n) << "\n";
		std::cout << "P1 = (" << (*(*(cluster.particles_iterators_.begin()))).position_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.z_ << ")" << " S1 = (" << (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << ")\n";
		std::cout << "P2 = (" << (*(*(cluster.particles_iterators_.begin()+1))).position_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).position_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).position_.z_ << ")" << " S2 = (" << (*(*(cluster.particles_iterators_.begin()+1))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).speed_.z_ << ")\n";
		PhysVector momentum_var_1 {n * (n * ((*(*(cluster.particles_iterators_.begin() + 1))).speed_ - (*(*(cluster.particles_iterators_.begin()))).speed_))};
		(*(*(cluster.particles_iterators_.begin()))).speed_ += momentum_var_1;
		(*(*(cluster.particles_iterators_.begin() + 1))).speed_ -= momentum_var_1;
		std::cout << "S1'	= (" << (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << ")\n";
		std::cout << "S2'	= (" << (*(*(cluster.particles_iterators_.begin()+1))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()+1))).speed_.z_ << ")\n";
	} else if (cluster.particles_iterators_.size() == 1 && cluster.walls_.size() == 1) {
			std::cout << "Solving particle-wall cluster with results:\n";
			switch ((*(cluster.walls_.begin())).wall_type_) {
				case 'x':
					std::cout << "P = (" << (*(*(cluster.particles_iterators_.begin()))).position_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.z_ << "), S = (" << (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << "), S'x = ";
					(*(*(cluster.particles_iterators_.begin()))).speed_.x_ = -((*(*(cluster.particles_iterators_.begin()))).speed_.x_);
					std::cout	<< (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << "\n";
					break;
				case 'y':
					std::cout << "P = (" << (*(*(cluster.particles_iterators_.begin()))).position_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.z_ << "), S = (" << (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << "), S'y = ";
					(*(*(cluster.particles_iterators_.begin()))).speed_.y_ = -((*(*(cluster.particles_iterators_.begin()))).speed_.y_);
					std::cout	<< (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << "\n";
					break;
				case 'z':
					std::cout << "P = (" << (*(*(cluster.particles_iterators_.begin()))).position_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).position_.z_ << "), S = (" << (*(*(cluster.particles_iterators_.begin()))).speed_.x_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.y_ << ", " << (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << "), S'z = ";
					(*(*(cluster.particles_iterators_.begin()))).speed_.z_ = -((*(*(cluster.particles_iterators_.begin()))).speed_.z_);
					std::cout	<< (*(*(cluster.particles_iterators_.begin()))).speed_.z_ << "\n";
					break;
		}
	} else throw std::runtime_error("Collision with more than two elements.");
}

void Collision::solve_collisions() {
	int i {0};
	for (Cluster& cluster: clusters_) {
		solve_cluster(cluster);
		++i;
	}
	std::cout << i << " clusters solved for this collision.\n";
	i = 0;
}

void Particle::move(double time) {
	// std::cout << "Adding vector of norm " << norm(speed_*time) << " to position vector P = (" << position_.x_ << ", " << position_.y_ << ", " << position_.z_ << ") -> ";
	position_ += (speed_*time);
	// std::cout << "P' = ("<< position_.x_ << ", " << position_.y_ << ", " << position_.z_ << ")\n";
}

void Gas::update_gas_state() {
	Collision collision = this->get_next_collision();
	total_time += collision.get_time();
	std::cout << "Chosen collision time: " << collision.get_time() << "\n";
	std::cout << "Calculated new collision at time: " << total_time << "\n";
	for (Particle& particle: particles_) {
		assert(collision.get_time() != 0);
		particle.move(collision.get_time());
	}
	collision.solve_collisions();
	collision.wipe();
}

void Gas::output_particles_positions() {
	for (Particle particle: particles_) {
		std::cout << "(" << particle.position_.x_ << ", " << particle.position_.y_ << ", " << particle.position_.z_ << ")\n";
	}
}


// end of member functions

}  // namespace thermo

int main() {
	thermo::Particle::radius_ = 1.5;
	thermo::Gas gas = {3000, 50., 0.3};
	// gas.output_particles_positions();
	int i {10};
	while (--i) {
		try {
			gas.update_gas_state();
		} catch (std::runtime_error& error) {
			std::cout << error.what() << "\n";
			exit(1);
		}
	}
	// gas.output_particles_positions();
}

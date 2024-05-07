#include <cmath>

namespace thermo {
    struct PhysVector {
        double x, y, z;
        double get_module() {
            return std::sqrt(x*x+y*y+z*z);
        }
        PhysVector(double _x{0.0}, double _y{0.0}, double _z{0.0}) : x{x_i}, y{y_i}, z{z_i} {

        }
    }

    class Particle {
        static double mass;
        short int last_collision;
        PhysVector position;
        PhysVector speed;
        PhysVector momentum;
    }

    class Gas {
        void update_gas_state(); // called in each iteration of the game loop
        public:
        double get_pressure(); // returns the value of the pressure of the gas
        int get_volume(); // returns the volume of the gas
        double get_temperature(); // returns the temperature of the gas
    }
}
#pragma once

#include "../manipulator.h"


namespace smlt {
namespace particles {

class SizeManipulator : public Manipulator {
public:
    SizeManipulator():
        Manipulator("scalar") {}

    void set_property(const std::string& name, float value) {
        if(name == "rate") rate_ = value;
    }

    void set_property(const std::string& name, int32_t value) {
        if(name == "rate") rate_ = (float) value;
    }

private:
    float rate_ = 0.1f;

    void do_manipulate(std::vector<Particle>& particles, float dt) {
        auto rate_diff = 1.0f + (rate_ * dt);
        for(auto& particle: particles) {
            particle.dimensions.x = std::max(0.0f, particle.dimensions.x * rate_diff);
            particle.dimensions.y = std::max(0.0f, particle.dimensions.y * rate_diff);
        }
    }
};


}
}

#pragma once

#include "../../types.h"
#include "../stage_node_behaviour.h"


namespace smlt {

class Organism;

namespace behaviours {

class SmoothFollow:
    public StageNodeBehaviour,
    public Managed<SmoothFollow> {

public:
    SmoothFollow();

    const std::string name() const {
        return "Smooth Follow";
    }

    void late_update(float dt) override;
    void set_target(ActorPtr actor);
    void set_target(ParticleSystemPtr ps);

    void set_follow_distance(float dist) { distance_ = dist; }
    void set_follow_height(float height) { height_ = height; }
    void set_damping(float damping);
    void set_rotation_damping(float damping);

private:
    StageNode* target_ = nullptr;

    float distance_ = 10.0;
    float height_ = 10.0;

    float damping_ = 5.0;
    float rotation_damping_ = 10.0;

};

}
}

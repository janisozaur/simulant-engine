#ifndef INTERFACES_H
#define INTERFACES_H

namespace kglt {

class KeyFrameAnimated {
public:
    virtual void add_animation(
        const unicode& name,
        uint32_t start_frame, uint32_t end_frame,
        float duration
    ) = 0;

    virtual void set_next_animation(const unicode& name) = 0;
    virtual void set_current_animation(const unicode& name) = 0;
    virtual void update(double dt) = 0;
};

}

#endif // INTERFACES_H

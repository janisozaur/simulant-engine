#include "input.h"
#include "input_manager.h"
#include "../input_controller.h"

namespace smlt {

InputManager::InputManager(InputController *controller):
    controller_(controller) {

    auto horizontal = new_axis("Horizontal");
    horizontal->set_positive_keyboard_key(KEYBOARD_CODE_D);
    horizontal->set_negative_keyboard_key(KEYBOARD_CODE_A);

    auto horizontal_alt = new_axis("Horizontal");
    horizontal_alt->set_positive_keyboard_key(KEYBOARD_CODE_RIGHT);
    horizontal_alt->set_negative_keyboard_key(KEYBOARD_CODE_LEFT);

    auto vertical = new_axis("Vertical");
    vertical->set_positive_keyboard_key(KEYBOARD_CODE_W);
    vertical->set_negative_keyboard_key(KEYBOARD_CODE_S);

    auto vertical_alt = new_axis("Vertical");
    vertical_alt->set_positive_keyboard_key(KEYBOARD_CODE_UP);
    vertical_alt->set_negative_keyboard_key(KEYBOARD_CODE_DOWN);

    auto fire1 = new_axis("Fire1");
    fire1->set_positive_keyboard_key(KEYBOARD_CODE_LCTRL);

    auto fire2 = new_axis("Fire2");
    fire2->set_positive_keyboard_key(KEYBOARD_CODE_LALT);
}

InputAxis* InputManager::new_axis(const std::string& name) {
    auto axis = InputAxis::create(name);
    axises_.push_back(axis);
    return axis.get();
}

AxisList InputManager::axises(const std::string& name) const {
    AxisList result;
    for(auto& axis: axises_) {
        result.push_back(axis.get());
    }
    return result;
}

void InputManager::each_axis(EachAxisCallback callback) {
    for(auto& axis: axises_) {
        callback(axis.get());
    }
}

void InputManager::delete_axises(const std::string& name) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [&name](InputAxis::ptr axis) -> bool {
                return axis->name() == name;
            }
        ), axises_.end()
    );
}

void InputManager::delete_axis(InputAxis* axis) {
    axises_.erase(
        std::remove_if(axises_.begin(), axises_.end(),
            [axis](InputAxis::ptr ax) -> bool {
                return ax.get() == axis;
            }
        ), axises_.end()
    );
}

std::size_t InputManager::axis_count(const std::string &name) const {
    return std::count_if(axises_.begin(), axises_.end(), [&name](std::shared_ptr<InputAxis> axis) -> bool {
        return axis->name() == name;
    });
}

float sgn(float v) {
    if(v > 0) { return 1.0f; }
    else { return -1.0f; }
}

void InputManager::_update_mouse_button_axis(InputAxis* axis, float dt) {
    float new_value = 0.0f;

    auto pbtn = axis->positive_mouse_button();
    auto nbtn = axis->negative_mouse_button();

    bool positive_pressed = false;
    bool negative_pressed = false;

    auto process_mouse = [this, pbtn, nbtn, &positive_pressed, &negative_pressed](MouseID id) {
        if(pbtn != -1 && controller_->mouse_button_state(id, pbtn)) {
            positive_pressed = true;
        }

        if(nbtn != -1 && controller_->mouse_button_state(id, nbtn)) {
            negative_pressed = true;
        }
    };

    // If the user requested input from all mice, do that
    if(axis->mouse_source() == ALL_MICE) {
        for(std::size_t i = 0; i < controller_->mouse_count(); ++i) {
            MouseID id = (MouseID) i;
            process_mouse(id);
        }
    } else {
        // Otherwise just check the one they asked for
        MouseID id = axis->mouse_source();
        process_mouse(id);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) new_value = 1.0f;
    if(negative_pressed) new_value = -1.0f;

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;
}

void InputManager::_update_joystick_button_axis(InputAxis* axis, float dt) {
    float new_value = 0.0f;

    auto pbtn = axis->positive_joystick_button();
    auto nbtn = axis->negative_joystick_button();

    bool positive_pressed = false;
    bool negative_pressed = false;

    auto process_joystick = [this, pbtn, nbtn, &positive_pressed, &negative_pressed](JoystickID id) {
        if(pbtn != -1 && controller_->joystick_button_state(id, pbtn)) {
            positive_pressed = true;
        }

        if(nbtn != -1 && controller_->joystick_button_state(id, nbtn)) {
            negative_pressed = true;
        }
    };

    // If the user requested input from all mice, do that
    if(axis->joystick_source() == ALL_JOYSTICKS) {
        for(std::size_t i = 0; i < controller_->joystick_count(); ++i) {
            JoystickID id = (JoystickID) i;
            process_joystick(id);
        }
    } else {
        // Otherwise just check the one they asked for
        JoystickID id = axis->joystick_source();
        process_joystick(id);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) new_value = 1.0f;
    if(negative_pressed) new_value = -1.0f;

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;
}

void InputManager::_update_keyboard_axis(InputAxis* axis, float dt) {
    float new_value = 0.0f;

    auto pkey = axis->positive_keyboard_key();
    auto nkey = axis->negative_keyboard_key();

    bool positive_pressed = false;
    bool negative_pressed = false;

    auto process_keyboard = [this, pkey, nkey, &positive_pressed, &negative_pressed](KeyboardID id) {
        if(pkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, pkey)) {
            positive_pressed = true;
        }

        if(nkey != KEYBOARD_CODE_NONE && controller_->keyboard_key_state(id, nkey)) {
            negative_pressed = true;
        }
    };

    // If the user requested input from all keyboards, do that
    if(axis->keyboard_source() == ALL_KEYBOARDS) {
        for(std::size_t i = 0; i < controller_->keyboard_count(); ++i) {
            KeyboardID id = (KeyboardID) i;
            process_keyboard(id);
        }
    } else {
        // Otherwise just check the one they asked for
        KeyboardID id = axis->keyboard_source();
        process_keyboard(id);
    }

    // If either positive or negative were pressed, adjust the value
    if(positive_pressed) new_value = 1.0f;
    if(negative_pressed) new_value = -1.0f;

    // If neither were pressed, then apply the return speed (making sure we don't pass zero)
    if(!positive_pressed && !negative_pressed) {
        auto sign = sgn(axis->value());
        new_value = std::max(0.0f, (abs(axis->value()) - (axis->return_speed_ * dt))) * sign;
    }

    axis->value_ = new_value;
}

void InputManager::update(float dt) {
    for(auto axis: axises_) {

        auto type = axis->type();
        if(type == AXIS_TYPE_KEYBOARD_KEY) {
            _update_keyboard_axis(axis.get(), dt);
        } else if(type == AXIS_TYPE_MOUSE_BUTTON) {
            _update_mouse_button_axis(axis.get(), dt);
        } else if(type == AXIS_TYPE_JOYSTICK_BUTTON) {
            _update_joystick_button_axis(axis.get(), dt);
        } else if(type == AXIS_TYPE_MOUSE_AXIS) {
            _update_mouse_axis_axis(axis.get(), dt);
        }
    }
}

void InputManager::_update_mouse_axis_axis(InputAxis *axis, float dt) {
    float new_value = 0.0f;

    auto process_mouse = [this, axis](MouseID mouse_id) {
        // We just store the mouse axis state directly, this will be a relative delta
        // from the last frame for either mouse movement, or scroll wheel
        return controller_->mouse_axis_state(mouse_id, axis->mouse_axis_);
    };

    // If the source is *all* mice, store the strongest axis (whether positive or negative)
    if(axis->mouse_source() == ALL_MICE) {
        for(std::size_t i = 0; i < controller_->mouse_count(); ++i) {
            auto this_value = process_mouse((MouseID) i);
            if(abs(this_value) > abs(new_value)) {
                new_value = this_value;
            }
        }
    } else {
        new_value = process_mouse(axis->mouse_source());
    }

    axis->value_ = new_value;
}


float InputManager::axis_value(const std::string& name) const {
    auto f = 0.0f;
    for(auto axis: axises(name)) {
        auto v = axis->value();

        // Return the result with the greatest overall value (positive or negative)
        if(fabs(v) > fabs(f)) {
            f = v;
        }
    }

    return f;
}


}

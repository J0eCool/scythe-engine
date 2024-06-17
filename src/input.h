#pragma once

#include "vec.h"

#include <string>

// this is cooler but more finicky; if we ever need to optimize Input then do it this way
// enum ButtonState {
//     btn_Released     = 0b00,
//     btn_Pressed      = 0b01,
//     btn_JustReleased = 0b10,
//     btn_JustPressed  = 0b11,
// };

struct ButtonState {
    bool lastPressed = false;
    bool pressed = false;
};

class Input {
public:
    virtual Vec2 getMousePos() const = 0;
    virtual ButtonState getButtonState(std::string name) const = 0;

    // returns true iff the Action was pressed this frame
    bool didPress(std::string name) {
        auto state = getButtonState(name);
        return state.pressed && !state.lastPressed;
    }
    // returns true when the Action is held down
    bool isHeld(std::string name) {
        auto state = getButtonState(name);
        return state.lastPressed || state.pressed;
    }
};

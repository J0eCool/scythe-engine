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
    // current position of the mouse, in window coordinates
    virtual Vec2 getMousePos() const = 0;
    // current state of a given button
    virtual ButtonState getButtonState(std::string name) const = 0;
    // simple text editing, supports appending and backspace
    virtual void editText(std::string &text) const = 0;

    // returns true iff the Action was pressed this frame
    bool didPress(std::string name) const {
        auto state = getButtonState(name);
        return state.pressed && !state.lastPressed;
    }
    // returns true when the Action is held down
    bool isHeld(std::string name) const {
        auto state = getButtonState(name);
        return state.lastPressed || state.pressed;
    }
    // returns true iff the Action was released this frame
    bool didRelease(std::string name) const {
        auto state = getButtonState(name);
        return !state.pressed && state.lastPressed;
    }

    float getAxis(std::string negName, std::string posName) const {
        return isHeld(posName) - isHeld(negName);
    }
};

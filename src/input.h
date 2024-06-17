#pragma once

#include "vec.h"

#include <map>

enum ButtonPressState {
    btn_Released     = 0b00,
    btn_Pressed      = 0b01,
    btn_JustReleased = 0b10,
    btn_JustPressed  = 0b11,
};

// InputActions 
struct InputAction {
    int keycode;
    enum Kind {
        Button,
        Axis,
    };
    Kind kind;
    union {
        ButtonPressState press;
        float amount;
    };

    InputAction() {
        // wait this is overcomplicated and terrible
    }
};

class Input {
    std::map<const char*, InputAction> actions;
public:
    float getAxis(const char* name) const;
    // 2D equivalent of getAxis
    Vec2 getGrid(const char* name) const;
};

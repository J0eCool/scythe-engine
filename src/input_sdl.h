#pragma once

#include "common.h"
#include "vec.h"

#include <map>
#include <string>

#include <SDL2/SDL.h>

struct ButtonState {
    bool lastPressed = false;
    bool pressed = false;
};

class Input {
    Vec2 _mousePos;

    // button name -> state mapping for Actions
    std::map<std::string, ButtonState> _buttons;

    enum InputKind {
        Key,
        MouseButton,
        ControllerButton,
        ControllerAxis,
    };
    struct Axis {
        float value;
        InputKind lastSet;
    };
    // axis name -> value per input axis
    std::map<std::string, Axis> _axes;

    // keycode -> button name
    std::map<SDL_Keycode, std::string> _keybinds;
    // mouse button -> button name
    std::map<Uint8, std::string> _mousebinds;
    // controller button -> button name
    std::map<Uint8, std::string> _ctrlbinds;

    struct PairAxis {
        std::string neg, pos;
    };
    // axis name -> pair of inputs (keys or controllers)
    std::map<std::string, PairAxis> _pairaxes;
    // controller axis -> axis name
    std::map<Uint8, std::string> _ctrlaxes;


    int _backspaces;
    std::string _appendText;
    bool _usingController = false;
    SDL_GameController *_controller = nullptr;

public:
    // call once per frame
    void update();

    // clear button mappings
    void resetBindings();
    void addKeybind(std::string name, SDL_Keycode key);
    void addMouseBind(std::string name, int mouseButton);
    void addControllerBind(std::string name, int ctrlButton);

    Vec2 getMousePos() const;

    ButtonState getButtonState(std::string name) const;

    void editText(std::string &text) const;

    // returns true iff the Action was pressed this frame
    bool didPress(std::string name) const;
    // returns true when the Action is held down
    bool isHeld(std::string name) const;
    // returns true iff the Action was released this frame
    bool didRelease(std::string name) const;

    void addAxisPair(std::string axis, std::string neg, std::string pos);
    void addControllerAxis(std::string name, Uint8 axis);
    float getAxis(std::string name) const;
};

#pragma once

#include "input.h"

#include <map>
#include <string>

#include <SDL2/SDL.h>

using namespace std::literals;

class Input_SDL : public Input {
    Vec2 _mousePos;

    // name -> state mapping for Actions
    std::map<std::string, ButtonState> buttons;
    // keycode -> Action name
    std::map<SDL_Keycode, std::string> keybinds;
public:
    Input_SDL() {
    }

    // call once per frame
    void update() {
        // clear just-pressed bit, so JustPressed -> Pressed and JustReleased -> Released
        for (auto& kv : buttons) {
            kv.second.lastPressed = kv.second.pressed;
        }

        // poll for new events and update button mappings
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                buttons["quit"].pressed = true;
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto it = keybinds.find(event.key.keysym.sym);
                if (it == keybinds.end()) {
                    continue;
                }
                bool press = event.type == SDL_KEYDOWN;
                if (isHeld(it->second) != press) {
                    buttons[it->second].pressed = press;
                }
            }
            }
        }
    }

    void addKeybind(std::string name, SDL_Keycode key) {
        keybinds[key] = name;
        if (buttons.find(name) == buttons.end()) {
            buttons[name] = {false, false};
        }
    }

    Vec2 getMousePos() const override {
        return _mousePos;
    }

    ButtonState getButtonState(std::string name) const override {
        assert(buttons.find(name) != buttons.end(), "Unknown button name: \"%s\"\n", name.c_str());
        return buttons.at(name);
    }
};

#pragma once

#include "common.h"
#include "input.h"

#include <map>
#include <string>

#include <SDL2/SDL.h>

using namespace std::literals;

class Input_SDL : public Input {
    Vec2 _mousePos;

    // name -> state mapping for Actions
    std::map<std::string, ButtonState> _buttons;
    // keycode -> Action name
    std::map<SDL_Keycode, std::string> _keybinds;

    int _backspaces;
    std::string _appendText;

public:
    Input_SDL() {
    }

    // call once per frame
    void update() {
        // clear just-pressed bit, so JustPressed -> Pressed and JustReleased -> Released
        for (auto& kv : _buttons) {
            kv.second.lastPressed = kv.second.pressed;
        }

        _backspaces = 0;
        _appendText = "";

        // poll for new events and update button mappings
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    _buttons["quit"].pressed = true;
                }
                break;
            }
            case SDL_QUIT: {
                _buttons["quit"].pressed = true;
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto sym = event.key.keysym.sym;
                bool isPress = event.type == SDL_KEYDOWN;
                if (isPress) {
                    //  handle logic for typing; this kinda sucks ngl
                    bool isText = sym >= SDLK_a && sym <= SDLK_z
                        || sym == SDLK_SPACE;
                    if (isText) {
                        _appendText += sym;
                    } else if (sym == SDLK_BACKSPACE) {
                        _backspaces++;
                    }
                }
                auto it = _keybinds.find(event.key.keysym.sym);
                if (it == _keybinds.end()) {
                    continue;
                }
                _buttons[it->second].pressed = isPress;
                break;
            }
            case SDL_MOUSEMOTION:
                _mousePos = Vec2i { event.motion.x, event.motion.y }.to<float>();
            }
        }
    }

    void addKeybind(std::string name, SDL_Keycode key) {
        _keybinds[key] = name;
        if (_buttons.find(name) == _buttons.end()) {
            _buttons[name] = {false, false};
        }
    }

    Vec2 getMousePos() const override {
        return _mousePos;
    }

    ButtonState getButtonState(std::string name) const override {
        assert(_buttons.find(name) != _buttons.end(), "Unknown button name: \"%s\"\n", name.c_str());
        return _buttons.at(name);
    }

    void editText(std::string &text) const override {
        for (int i = 0; i < min<int>(text.size(), _backspaces); ++i) {
            text.pop_back();
        }
        text += _appendText;
    }
};

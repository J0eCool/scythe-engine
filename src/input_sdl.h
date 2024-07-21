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
    // mouse button -> Action name
    std::map<SDL_Keycode, std::string> _mousebinds;

    int _backspaces;
    std::string _appendText;

public:
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
                bool isPress = event.type == SDL_KEYDOWN;
                auto sym = event.key.keysym.sym;
                if (isPress) {
                    //  handle logic for typing; this kinda sucks ngl
                    /// TODO: investigate SDL_TEXTEDITING events
                    bool isText = sym >= SDLK_a && sym <= SDLK_z
                        || sym == SDLK_SPACE;
                    if (isText) {
                        _appendText += sym;
                    } else if (sym == SDLK_BACKSPACE) {
                        _backspaces++;
                    }
                }
                auto it = _keybinds.find(sym);
                if (it != _keybinds.end()) {
                    _buttons[it->second].pressed = isPress;
                }
                break;
            }
            case SDL_MOUSEMOTION:
                _mousePos = Vec2i { event.motion.x, event.motion.y }.to<float>();
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                bool isPress = event.type == SDL_MOUSEBUTTONDOWN;
                auto btn = event.button.button;
                auto it = _mousebinds.find(btn);
                if (it != _mousebinds.end()) {
                    _buttons[it->second].pressed = isPress;
                }
                break;
            }
            }
        }
    }

    void addKeybind(std::string name, SDL_Keycode key) {
        _keybinds[key] = name;
        if (_buttons.find(name) == _buttons.end()) {
            _buttons[name] = {false, false};
        }
    }
    void addMouseBind(std::string name, int mouseButton) {
        _mousebinds[mouseButton] = name;
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

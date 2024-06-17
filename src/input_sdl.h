#pragma once

#include "input.h"

#include <map>
#include <string>

#include <SDL2/SDL.h>

using namespace std::literals;

class Input_SDL : public Input {
    Vec2 _mousePos;

    // name -> state mapping
    std::map<std::string, ButtonState> buttons;
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
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    buttons["quit"].pressed = true;
                    break;
                case SDLK_r:
                    if (!isHeld("reload")) {
                        buttons["reload"].pressed = true;
                    }
                    break;
                case SDLK_l:
                    buttons["logging"].pressed = true;
                    break;
                }
                break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_q:
                    if (isHeld("quit")) {
                        buttons["quit"].pressed = false;
                    }
                    break;
                case SDLK_r:
                    if (isHeld("reload")) {
                        buttons["reload"].pressed = false;
                    }
                    break;
                case SDLK_l:
                    if (isHeld("logging")) {
                        buttons["logging"].pressed = false;
                    }
                    break;
                }
            }
        }
    }

    Vec2 getMousePos() const override {
        return _mousePos;
    }

    ButtonState getButtonState(std::string name) override {
        // assert(buttons.find(name) != buttons.end(), "Unknown button name: \"%s\"\n", name.c_str());
        return buttons[name];
    }
};

#include "input_sdl.h"

using namespace std::literals;

// call once per frame
void Input::update() {
    Tracer trace("Input::update");
    // clear just-pressed bit, so JustPressed -> Pressed and JustReleased -> Released
    for (auto& kv : _buttons) {
        kv.second.lastPressed = kv.second.pressed;
    }
    trace("updated %d buttons", _buttons.size());

    _backspaces = 0;
    _appendText = "";

    // poll for new events and update button mappings
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        trace("event type=%d", event.type);
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
            _usingController = false;
            break;
        }
        case SDL_MOUSEMOTION: {
            _mousePos = Vec2i { event.motion.x, event.motion.y }.to<float>();
            break;
        }
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
        case SDL_CONTROLLERAXISMOTION: {
            auto it = _ctrlaxes.find(event.caxis.axis);
            if (it != _ctrlaxes.end()) {
                float v = (float)event.caxis.value / __INT16_MAX__;
                const float deadzone = 0.1f;
                if (abs(v) < deadzone) {
                    v = 0;
                }
                _axes[it->second].value = v;
            }
            _usingController = true;
            break;
        }
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            bool isPress = event.type == SDL_CONTROLLERBUTTONDOWN;
            auto btn = event.cbutton.button;
            auto it = _ctrlbinds.find(btn);
            if (it != _ctrlbinds.end()) {
                _buttons[it->second].pressed = isPress;
            }
            _usingController = false;
            break;
        }
        }
    }

    if (!_usingController) {
        for (auto &axis: _axes) {
            auto pair = _pairaxes[axis.first];
            float v = isHeld(pair.pos) - isHeld(pair.neg);
            axis.second.value = v;
        }
    }
}

// clear button mappings
void Input::resetBindings() {
    _buttons.clear();
    _keybinds.clear();
    _mousebinds.clear();
    _ctrlbinds.clear();

    // reset previous controller state if any
    if (_controller) {
        SDL_GameControllerClose(_controller);
        _controller = nullptr;
    }
    if (SDL_NumJoysticks() > 0) {
        _controller = SDL_GameControllerOpen(0);
    }
}

void Input::addKeybind(std::string name, SDL_Keycode key) {
    Tracer trace((std::string("Input::addKeybind-")+name).c_str());
    _keybinds[key] = name;
    if (_buttons.find(name) == _buttons.end()) {
        trace("adding to _buttons");
        _buttons[name] = {false, false};
    }
}
void Input::addMouseBind(std::string name, int mouseButton) {
    Tracer trace((std::string("Input::addMouseBind-")+name).c_str());
    _mousebinds[mouseButton] = name;
    if (_buttons.find(name) == _buttons.end()) {
        trace("adding to _buttons");
        _buttons[name] = {false, false};
    }
}
void Input::addControllerBind(std::string name, int ctrlButton) {
    Tracer trace((std::string("Input::addControllerBind-")+name).c_str());
    _ctrlbinds[ctrlButton] = name;
    if (_buttons.find(name) == _buttons.end()) {
        trace("adding to _buttons");
        _buttons[name] = {false, false};
    }
}

Vec2 Input::getMousePos() const {
    return _mousePos;
}

ButtonState Input::getButtonState(std::string name) const {
    Tracer((std::string("Input::getButtonState-")+name).c_str());
    if (_buttons.find(name) == _buttons.end()) {
        log_once("Unknown button name: \"%s\"", name.c_str());
        return { false, false };
    }
    return _buttons.at(name);
}

void Input::editText(std::string &text) const {
    for (int i = 0; i < min<int>(text.size(), _backspaces); ++i) {
        text.pop_back();
    }
    text += _appendText;
}

// returns true iff the Action was pressed this frame
bool Input::didPress(std::string name) const {
    auto state = getButtonState(name);
    return state.pressed && !state.lastPressed;
}
// returns true when the Action is held down
bool Input::isHeld(std::string name) const {
    auto state = getButtonState(name);
    return state.lastPressed || state.pressed;
}
// returns true iff the Action was released this frame
bool Input::didRelease(std::string name) const {
    auto state = getButtonState(name);
    return !state.pressed && state.lastPressed;
}

void Input::addAxisPair(std::string axis, std::string neg, std::string pos) {
    assert(_buttons.find(neg) != _buttons.end(),
        "Unknown button: \"%s\"", neg.c_str());
    assert(_buttons.find(pos) != _buttons.end(),
        "Unknown button: \"%s\"", pos.c_str());
    _pairaxes[axis] = { neg, pos };
    if (_axes.find(axis) == _axes.end()) {
        _axes[axis] = { 0, Key };
    }
}
void Input::addControllerAxis(std::string name, Uint8 axis) {
    _ctrlaxes[axis] = name;
    if (_axes.find(name) == _axes.end()) {
        _axes[name] = { 0, Key };
    }
}
float Input::getAxis(std::string name) const {
    assert(_axes.find(name) != _axes.end(),
        "Unknown axis name: \"%s\"", name.c_str());
    return _axes.at(name).value;
}

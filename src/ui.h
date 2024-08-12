#pragma once

#include "input_sdl.h"
#include "render_sdl.h"
#include "vec.h"

#include <cstring>
#include <vector>

struct Button {
    const char* label = "";
    bool isHovered = false;
    bool isPressed = false;
    Vec2 pos { 0, 0 };
    Vec2 size { 0, 0 };

    void render(Renderer* renderer) {
        if (isPressed && isHovered) {
            renderer->setColor(0.25, 0.25, 0.25, 1.0);
        } else if (isHovered) {
            renderer->setColor(0.75, 0.75, 0.75, 1.0);
        } else {
            renderer->setColor(0.5, 0.5, 0.5, 1.0);
        }
        renderer->drawRect(pos, size);
        renderer->drawText(label, pos + Vec2{5});
    }
};

struct Label {
    static const size_t maxLen = 31;
    char buffer[maxLen+1];
    Vec2 pos { 0, 0 };
    Vec2 size { 0, 0 }; // bounding box

    void render(Renderer* renderer) {
        renderer->drawText(buffer, pos);
    }
};

enum UIKind {
    uiButton,
    uiLabel,
};

struct UIElement {
    UIKind kind;
    union {
        Button button;
        Label label;
    };

    UIElement(Button _button) : kind(uiButton), button(_button) {}
    UIElement(Label _label) : kind(uiLabel), label(_label) {}

    void render(Renderer* renderer) {
        switch (kind) {
        case uiButton:
            button.render(renderer);
            break;
        case uiLabel:
            label.render(renderer);
            break;
        }
    }

    void debugPrint() const {
        // indented by two because we always call this from a UI
        switch (kind) {
        case uiButton:
            log("  button label=\"%s\" pos=<%f, %f> size=<%f, %f>",
                button.label,
                button.pos.x, button.pos.y,
                button.size.x, button.size.y);
            break;
        case uiLabel:
            log("  label text=\"%s\" pos=<%f, %f> size=<%f, %f>",
                label.buffer,
                label.pos.x, label.pos.y,
                label.size.x, label.size.y);
            break;
        default:
            log("  [UNKNOWN]");
            break;
        }
    }
};

class UI {
    Allocator* _allocator;
    Input* _input;
    Vec2 _padding { 10 }; // space between elements

    Vec2 _origin; // starting point of cursor
    Vec2 _cursor; // current position to place an element
    float _lineHeight; // max height of elements on current line

    std::vector<UIElement> _elements;
    // index of the next element to use; reset to 0 each frame
    int _elemIdx;
public:
    UI(Allocator *allocator, Input* input)
        : _allocator(allocator), _input(input) {}

    void unload() {
        _elements.clear();
    }

    void startUpdate(Vec2 pos) {
        _cursor = _origin = pos;
        _elemIdx = 0;
        _lineHeight = 0;
    }

    void render(Renderer* renderer) {
        // only iterate up to _elemIdx, because we may have fewer elems visible
        // this frame than have ever been created
        for (int i = 0; i < _elemIdx; ++i) {
            _elements[i].render(renderer);
        }
    }

    /// @brief creates a button at the cursor
    /// @param label text to display on the button
    /// @return true when clicked on
    bool button(const char* label) {
        UIElement &elem = nextElem();
        if (elem.kind != uiButton) {
            elem.kind = uiButton;
            elem.button = Button();
        }
        Button &button = elem.button;
        button.label = label;
        button.pos = _cursor;
        button.size = Renderer::fontSize * Vec2{(float)strlen(label), 1}
            + _padding;

        _cursor.x += button.size.x + _padding.x;
        _lineHeight = max(_lineHeight, button.size.y);

        Vec2 mouse = _input->getMousePos();
        button.isHovered = in_rect(mouse, button);
        if (!button.isPressed) {
            button.isPressed = button.isHovered && _input->didPress("click");
            return false;
        } else if (_input->didRelease("click")) {
            button.isPressed = false;
            // only count clicks if we release the click over this button
            return button.isHovered;
        } else {
            return false;
        }
    }

    void label(const char* text) {
        UIElement &elem = nextElem();
        if (elem.kind != uiLabel) {
            elem.kind = uiLabel;
            elem.label = Label();
        }
        Label &label = elem.label;
        strncpy(label.buffer, text, Label::maxLen);
        int len = min(strlen(text), Label::maxLen);
        // Label::maxLen is 1 less than the buffer size, so we always have room
        // for the terminal null byte
        label.buffer[len] = '\0';
        label.pos = _cursor;
        label.size = Renderer::fontSize * Vec2{(float)len, 1};

        _cursor.x += label.size.x + _padding.x;
        _lineHeight = max(_lineHeight, label.size.y);
    }
    void label(int num) {
        char buffer[16];
        sprintf(buffer, "%d", num);
        label(buffer);
    }
    void label(float num) {
        char buffer[32];
        sprintf(buffer, "%f", num);
        label(buffer);
    }
    template <typename T, typename ...Ts>
    void labels(T arg, Ts ...args) {
        label(arg);
        labels(args...);
    }
    void labels() {}

    void line() {
        _cursor.y += _lineHeight + _padding.y;
        _cursor.x = _origin.x;
        _lineHeight = 0;
    }

    void debugPrint() const {
        log("ui elems: %d", _elements.size());
        for (auto &elem : _elements) {
            elem.debugPrint();
        }
    }

private:
    /// @brief gets the next UIElement to update, grows the `_elements` array
    /// by one if needed
    UIElement& nextElem() {
        if (_elemIdx >= _elements.size()) {
            // arbitrarily default to a label; could templatize this function later
            _elements.emplace_back(Label());
        }
        // if the elem index is ever off by more than one, we've done something
        // very wrong, and better to crash than allocate a billion slots
        assert(_elemIdx < _elements.size(), "error managing _elemIdx");
        return _elements[_elemIdx++];
    }
};

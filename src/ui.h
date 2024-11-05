#pragma once

#include "input_sdl.h"
#include "render_sdl.h"
#include "vec.h"

#include <vector>

struct UIButton {
    const char* label = "";
    bool isHovered = false;
    bool isPressed = false;
    Vec2 pos { 0, 0 };
    Vec2 size { 0, 0 };

    void render(Renderer* renderer);
};

struct UILabel {
    static const size_t maxLen = 31;
    char buffer[maxLen+1];
    Vec2 pos { 0, 0 };
    Vec2 size { 0, 0 }; // bounding box

    void render(Renderer* renderer);
};

struct UISlider {
    float pct;
    Vec2 pos;
    Vec2 size;
    bool isHovered, isPressed;

    void render(Renderer* renderer);

    /// @brief Get the rect that corresponds with the tab of the slider
    Rect tabRect() const;
};

template <typename T>
T sliderLerp(float t, T lo, T hi) {
    // rewrite lerp so we can round to nearest integer rather than floor
    return round((1-t)*lo + t*hi);
}
// template <>
// float sliderLerp<float>(float t, float lo, float hi);

struct UIRect {
    Color color;
    Vec2 pos;
    Vec2 size;

    void render(Renderer* renderer);
};

enum UIKind {
    uiButton,
    uiLabel,
    uiSlider,
    uiRect,
};

struct UIElement {
    UIKind kind;
    union {
        UIButton button;
        UILabel label;
        UISlider slider;
        UIRect rect;
    };

    UIElement(UIButton _button) : kind(uiButton), button(_button) {}
    UIElement(UILabel _label) : kind(uiLabel), label(_label) {}

    void render(Renderer* renderer);

    void debugPrint() const;
};

class UI {
    ///FIXME: we don't actually use or need this
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

    void unload();
    void startUpdate(Vec2 pos = {0, 0});
    void region(Vec2 pos);
    void render(Renderer* renderer);

    /// @brief creates a button at the cursor
    /// @param label text to display on the button
    /// @return true when clicked on
    bool button(const char* label);

    void label(const char* text);
    void label(int num);
    void label(float num);
    void label(Vec2 v);
    template <typename T, typename ...Ts>
    void labels(T arg, Ts ...args) {
        label(arg);
        labels(args...);
    }
    void labels() {} // base case for template recursion; nop

    /// @brief A slider the player can click and drag on to change a value
    /// @param val the value to change
    /// @param lo lower bound of the value
    /// @param hi upper bound of the value
    template <typename T>
    void slider(T &val, T lo, T hi) {
        UIElement &elem = nextElem();
        if (elem.kind != uiSlider) {
            elem.kind = uiSlider;
            elem.slider = UISlider();
        }
        UISlider &slider = elem.slider;
        slider.pos = _cursor;
        slider.size = _padding + Vec2{200, 10};

        _cursor.x += slider.size.x + _padding.x;
        _lineHeight = max(_lineHeight, slider.size.y);

        Vec2 mouse = _input->getMousePos();
        slider.isHovered = in_rect(mouse, slider.pos,
            // add the tab size to the acceptable slider bounds
            /// TODO: more accurate input handling means revisit this
            slider.size+Vec2{slider.size.y,0});
        // handle click
        if (!slider.isPressed) {
            slider.isPressed = slider.isHovered && _input->didPress("click");
        } else if (_input->didRelease("click")) {
            slider.isPressed = false;
        }
        // handle drag
        if (slider.isPressed) {
            float t = clamp((mouse.x-slider.pos.x) / slider.size.x);
            slider.pct = t;
            val = sliderLerp<T>(t, lo, hi);
        }

        // always update pct to match current val
        slider.pct = clamp(float(val-lo) / float(hi-lo));
    }

    void rect(Color color, Vec2 size);


    /// @brief Linebreak; moves cursor down to new line, resetting x position
    void line();

    /// @brief Aligns elements horizontally
    /// @param x Set the cursor's X position to this many pixels from origin
    void align(float x);

    void debugPrint() const;

private:
    /// @brief gets the next UIElement to update, grows the `_elements` array
    /// by one if needed
    UIElement& nextElem();
};

#pragma once

#include "ui.h"

template <typename T>
bool uiParamMult(UI &ui, const char* text, T &val, T mult, T lo, T hi) {
    return uiParam(ui, text, val, val/mult, val*mult, lo, hi);
}
template <typename T>
bool uiParam(UI &ui, const char* text, T &val, T delta, T lo, T hi) {
    return uiParam(ui, text, val, val-delta, val+delta, lo, hi);
}
template <typename T>
bool uiParam(UI &ui, const char* text, T &val, T dec, T inc, T lo, T hi) {
    // right-align the labels :O
    ui.align(240-Renderer::fontSize.x*(strlen(text)+2));
    ui.labels(text, ":");
    T set = val;
    if (ui.button("<")) {
        set = max(dec, lo);
    }
    ui.label(val);
    if (ui.button(">")) {
        set = min(inc, hi);
    }
    ui.align(400);
    ui.slider(set, lo, hi);        
    ui.line();
    if (set != val) {
        val = set;
        return true;
    }
    return false;
}

/// @brief UI Widget that toggles a boolean variable
/// @param option the option to display and update
/// @param ifOn text to show when `option` is `true`
/// @param ifOff text when `option` is `false`
/// @return `true` when clicked (for callback purposes)
bool uiToggle(UI &ui, bool &option, const char* ifOn, const char* ifOff) {
    if (ui.button(option ? ifOn : ifOff)) {
        option = !option;
        return true;
    }
    return false;
}

/// @brief bootleg color picker
/// @param color color to edit
/// @return `true` when modified
bool uiColor(UI &ui, Color &color) {
    bool changed = false;
    ui.align(160);
    ui.rect(color, Vec2{32});
    const int delta = 5;
    changed |= uiParam(ui, "R", color.r,
        Uint8(color.r-delta), Uint8(color.r+delta),
        Uint8(0), Uint8(255));
    changed |= uiParam(ui, "G", color.g,
        Uint8(color.g-delta), Uint8(color.g+delta),
        Uint8(0), Uint8(255));
    changed |= uiParam(ui, "B", color.b,
        Uint8(color.b-delta), Uint8(color.b+delta),
        Uint8(0), Uint8(255));
    return changed;
}

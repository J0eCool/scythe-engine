#include "uilib.h"

bool uiToggle(UI &ui, bool &option, const char* ifOn, const char* ifOff) {
    if (ui.button(option ? ifOn : ifOff)) {
        option = !option;
        return true;
    }
    return false;
}

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

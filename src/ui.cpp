#include "ui.h"

#include <cstring>

void UIButton::render(Renderer* renderer) {
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
// static const size_t UILabel::maxLen = 31;
void UILabel::render(Renderer* renderer) {
    renderer->drawText(buffer, pos);
}

void UISlider::render(Renderer* renderer) {
    Rect tab = tabRect();
    float barH = 6;
    renderer->setColor(0.3, 0.3, 0.3);
    renderer->drawBox(pos + Vec2{tab.size.x, size.y-barH}/2, Vec2{size.x, barH});

    if (isPressed) {
        // set to pressed color regardless of if it's hovered, because
        // click+drag on a slider updates when the mouse leaves the bounds
        renderer->setColor(0.25, 0.25, 0.25, 1.0);
    } else if (isHovered) {
        renderer->setColor(0.75, 0.75, 0.75, 1.0);
    } else {
        renderer->setColor(0.5, 0.5, 0.5, 1.0);
    }
    renderer->drawRect(tab);
}

Rect UISlider::tabRect() const {
    return Rect {
        pos + pct*Vec2{size.x, 0},
        {size.y, size.y}
    };
}

void UIRect::render(Renderer* renderer) {
    renderer->setColor(color);
    renderer->drawRect(pos, size);
}

void UIElement::render(Renderer* renderer) {
    switch (kind) {
    case uiButton:
        button.render(renderer);
        break;
    case uiLabel:
        label.render(renderer);
        break;
    case uiSlider:
        slider.render(renderer);
        break;
    case uiRect:
        rect.render(renderer);
        break;
    }
}

void UIElement::debugPrint() const {
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
    case uiSlider:
        log("  slider val=\"%f\" pos=<%f, %f> size=<%f, %f>",
            slider.pct,
            slider.pos.x, slider.pos.y,
            slider.size.x, slider.size.y);
        break;
    default:
        log("  [UNKNOWN]");
        break;
    }
}

void UI::unload() {
    _elements.clear();
}

void UI::startUpdate(Vec2 pos) {
    _cursor = _origin = pos;
    _elemIdx = 0;
    _lineHeight = 0;
}

void UI::region(Vec2 pos) {
    _cursor = _origin = pos;
}

void UI::render(Renderer* renderer) {
    // only iterate up to _elemIdx, because we may have fewer elems visible
    // this frame than have ever been created
    for (int i = 0; i < _elemIdx; ++i) {
        _elements[i].render(renderer);
    }

    // draw a line to closest interactable UI element to the mouse
    Vec2 mouse = _input->getMousePos();
    Rect closest;
    for (auto &elem : _elements) {
        // Maybe<Rect> getInteractRect (or std::vector<Rect> for multi-thing widgets)
        // could using Maybe = std::optional
        Rect rect;
        if (elem.kind == uiButton) {
            rect = { elem.button.pos, elem.button.size };
        } else if (elem.kind == uiSlider) {
            rect = elem.slider.tabRect();
        } else {
            continue; // return {}
        }

        // pick the closer one
        if ((rect.pos-mouse).len2() < (closest.pos-mouse).len2()) {
            closest = rect;
        }
    }

    // todo: snap to rect border
    renderer->setColor(Color::red);
    renderer->drawLine(mouse, closest.pos + closest.size/2);
}

bool UI::button(const char* label) {
    UIElement &elem = nextElem();
    if (elem.kind != uiButton) {
        elem.kind = uiButton;
        elem.button = UIButton();
    }
    UIButton &button = elem.button;
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

void UI::label(const char* text) {
    if (strcmp(text, "\n") == 0) {
        line();
        return;
    }
    UIElement &elem = nextElem();
    if (elem.kind != uiLabel) {
        elem.kind = uiLabel;
        elem.label = UILabel();
    }
    UILabel &label = elem.label;
    strncpy(label.buffer, text, UILabel::maxLen);
    int len = min(strlen(text), UILabel::maxLen);
    // UILabel::maxLen is 1 less than the buffer size, so we always have room
    // for the terminal null byte
    label.buffer[len] = '\0';
    label.pos = _cursor;
    label.size = Renderer::fontSize * Vec2{(float)len, 1};

    _cursor.x += label.size.x + _padding.x;
    _lineHeight = max(_lineHeight, label.size.y);
}
void UI::label(int num) {
    char buffer[16];
    sprintf(buffer, "%d", num);
    label(buffer);
}
void UI::label(float num) {
    char buffer[32];
    sprintf(buffer, "%.2f", num);
    label(buffer);
}
void UI::label(Vec2 v) {
    char buffer[64];
    sprintf(buffer, "<%.2f, %.2f>", v.x, v.y);
    label(buffer);
}

void UI::rect(Color color, Vec2 size) {
    UIElement &elem = nextElem();
    if (elem.kind != uiRect) {
        elem.kind = uiRect;
        elem.rect = UIRect();
    }
    UIRect &rect = elem.rect;
    rect.pos = _cursor;
    rect.size = size;
    rect.color = color;

    _cursor.x += rect.size.x + _padding.x;
    _lineHeight = max(_lineHeight, rect.size.y);
}


void UI::line() {
    _cursor.y += _lineHeight + _padding.y;
    _cursor.x = _origin.x;
    _lineHeight = 0;
}

void UI::align(float x) {
    _cursor.x = _origin.x + x;
}

void UI::debugPrint() const {
    log("ui elems: %d", _elements.size());
    for (auto &elem : _elements) {
        elem.debugPrint();
    }
}

UIElement& UI::nextElem() {
    if (_elemIdx >= _elements.size()) {
        // arbitrarily default to a label; could templatize this function later
        _elements.emplace_back(UILabel());
    }
    // if the elem index is ever off by more than one, we've done something
    // very wrong, and better to crash than allocate a billion slots
    assert(_elemIdx < _elements.size(), "error managing _elemIdx");
    return _elements[_elemIdx++];
}

// render.h
#pragma once

class Renderer {
    // this is a tedious amount of repetition and boilerplate
    // we could absolutely use some codegen here later on
protected:
    void (*_setColor)(Renderer* self, float r, float g, float b, float a);
    void (*_drawRect)(Renderer* self, float x, float y, float w, float h);
    void (*_drawBox)(Renderer* self, float x, float y, float w, float h);

public:
    void setColor(float r, float g, float b, float a) {
        _setColor(this, r, g, b, a);
    }
    void drawRect(float x, float y, float w, float h) {
        _drawRect(this, x, y, w, h);
    }
    void drawBox(float x, float y, float w, float h) {
        _drawBox(this, x, y, w, h);
    }
};

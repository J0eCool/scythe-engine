// render.h
#pragma once

// Abstract base class for rendering backends to implement
class Renderer {
public:
    virtual ~Renderer() {}
    virtual void setColor(float r, float g, float b, float a) = 0;
    virtual void drawRect(float x, float y, float w, float h) = 0;
    virtual void drawBox(float x, float y, float w, float h) = 0;
    virtual void drawText(const char* text, float x, float y) = 0;
};

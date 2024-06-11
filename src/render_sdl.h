#pragma once

#include "render.h"

#include <SDL2/SDL.h>

class Renderer_SDL : public Renderer {
    SDL_Renderer* _sdlRenderer;

public:
    Renderer_SDL(SDL_Renderer* sdl) : _sdlRenderer(sdl) {}

    void setColor(float r, float g, float b, float a) override {
        SDL_SetRenderDrawColor(_sdlRenderer, 255*r, 255*g, 255*b, 255*a);
    }
    void drawRect(float x, float y, float w, float h) override {
        SDL_Rect rect {(int)x,(int)y,(int)w,(int)h};
        SDL_RenderFillRect(_sdlRenderer, &rect);
    }
    void drawBox(float x, float y, float w, float h) override {
        SDL_Rect rect {(int)x, (int)y, (int)w, (int)h};
        SDL_RenderDrawRect(_sdlRenderer, &rect);
    }
};

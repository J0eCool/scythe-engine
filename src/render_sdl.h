#pragma once

#include "render.h"

#include <SDL2/SDL.h>

class Renderer_SDL : public Renderer {
    SDL_Renderer* _sdlRenderer;
public:
    Renderer_SDL(SDL_Renderer* sdl) : _sdlRenderer(sdl) {
        _setColor = Renderer_SDL::setColor_impl;
        _drawRect = Renderer_SDL::drawRect_impl;
        _drawBox = Renderer_SDL::drawBox_impl;
    }

private:
    static void setColor_impl(Renderer* _self, float r, float g, float b, float a) {
        Renderer_SDL* self = (Renderer_SDL*)_self;
        SDL_SetRenderDrawColor(self->_sdlRenderer, 255*r, 255*g, 255*b, 255*a);
    }
    static void drawRect_impl(Renderer* _self, float x, float y, float w, float h) {
        Renderer_SDL* self = (Renderer_SDL*)_self;
        SDL_Rect rect {(int)x,(int)y,(int)w,(int)h};
        SDL_RenderFillRect(self->_sdlRenderer, &rect);
    }
    static void drawBox_impl(Renderer* _self, float x, float y, float w, float h) {
        Renderer_SDL* self = (Renderer_SDL*)_self;
        SDL_Rect rect {(int)x, (int)y, (int)w, (int)h};
        SDL_RenderDrawRect(self->_sdlRenderer, &rect);
    }
};

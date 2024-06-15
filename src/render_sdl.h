#pragma once

#include "render.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Renderer_SDL : public Renderer {
    SDL_Renderer* _sdlRenderer;
    SDL_Texture *_alphabetTexture;

public:
    Renderer_SDL(SDL_Renderer* sdl) : _sdlRenderer(sdl) {
        SDL_Surface *surf = IMG_Load("../data/alpha.png");
        assert(surf, "alpha.png failed to load");
        _alphabetTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surf);
        SDL_FreeSurface(surf);
    }

    ~Renderer_SDL() override {
        SDL_DestroyTexture(_alphabetTexture);
    }

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

    void drawText(const char* text, float x, float y) override {
        SDL_Rect srcRect {0, 0, 12, 28};
        SDL_Rect dstRect {(int)x, (int)y, 12, 28};
        SDL_RenderCopy(_sdlRenderer, _alphabetTexture, &srcRect, &dstRect);
    }
};

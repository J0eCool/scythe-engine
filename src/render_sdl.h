#pragma once

#include "common.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Renderer {
    SDL_Renderer* _sdlRenderer;
    SDL_Texture *_alphabetTexture;

public:
    Renderer(SDL_Renderer* sdl) : _sdlRenderer(sdl) {
        SDL_Surface *surf = IMG_Load("../data/alpha.png");
        assert(surf, "alpha.png failed to load");
        _alphabetTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surf);
        SDL_FreeSurface(surf);
    }

    ~Renderer() {
        SDL_DestroyTexture(_alphabetTexture);
    }

    void setColor(float r, float g, float b, float a) {
        SDL_SetRenderDrawColor(_sdlRenderer, 255*r, 255*g, 255*b, 255*a);
    }
    void drawRect(float x, float y, float w, float h) {
        SDL_Rect rect {(int)x,(int)y,(int)w,(int)h};
        SDL_RenderFillRect(_sdlRenderer, &rect);
    }
    void drawRect(Vec2 pos, Vec2 size) {
        drawRect(pos.x, pos.y, size.x, size.y);
    }
    void drawBox(float x, float y, float w, float h) {
        SDL_Rect rect {(int)x, (int)y, (int)w, (int)h};
        SDL_RenderDrawRect(_sdlRenderer, &rect);
    }

    void drawText(const char* text, float x, float y) {
        int i = 0;
        int w = 12;
        int h = 28;
        while (char c = text[i++]) {
            if (c >= 'a' && c <= 'z') {
                // toupper
                c -= 'a' - 'A';
            }
            int j, k;
            if (c >= 'A' && c <= 'Z') {
                j = (c - 'A') % 13;
                k = (c - 'A') / 13;
            } else if (c >= '0' && c <= '9') {
                j = c - '0';
                k = 2;
            } else {
                continue;
            }
            SDL_Rect srcRect {j*w, k*h, w, h};
            SDL_Rect dstRect {(int)x + (i-1)*w, (int)y, w, h};
            SDL_RenderCopy(_sdlRenderer, _alphabetTexture, &srcRect, &dstRect);
        }
    }

    SDL_Renderer* sdl() const {
        return _sdlRenderer;
    }
};

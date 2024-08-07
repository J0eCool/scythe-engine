#pragma once

#include "common.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

struct Color {
    Uint8 r, g, b, a;

    Color operator+(Color c) const {
        return {Uint8(r+c.r), Uint8(g+c.g), Uint8(b+c.b), Uint8(a+c.a)};
    }
    Color operator*(float s) const {
        return {Uint8(r*s), Uint8(g*s), Uint8(b*s), Uint8(a*s)};
    }
};
Color operator*(float s, Color c) {
    return c*s;
}

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

    SDL_Renderer* sdl() const {
        return _sdlRenderer;
    }

    void setColor(float r, float g, float b, float a) {
        SDL_SetRenderDrawColor(_sdlRenderer, 255*r, 255*g, 255*b, 255*a);
    }
    void setColor(Color c) {
        SDL_SetRenderDrawColor(_sdlRenderer, c.r, c.g, c.b, c.a);
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

    static const Vec2 fontSize;
    void drawText(const char* text, Vec2 pos) {
        drawText(text, pos.x, pos.y);
    }
    void drawText(const char* text, float x, float y) {
        int i = 0;
        int w = (int)fontSize.x;
        int h = (int)fontSize.y;
        while (char c = text[i++]) {
            if (c < 0) {
                // unsupported character; no extended ASCII tyvm
                continue;
            }
            if (c >= 'a' && c <= 'z') {
                // uppercase everything cause we have no lowercase yet
                c -= 'a' - 'A';
            }
            int j = c % 16;
            int k = c / 16;
            SDL_Rect srcRect {j*w, k*h, w, h};
            SDL_Rect dstRect {(int)x + (i-1)*w, (int)y, w, h};
            SDL_RenderCopy(_sdlRenderer, _alphabetTexture, &srcRect, &dstRect);
        }
    }

    void drawImage(SDL_Texture *texture, float x, float y, float w, float h) {
        SDL_Rect destRect { (int)x, (int)y, (int)w, (int)h };
        SDL_RenderCopy(_sdlRenderer, texture, nullptr, &destRect);
    }
    void drawImage(SDL_Texture *texture, Vec2 pos, Vec2 size) {
        drawImage(texture, pos.x, pos.y, size.x, size.y);
    }
};

const Vec2 Renderer::fontSize { 12, 28 };

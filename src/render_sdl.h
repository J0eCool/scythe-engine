#pragma once

#include "color.h"
#include "common.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SMOL_TEXT true
#if SMOL_TEXT
    #define FONT_FILE "../data/alpha_small.png"
    #define FONT_SIZE {8, 8}
#else
    #define FONT_FILE "../data/alpha.png"
    #define FONT_SIZE {21, 12}
#endif

class Renderer {
    SDL_Renderer* _sdlRenderer;
    SDL_Texture *_alphabetTexture;

    Color _bgColor;

public:
    Renderer(SDL_Renderer* sdl) : _sdlRenderer(sdl) {
        const char* filename = FONT_FILE;
        SDL_Surface *surf = IMG_Load(filename);
        assert(surf, "%s failed to load", filename);
        _alphabetTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surf);
        SDL_FreeSurface(surf);
    }

    ~Renderer() {
        SDL_DestroyTexture(_alphabetTexture);
    }

    SDL_Renderer* sdl() const {
        return _sdlRenderer;
    }

    void startFrame() {
        setColor(_bgColor);
        SDL_RenderClear(_sdlRenderer);
    }
    void endFrame() {
        SDL_RenderPresent(_sdlRenderer);
    }

    void background(Color c) {
        _bgColor = c;
    }

    void setColor(float r, float g, float b, float a = 1.0f) {
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
    void drawRect(Rect rect) {
        drawRect(rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
    }
    void drawBox(float x, float y, float w, float h) {
        SDL_Rect rect {(int)x, (int)y, (int)w, (int)h};
        SDL_RenderDrawRect(_sdlRenderer, &rect);
    }
    void drawBox(Vec2 pos, Vec2 size) {
        drawBox(pos.x, pos.y, size.x, size.y);
    }
    void drawBox(Rect rect) {
        drawBox(rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
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
const Vec2 Renderer::fontSize = FONT_SIZE;

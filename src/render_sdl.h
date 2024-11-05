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
    Renderer(SDL_Renderer* sdl);

    ~Renderer();

    SDL_Renderer* sdl() const;

    void startFrame();
    void endFrame();

    void background(Color c);

    void setColor(float r, float g, float b, float a = 1.0f);
    void setColor(Color c);
    void drawRect(float x, float y, float w, float h);
    void drawRect(Vec2 pos, Vec2 size);
    void drawRect(Rect rect);
    void drawBox(float x, float y, float w, float h);
    void drawBox(Vec2 pos, Vec2 size);
    void drawBox(Rect rect);

    void drawLine(Vec2 a, Vec2 b);

    static const Vec2 fontSize;
    void drawText(const char* text, Vec2 pos);
    void drawText(const char* text, float x, float y);

    void drawImage(SDL_Texture *texture, float x, float y, float w, float h);
    void drawImage(SDL_Texture *texture, Vec2 pos, Vec2 size);
};

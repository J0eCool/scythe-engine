#include "render_sdl.h"

#define SMOL_TEXT true
#if SMOL_TEXT
    #define FONT_FILE "../data/alpha_small.png"
    #define FONT_SIZE {8, 8}
#else
    #define FONT_FILE "../data/alpha.png"
    #define FONT_SIZE {21, 12}
#endif

const Vec2 Renderer::fontSize = FONT_SIZE;

Renderer::Renderer(SDL_Renderer* sdl) : _sdlRenderer(sdl) {
    const char* filename = FONT_FILE;
    SDL_Surface *surf = IMG_Load(filename);
    assert(surf, "%s failed to load", filename);
    _alphabetTexture = SDL_CreateTextureFromSurface(_sdlRenderer, surf);
    SDL_FreeSurface(surf);
}

Renderer::~Renderer() {
    SDL_DestroyTexture(_alphabetTexture);
}

SDL_Renderer* Renderer::sdl() const {
    return _sdlRenderer;
}

void Renderer::startFrame() {
    setColor(_bgColor);
    SDL_RenderClear(_sdlRenderer);
}
void Renderer::endFrame() {
    SDL_RenderPresent(_sdlRenderer);
}

void Renderer::background(Color c) {
    _bgColor = c;
}

void Renderer::setColor(float r, float g, float b, float a) {
    SDL_SetRenderDrawColor(_sdlRenderer, 255*r, 255*g, 255*b, 255*a);
}
void Renderer::setColor(Color c) {
    SDL_SetRenderDrawColor(_sdlRenderer, c.r, c.g, c.b, c.a);
}
void Renderer::drawRect(float x, float y, float w, float h) {
    SDL_Rect rect {(int)x,(int)y,(int)w,(int)h};
    SDL_RenderFillRect(_sdlRenderer, &rect);
}
void Renderer::drawRect(Vec2 pos, Vec2 size) {
    drawRect(pos.x, pos.y, size.x, size.y);
}
void Renderer::drawRect(Rect rect) {
    drawRect(rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
}
void Renderer::drawBox(float x, float y, float w, float h) {
    SDL_Rect rect {(int)x, (int)y, (int)w, (int)h};
    SDL_RenderDrawRect(_sdlRenderer, &rect);
}
void Renderer::drawBox(Vec2 pos, Vec2 size) {
    drawBox(pos.x, pos.y, size.x, size.y);
}
void Renderer::drawBox(Rect rect) {
    drawBox(rect.pos.x, rect.pos.y, rect.size.x, rect.size.y);
}

void Renderer::drawLine(Vec2 a, Vec2 b) {
    SDL_RenderDrawLine(_sdlRenderer, a.x, a.y, b.x, b.y);
}

void Renderer::drawText(const char* text, Vec2 pos) {
    drawText(text, pos.x, pos.y);
}
void Renderer::drawText(const char* text, float x, float y) {
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

void Renderer::drawImage(SDL_Texture *texture, float x, float y, float w, float h) {
    SDL_Rect destRect { (int)x, (int)y, (int)w, (int)h };
    SDL_RenderCopy(_sdlRenderer, texture, nullptr, &destRect);
}
void Renderer::drawImage(SDL_Texture *texture, Vec2 pos, Vec2 size) {
    drawImage(texture, pos.x, pos.y, size.x, size.y);
}

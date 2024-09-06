// EyeGenScene - Eye generator

#pragma once

#include "color.h"
#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "texGen.h"
#include "ui.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <math.h>

class EyeGenScene : public Scene {
    UI _ui;

    int colorIdx = 0; // current gradient step index

    // params affecting display
    int renderSize = 1024; // NxN size of total display area on screen
    int gridSize = 16; // render an NxN grid of textures

    SDL_Texture *tex = nullptr;

    bool _shouldGenerate = true;
    struct EyeParams {
        Vec2 cornerA, cornerB;
        Vec2 pupil;

        SDL_Texture* generateTexture(Renderer* renderer) const {
            int texSize = 256;
            const int bpp = 32;
            auto surface = SDL_CreateRGBSurface(
                0, texSize, texSize, bpp,
                // RGBA bitmasks; A mask is special
                0xff, 0xff << 8, 0xff << 16, 0);
            for (int x = 0; x < texSize; ++x) {
                for (int y = 0; y < texSize; ++y) {
                    Color *pixel = (Color*)((Uint8*)surface->pixels
                        + y*surface->pitch
                        + x*surface->format->BytesPerPixel);
                    Vec2 pos = Vec2 {float(x), float(y)} / texSize;
                    *pixel = Color::black;
                    float w = cornerB.x-cornerA.x;
                    float t = (pos.x-cornerA.x) / w;
                    if (t >= 0 && t <= 1) {
                        *pixel = Color::white;
                    }
                }
            }
            return SDL_CreateTextureFromSurface(renderer->sdl(), surface);
        }
    } _params;

    Color bgColor {0x60, 0x1f, 0x80};

public:
    EyeGenScene(Allocator *alloc, Input *input) :
            _ui(alloc, input) {
    }

    void onLoad() override {
        _shouldGenerate = true;
        _params = {
            {0.3, 0.5}, {0.7, 0.5},
            {0.5, 0.5},
        };
    }

    void onUnload() override {
        _ui.unload();
        SDL_DestroyTexture(tex);
    }

    void update(float dt) override {
        _ui.startUpdate({ 120, 30 });
        _ui.labels("GENERATE AN EYE OR SOMETHING", "\n");
        _ui.labels("CORNER A", _params.cornerA, "\n");
        _ui.labels("CORNER B", _params.cornerB, "\n");
        _ui.labels("PUPIL", _params.pupil, "\n");
    }

    void render(Renderer *renderer) override {
        if (_shouldGenerate) {
            _shouldGenerate = false;
            generateTexture(renderer);
        }

        renderer->background(bgColor);
        renderer->drawImage(tex, {800, 50}, {1024, 1024});

        _ui.render(renderer);
    }

    void generateTexture(Renderer* renderer) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
        tex = _params.generateTexture(renderer);
    }
};

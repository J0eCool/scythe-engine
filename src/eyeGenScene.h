// EyeGenScene - Eye generator

#pragma once

#include "color.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "ui.h"
#include "vec.h"

#include <SDL2/SDL.h>

class EyeGenScene : public Scene {
    UI _ui;
    Input* _input;

    SDL_Texture *tex = nullptr;

    Color bgColor {0x60, 0x1f, 0x80};
    Vec2 previewPos {800, 50};
    Vec2 previewSize {1024, 1024};

    bool _shouldGenerate = true;
    struct EyeParams {
        Vec2 cornerA, cornerB;
        Vec2 pupil;
        float curveTop, curveBot;
        float pupilSize, iris;
        Color color;

        SDL_Texture* generateTexture(Renderer* renderer) const;
    } _params;

public:
    EyeGenScene(Allocator *alloc, Input *input); 

    void onLoad() override;
    void onUnload() override;
    void update(float dt) override;
    void render(Renderer *renderer) override;

    void generateTexture(Renderer* renderer);
};

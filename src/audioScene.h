// AudioScene - Playing around with SDL_Audio

#pragma once

#include "color.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "ui.h"

#include <SDL2/SDL.h>

class AudioScene : public Scene {
    UI _ui;
    Input* _input;

    Color bgColor {0x90, 0x10, 0x20};

public:
    AudioScene(Allocator *alloc, Input *input); 

    void onLoad() override;
    void onUnload() override;
    void update(float dt) override;
    void render(Renderer *renderer) override;
};

// TexGenScene - texture generator

#pragma once

#include "color.h"
#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "texGen.h"
#include "ui.h"
#include "uilib.h"
#include "vec.h"

#include <algorithm>
#include <math.h>

class TexGenScene : public Scene {
    TexGen *_texGen;
    UI _ui;

    int colorIdx = 0; // current gradient step index

    // params affecting display
    int renderSize = 1024; // NxN size of total display area on screen
    int gridSize = 16; // render an NxN grid of textures

    bool _shouldGenerate;

public:
    TexGenScene(TexGen *texGen, Allocator *alloc, Input *input) :
            _texGen(texGen), _ui(alloc, input) {
    }

    void onLoad() override {
        _shouldGenerate = true; 

        loadParams();
    }

    void onUnload() override {
        _ui.unload();

        saveParams();
    }

    const char* saveFile = "../data/texture.texParams";
    void saveParams() {
        saveToFile(saveFile, _texGen->texParams);
        log("params saved to file");
    }

    void loadParams() {
        if (loadFromFile(saveFile, _texGen->texParams)) {
            log("params loaded from file");
            _shouldGenerate = true;
        }
    }

    void update(float dt) override {
        _texGen->update(dt);
        // if we change a param that affects texture appearance, regenerate the textures
        _shouldGenerate = _texGen->isAnimating();
        
        _ui.startUpdate({ 90, 30 });

        _ui.align(180);
        if (_ui.button("reroll")) {
            _texGen->reroll();
            _shouldGenerate = true;
        }
        if (_ui.button("save")) {
            saveParams();
        }
        if (_ui.button("load")) {
            loadParams();
        }
        _ui.line();

        _shouldGenerate |= uiParam(_ui, "noise size", _texGen->texParams.noiseSize,
            1, 3, _texGen->texParams.texSize);
        _shouldGenerate |= uiParam(_ui, "num textures", _texGen->texParams.numTextures,
            1, 1, 64);
        const int nModes = 6;
        _shouldGenerate |= uiParam(_ui, "mode", _texGen->texParams.mode,
            1, 0, nModes-1);
        _shouldGenerate |= uiParam(_ui, "noise scale", _texGen->texParams.noiseScale,
            1, 1, 10);
        _shouldGenerate |= uiParamMult(_ui, "tex size", _texGen->texParams.texSize,
            2, 4, 512);
        _shouldGenerate |= uiParamMult(_ui, "grid size", gridSize,
            2, 1, 64);

        _shouldGenerate |= uiParam<float>(_ui, "noise anim", _texGen->texParams.noiseAnimScale,
            0.01, 0, 1);
        _shouldGenerate |= uiParam<float>(_ui, "grad anim", _texGen->texParams.gradAnimScale,
            0.01, 0, 1);
        _shouldGenerate |= uiParam<float>(_ui, "tile anim", _texGen->texParams.tileAnimScale,
            0.01, 0, 5);

        auto &steps = _texGen->texParams.gradient.steps;
        for (auto step : steps) {
            _ui.rect(step.color, Vec2{32});
        }
        if (_ui.button("reset")) {
            _texGen->texParams.gradient = Gradient{};
            _shouldGenerate = true;
            colorIdx = 0;
        }
        if (_ui.button("RANDOMIZE")) {
            auto& params = _texGen->texParams;
            for (auto &step : params.gradient.steps) {
                step.color.r = rand() % 256;
                step.color.g = rand() % 256;
                step.color.b = rand() % 256;
                step.pos = (rand() % 1024) / 1024.0;
            }
            params.gradient.steps[0].pos = 0;
            params.gradient.steps[1].pos = 1;
            std::sort(params.gradient.steps.begin(), params.gradient.steps.end());
            _texGen->reroll();
            _shouldGenerate = true;
        }
        _ui.line();
        int nColors = steps.size();
        GradientStep &step = steps[colorIdx];
        uiParam(_ui, "gradient idx", colorIdx,
            (colorIdx+nColors-1) % nColors, (colorIdx+1) % nColors,
            0, nColors-1);
        _ui.line();
        _ui.align(40);
        if (_ui.button("add")) {
            if (colorIdx+1 < steps.size()) {
                GradientStep newStep {
                    lerp(0.5, step.color, steps[colorIdx+1].color),
                    lerp(0.5, step.pos, steps[colorIdx+1].pos)
                };
                steps.insert(steps.begin()+colorIdx+1, newStep);
            } else {
                steps.push_back({step.color, 1.0});
            }
            colorIdx++;
            _shouldGenerate = true;
        }
        if (nColors > 2 && _ui.button("del")) {
            steps.erase(steps.begin()+colorIdx);
            colorIdx--;
            _shouldGenerate = true;
        }
        _shouldGenerate |= uiParam<float>(_ui, "pos", step.pos,
            step.pos-0.01, step.pos+0.01,
            0, 1);
        _ui.align(40);
        if (_ui.button("dup")) {
            // duplicate current color
            steps.insert(steps.begin()+colorIdx+1, step);
        }
        _shouldGenerate |= uiColor(_ui, step.color);

        // maintain sort order of color steps when pos changes
        if (colorIdx > 0 && step.pos < steps[colorIdx-1].pos) {
            std::swap(steps[colorIdx-1], steps[colorIdx]);
            colorIdx--;
        }
        if (colorIdx+1 < steps.size() && step.pos > steps[colorIdx+1].pos) {
            std::swap(steps[colorIdx], steps[colorIdx+1]);
            colorIdx++;
        }
    }

public:
    Vec2 tileSize() const {
        return Vec2{(float)renderSize} / gridSize;
    }

    void render(Renderer *renderer) override {
        renderer->background({0x30, 0x8f, 0x10});
        if (_shouldGenerate) {
            _texGen->generateTextures(renderer);
            _shouldGenerate = false;
        }

        int rep = gridSize;
        Vec2 ts = tileSize();
        for (int i = 0; i < rep; ++i) {
            for (int j = 0; j < rep; ++j) {
                renderer->drawImage(_texGen->textureForIndex(i*rep + j),
                    Vec2{800, 40} + Vec2i{i, j}.to<float>()*tileSize(),
                    tileSize());
            }
        }

        _texGen->renderAtlas(renderer, {160, 820});

        _ui.render(renderer);
    }
};

#include "eyeGenScene.h"

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "ui.h"
#include "uilib.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <math.h>

SDL_Texture* EyeGenScene::EyeParams::generateTexture(Renderer* renderer) const {
    int texSize = 256;
    const int bpp = 32;
    auto surface = SDL_CreateRGBSurface(
        0, texSize, texSize, bpp,
        // RGBA bitmasks; A mask is special
        0xff, 0xff << 8, 0xff << 16, 0);
    Vec2 ba = cornerB - cornerA;
    Vec2 right = ba.normalized();
    // {-y, x} is 90deg counterclockwise from {x, y}
    Vec2 up = Vec2{-right.y, right.x};
    for (int x = 0; x < texSize; ++x) {
        for (int y = 0; y < texSize; ++y) {
            Color *pixel = (Color*)((Uint8*)surface->pixels
                + y*surface->pitch
                + x*surface->format->BytesPerPixel);
            Vec2 pos = Vec2 {float(x), float(y)} / texSize;
            *pixel = Color::black;

            Vec2 pa = pos-cornerA;
            Vec2 uv {
                // dot product projects the components of pa onto the basis vecs
                pa.dot(right) / ba.len(),
                pa.dot(up) / ba.len(),
            };

            if (uv.x >= 0 && uv.x <= 1) {
                float top = curveTop*uv.x*(uv.x - 1);
                float bot = curveBot*uv.x*(1 - uv.x);
                if (uv.y > top && uv.y < bot) {
                    *pixel = Color::white;
                    float r = (pos - pupil).len();
                    if (r < (pupilSize/2)) {
                        *pixel = r/(pupilSize/2) < (1-iris)
                            ? Color::black : color;
                    }
                }
            }
        }
    }
    auto tex = SDL_CreateTextureFromSurface(renderer->sdl(), surface);
    SDL_FreeSurface(surface);
    return tex;
}

EyeGenScene::EyeGenScene(Allocator *alloc, Input *input) :
        _input(input), _ui(alloc, input) {
}

void EyeGenScene::onLoad() {
    _shouldGenerate = true;
    _params = {
        {0.1, 0.3}, {0.9, 0.5},
        {0.4, 0.5},
        0.2, 1.3,
        0.35, 0.3,
        {0x10, 0x20, 0x90},
    };
}

void EyeGenScene::onUnload() {
    _ui.unload();
    SDL_DestroyTexture(tex);
}

void EyeGenScene::update(float dt) {
    _ui.startUpdate({ 120, 30 });
    _ui.labels("GENERATE AN EYE OR SOMETHING", "\n");
    _shouldGenerate |=
        uiParam(_ui, "A.x", _params.cornerA.x, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "A.y", _params.cornerA.y, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "B.x", _params.cornerB.x, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "B.y", _params.cornerB.y, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "curveTop", _params.curveTop, 0.01f, -1.0f, 3.0f);
    _shouldGenerate |=
        uiParam(_ui, "curveBot", _params.curveBot, 0.01f, -1.0f, 3.0f);
    _shouldGenerate |=
        uiParam(_ui, "P.x", _params.pupil.x, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "P.y", _params.pupil.y, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "pupilSize", _params.pupilSize, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |=
        uiParam(_ui, "iris", _params.iris, 0.01f, 0.0f, 1.0f);
    _shouldGenerate |= uiColor(_ui, _params.color);

    auto mouse = _input->getMousePos();
    auto pos = (mouse - previewPos)/previewSize;
    if (in_rect(pos, {0, 0}, {1, 1})) {
        if (_input->isHeld("click")) {
            _shouldGenerate = true;
            _params.pupil = pos;
        }
        if (_input->isHeld("rclick")) {
            _shouldGenerate = true;
            _params.cornerB = pos;
        }
    }
}

void EyeGenScene::render(Renderer *renderer) {
    if (_shouldGenerate) {
        _shouldGenerate = false;
        generateTexture(renderer);
    }

    renderer->background(bgColor);
    renderer->drawImage(tex, previewPos, previewSize);

    _ui.render(renderer);
}

void EyeGenScene::generateTexture(Renderer* renderer) {
    if (tex) {
        SDL_DestroyTexture(tex);
    }
    tex = _params.generateTexture(renderer);
}

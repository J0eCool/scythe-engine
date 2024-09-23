#include "eyeGenScene.h"

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "texGen.h" // hey probably remove this
#include "ui.h"
#include "uilib.h"
#include "vec.h"

#include <SDL2/SDL.h>
#include <math.h>

///-----------------------------------------------------------------------------
/// BOY HOWDY it's the HAX REGION
///-----------------------------------------------------------------------------

/// color.cpp -----------------------------------------------------------------

const Color Color::black { 0x00, 0x00, 0x00 };
const Color Color::white { 0xff, 0xff, 0xff };
const Color Color::red   { 0xff, 0x00, 0x00 };
const Color Color::green { 0x00, 0xff, 0x00 };
const Color Color::blue  { 0x00, 0x00, 0xff };

Color operator*(float s, Color c) {
    return c*s;
}

Color rgbColor(float r, float g, float b, float a) {
    return {Uint8(0xff*r), Uint8(0xff*g), Uint8(0xff*b), Uint8(0xff*a)};
}

Color hsvColor(float h, float s, float v) {
    // algorithm from https://www.rapidtables.com/convert/color/hsv-to-rgb.html
    const float c = v*s;
    const float x = c*(1 - fabs(fmod(h/60, 2) - 1));
    const float m = v - c;
    Color rgb;
    h = fmod(h, 360);
    if (h < 60)       { rgb = rgbColor(c, x, 0); }
    else if (h < 120) { rgb = rgbColor(x, c, 0); }
    else if (h < 180) { rgb = rgbColor(0, c, x); }
    else if (h < 240) { rgb = rgbColor(0, x, c); }
    else if (h < 300) { rgb = rgbColor(x, 0, c); }
    else              { rgb = rgbColor(c, 0, x); }
    rgb.r += m;
    rgb.g += m;
    rgb.b += m;
    return rgb;
}

std::ostream& operator<<(std::ostream &stream, Color const& c) {
    return stream << (int32_t)c.r << ' ' << (int32_t)c.g << ' ' << (int32_t)c.b << ' ' << (int32_t)c.a;
}
std::istream& operator>>(std::istream &stream, Color &c) {
    int32_t r, g, b, a;
    stream >> r >> g >> b >> a;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return stream;
}

std::ostream& operator<<(std::ostream &stream, Gradient const& gradient) {
    stream << gradient.steps.size() << ' ';
    for (auto &step : gradient.steps) {
        stream << step.color << ' ' << step.pos << '\n';
    }
    return stream;
}
std::istream& operator>>(std::istream &stream, Gradient &gradient) {
    int nSteps;
    stream >> nSteps;
    gradient.steps.clear();
    for (int i = 0; i < nSteps; ++i) {
        GradientStep step;
        stream >> step.color >> step.pos;
        gradient.steps.push_back(step);
    }
    return stream;
}

/// vec.cpp --------------------------------------------------------------------

Vec2i floorv(Vec2f v) {
    return { (int)floor(v.x), (int)floor(v.y) };
}
Vec2i ceilv(Vec2f v) {
    return { (int)ceil(v.x), (int)ceil(v.y) };
}

bool in_rect(Vec2 p, Vec2 rPos, Vec2 rSize) {
    return p.x >= rPos.x && p.x <= rPos.x + rSize.x &&
        p.y >= rPos.y && p.y <= rPos.y + rSize.y;
}

/// ui.cpp ---------------------------------------------------------------------

template <>
float sliderLerp<float>(float t, float lo, float hi) {
    return lerp(t, lo, hi);
}

/// texGen.cpp -----------------------------------------------------------------

NoiseSample operator*(float s, NoiseSample n) {
    return { s*n.v, s*n.color, s*n.pos };
}
NoiseSample operator+(NoiseSample a, NoiseSample b) {
    return { a.v+b.v, a.color+b.color, a.pos+b.pos };
}

float dot(NoiseSample a, NoiseSample b) {
    // I mean in principle this isn't too bad
    // we're computing a measure of how well-aligned two samples are in a
    // 3D space of x,y,v
    return a.v*b.v + dot(a.pos, b.pos);
}

Serialize<TexParams> serialize(TexParams &params) {
    Serialize<TexParams> serial(params);
    serial.addField("seed", params.seed);
    serial.addField("noiseSize", params.noiseSize);
    serial.addField("numTextures", params.numTextures);
    serial.addField("mode", params.mode);
    serial.addField("noiseScale", params.noiseScale);
    serial.addField("texSize", params.texSize);
    serial.addField("gradient", params.gradient);
    serial.addField("gradAnimScale", params.gradAnimScale);
    serial.addField("noiseAnimScale", params.noiseAnimScale);
    serial.addField("tileAnimScale", params.tileAnimScale);
    return serial;
}

/// uilib.cpp ------------------------------------------------------------------

bool uiToggle(UI &ui, bool &option, const char* ifOn, const char* ifOff) {
    if (ui.button(option ? ifOn : ifOff)) {
        option = !option;
        return true;
    }
    return false;
}

bool uiColor(UI &ui, Color &color) {
    bool changed = false;
    ui.align(160);
    ui.rect(color, Vec2{32});
    const int delta = 5;
    changed |= uiParam(ui, "R", color.r,
        Uint8(color.r-delta), Uint8(color.r+delta),
        Uint8(0), Uint8(255));
    changed |= uiParam(ui, "G", color.g,
        Uint8(color.g-delta), Uint8(color.g+delta),
        Uint8(0), Uint8(255));
    changed |= uiParam(ui, "B", color.b,
        Uint8(color.b-delta), Uint8(color.b+delta),
        Uint8(0), Uint8(255));
    return changed;
}

/// render_sdl.cpp -------------------------------------------------------------

const Vec2 Renderer::fontSize = FONT_SIZE;

///-----------------------------------------------------------------------------
/// BOY HOWDY it's the HAX REGION
///-----------------------------------------------------------------------------

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

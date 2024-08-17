// TexGenScene - texture generator

#pragma once

#include "common.h"
#include "ui.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "vec.h"

#include <math.h>
#include <random>
#include <vector>

#include <SDL2/SDL.h>

struct GradientStep {
    Color color;
    float pos; // [0, 1], what value of the gradient is set to this color
};

struct Gradient {
    std::vector<GradientStep> steps;

    Gradient() {
        steps.push_back({Color::black, 0});
        steps.push_back({Color::white, 1});
    }

    Color sample(float t) {
        t = clamp(t);
        // we assume that the steps are sorted
        int i = 0;
        for (; i < steps.size()-1; ++i) {
            if (steps[i+1].pos >= t) {
                break;
            }
        }
        float s = (t-steps[i].pos) / (steps[i+1].pos-steps[i].pos);
        return lerp(s, steps[i].color, steps[i+1].color);
    }
};

class TexGenScene {
    UI _ui;
    std::ranlux24_base _rand_engine;
    // latest seed used to generate textures
    Uint32 _textureSeed;
    std::vector<SDL_Texture*> _textures;
    std::vector<int> _texIndices;

    struct {
        // params afffecting generation
        int noiseSize = 11; // generate an NxN grid of samples
        int numTextures = 8; // generate N texture variations
        // `mode` is how to display the noise data
        //   0 - display each grid cell using the upper-left point's color
        //   1 - interpolate between the color values of each
        //   2 - map noise value onto a color gradient
        //   3 - map noise value onto a grayscale gradient
        //   4 - same as mode 3, but color-coded per tile variant
        //   5 - gradient-mapped color values (soon the default)
        int mode = 5;
        int noiseScale = 2; // range of the scalar noise values
        int texSize = 32; // NxN pixel size of generated texture
        Gradient gradient;
    } texParams;

    int colorIdx = 0; // current gradient step index

    // params affecting display
    int renderSize = 1024; // NxN size of total display area on screen
    int gridSize = 16; // render an NxN grid of textures

    int randInt(int limit = INT32_MAX) {
        return std::uniform_int_distribution<int>(0, limit-1)(_rand_engine);
    }
    float randFloat(float limit = 1.0) {
        return std::uniform_real_distribution<float>(0, limit)(_rand_engine);
    }
    float randFloat(float lo, float hi) {
        return lerp(randFloat(), lo, hi);
    }
    Uint8 randByte() {
        return randInt(0x100);
    }
    /// @brief Generate a random `true` or `false` value
    /// @param p probability of a `true` result
    bool randBool(float p = 0.5) {
        return randFloat() < p;
    }
    /// @brief Approximate a normal distribution by taking the average of 4 rolls.
    /// This biases the distribution towards 0.5
    /// @return A value between 0 and 1
    float randNormalish() {
        return (randFloat() + randFloat() + randFloat() + randFloat()) / 4;
    }

    // data per grid cell
    struct NoiseSample {
        float v; // scalar noise value
        Color color; // RGB color data
        Vec2f pos; // offset within the grid cell, expected to be in [0, 1)
    };

    void generateNoise(NoiseSample* noise, int n) {
        for (int y = 0; y < n; ++y) {
            for (int x = 0; x < n; ++x) {
                noise[y*n + x] = {
                    randFloat(0.0, 1.0),
                    { randByte(), randByte(), randByte(), 0xff },
                    { randFloat(0.3, 1.0),
                      randFloat(0.3, 1.0),
                    },
                };
            }
        }
    }

    SDL_Surface* generateSurface(NoiseSample* noise, int n, int texSize) {
        const int bpp = 32;
        SDL_Surface *surface = SDL_CreateRGBSurface(
            0, texSize, texSize, bpp,
            // RGBA bitmasks; A mask is special
            0xff, 0xff << 8, 0xff << 16, 0);
        Vec2f ntoi { float(texSize)/n, float(texSize)/n };
        Vec2f iton { float(n)/texSize, float(n)/texSize };
        // for each noise cell
        for (int y = -1; y < n; ++y) {
            for (int x = -1; x < n; ++x) {
                // sample each corner 
                // _i for index
                // _n for noise-coord position
                Vec2f p_n = {float(x), float(y)};
                Vec2i ul_i = Vec2i{x+0, y+0};
                Vec2i ur_i = Vec2i{x+0, y+1};
                Vec2i bl_i = Vec2i{x+1, y+0};
                Vec2i br_i = Vec2i{x+1, y+1};
                NoiseSample ul_s = noise[((ul_i.y+n) % n)*n + ((ul_i.x+n) % n)];
                NoiseSample ur_s = noise[((ur_i.y+n) % n)*n + ((ur_i.x+n) % n)];
                NoiseSample bl_s = noise[((bl_i.y+n) % n)*n + ((bl_i.x+n) % n)];
                NoiseSample br_s = noise[((br_i.y+n) % n)*n + ((br_i.x+n) % n)];
                Vec2f ul = ul_i.to<float>() + ul_s.pos;
                Vec2f ur = ur_i.to<float>() + ur_s.pos;
                Vec2f bl = bl_i.to<float>() + bl_s.pos;
                Vec2f br = br_i.to<float>() + br_s.pos;
                Vec2i bound_lo = floorv(ntoi*min(ul, min(ur, bl)));
                Vec2i bound_hi =  ceilv(ntoi*max(br, max(ur, bl)));
                assert(bound_lo.x <= bound_hi.x && bound_lo.y <= bound_hi.y,
                    "invalid noise bounds <%f, %f> hi=<%f, %f>",
                    bound_lo.x, bound_lo.y, bound_hi.x, bound_hi.y);

                // iterate over the pixels in the AABB
                for (int j = max(0, bound_lo.y); j < min(texSize, bound_hi.y); ++j) {
                    for (int i = max(0, bound_lo.x); i < min(texSize, bound_hi.x); ++i) {
                        Vec2 p = iton * Vec2 { float(i), float(j) };

                        // convert to UV coordinates inside the quad
                        // if either U or V is out of [0, 1], then we're not inside the quad
                        // https://iquilezles.org/articles/ibilinear/
                        Vec2 e = ur-ul;
                        Vec2 f = bl-ul;
                        Vec2 g = ul-ur+br-bl;
                        Vec2 h = p - ul;
                        float k2 = g.cross(f);
                        float k1 = e.cross(f) + h.cross(g);
                        float k0 = h.cross(e);
                        float disc = k1*k1 - 4*k0*k2;
                        Vec2 uv;
                        if (abs(k2) < 0.001) {
                            // if perfectly parallel, linear
                            uv = {
                                (h.x*k1 + f.x*k0) / (e.x*k1 - g.x*k0),
                                -k0/k1
                            };
                        } else {
                            if (disc < 0) {
                                continue;
                            }

                            uv.y = (-k1 - sqrt(disc))/(2*k2);
                            uv.x = (h.x - f.x*uv.y)/(e.x + g.x*uv.y);
                            if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
                                uv.y = (-k1 + sqrt(disc))/(2*k2);
                                uv.x = (h.x - f.x*uv.y)/(e.x + g.x*uv.y);
                            }
                            if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1) {
                                continue;
                            }
                        }

                        // given 4 samples and a UV coordinate, interpolate
                        Color *pixel = (Color*)((Uint8*)surface->pixels
                            + j*surface->pitch
                            + i*surface->format->BytesPerPixel);
                        if (texParams.mode == 0) {
                            *pixel = ul_s.color;
                        } else if (texParams.mode == 1) {
                            Color a = smoothstep(uv.x, ul_s.color, ur_s.color);
                            Color b = smoothstep(uv.x, bl_s.color, br_s.color);
                            Color c = smoothstep(uv.y, a, b);
                            *pixel = c;
                        } else {
                            float a = smoothstep(uv.x, ul_s.v, ur_s.v);
                            float b = smoothstep(uv.x, bl_s.v, br_s.v);
                            float c = smoothstep(uv.y, a, b);
                            int cc = 0xff*(texParams.noiseScale*c);
                            if (texParams.mode == 2) {
                                *pixel = {0, 0, 0, 0xff};
                                // RGB cycles in 3s, value up/down cycles in 2s
                                bool even = (cc % 0x200) < 0x100;
                                cc = cc % 0x300;
                                if (cc >= 0x200) {
                                    if (even) cc = 0xff - (cc%0x100);
                                    pixel->g = cc % 0x100;
                                } else if (cc >= 0x100) {
                                    if (even) cc = 0xff - (cc%0x100);
                                    pixel->r = cc % 0x100;
                                } else {
                                    if (even) cc = 0xff - (cc%0x100);
                                    pixel->b = cc % 0x100;
                                }
                            } else if (texParams.mode == 3) {
                                if ((cc/0x100) % 2 == 0) {
                                    cc = (0xff - cc%0x100);
                                } else {
                                    cc %= 0x100;
                                }
                                *pixel = {Uint8(cc), Uint8(cc), Uint8(cc)};
                            } else if (texParams.mode == 4) {
                                if ((cc/0x100) % 2 == 0) {
                                    cc = (0xff - cc%0x100);
                                } else {
                                    cc %= 0x100;
                                }
                                const float r = 360.0f * (9/43.0f);
                                *pixel = (cc/255.0f) * hsvColor(r*_textures.size(), 1, 1);
                            } else if (texParams.mode == 5) {
                                *pixel = texParams.gradient.sample(c);
                            } else {
                                check(false, "invalid mode");
                                return surface;
                            }
                        }
                    }
                }
            }
        }
        return surface;
    }

    template <typename T>
    bool uiParam(const char* text, T &val, T dec, T inc, T lo, T hi) {
        // right-align the labels :O
        _ui.align(240-Renderer::fontSize.x*(strlen(text)+2));
        _ui.labels(text, ":");
        T set = val;
        if (_ui.button("<")) {
            set = max(dec, lo);
        }
        _ui.label(val);
        if (_ui.button(">")) {
            set = min(inc, hi);
        }
        _ui.align(400);
        _ui.slider(set, lo, hi);        
        _ui.line();
        if (set != val) {
            val = set;
            return true;
        }
        return false;
    }

    /// @brief UI Widget that toggles a boolean variable
    /// @param option the option to display and update
    /// @param ifOn text to show when `option` is `true`
    /// @param ifOff text when `option` is `false`
    /// @return `true` when clicked (for callback purposes)
    bool uiToggle(bool &option, const char* ifOn, const char* ifOff) {
        if (_ui.button(option ? ifOn : ifOff)) {
            option = !option;
            return true;
        }
        return false;
    }

    /// @brief bootleg color picker
    /// @param color color to edit
    /// @return `true` when modified
    bool uiColor(Color &color) {
        bool changed = false;
        _ui.align(160);
        _ui.rect(color, Vec2{32});
        changed |= uiParam("R", color.r,
            Uint8(color.r-1), Uint8(color.r+1),
            Uint8(0), Uint8(255));
        changed |= uiParam("G", color.g,
            Uint8(color.g-1), Uint8(color.g+1),
            Uint8(0), Uint8(255));
        changed |= uiParam("B", color.b,
            Uint8(color.b-1), Uint8(color.b+1),
            Uint8(0), Uint8(255));
        return changed;
    }

public:
    TexGenScene(Allocator *alloc, Input *input) : _ui(alloc, input) {}

    ~TexGenScene() {
        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
    }

    void onLoad() {
        // random_device seems to give a consistent value in dll, so offset by
        // ticks elapsed since program start to get new results on each reload
        std::random_device r;
        _textureSeed = r() + SDL_GetTicks();
        _rand_engine = std::ranlux24_base(_textureSeed);
    }

    void onUnload() {
        _ui.unload();
    }

    void generateTextures(Renderer *renderer) {
        Tracer("Game::generateTextures");
        Timer timer;

        _rand_engine.seed(_textureSeed);

        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
        _textures.clear();
        _texIndices.clear();
        // noise width/height
        int n = texParams.noiseSize;
        NoiseSample boundary[n*n];
        generateNoise(boundary, n);

        for (int i = 0; i < texParams.numTextures; ++i) {
            NoiseSample noise[n*n];
            generateNoise(noise, n);
            // set the generated noise's boundary to be equal to the boundary noise
            for (int j = 0; j < n; ++j) {
                /* [x][0]   */ noise[j] = boundary[j];
                /* [x][n-1] */ noise[j+n*(n-1)] = boundary[j+n];
                /* [0][y]   */ noise[j*n] = boundary[j*n];
                /* [n-1][y] */ noise[j*n+n-1] = boundary[j*n+n-1];
                // noise[j].color.g = noise[j].color.r; noise[j].color.b = noise[j].color.r;
                // noise[j+n*(n-1)].color.g = noise[j+n*(n-1)].color.r; noise[j+n*(n-1)].color.b = noise[j+n*(n-1)].color.r;
                // noise[j*n].color.g = noise[j*n].color.r; noise[j*n].color.b = noise[j*n].color.r;
                // noise[j*n+n-1].color.g = noise[j*n+n-1].color.r; noise[j*n+n-1].color.b = noise[j*n+n-1].color.r;
            }
            SDL_Surface *surface = generateSurface(noise, n, texParams.texSize);
            auto sdl = renderer->sdl();
            _textures.push_back(SDL_CreateTextureFromSurface(sdl, surface));
            SDL_FreeSurface(surface);
        }

        log("Generated %d textures in %dms", _textures.size(), timer.elapsedMs());
    }

    void update(Renderer *renderer) {
        // if we change a param that affects texture appearance, regenerate the textures
        bool changed = false;
        
        _ui.startUpdate({ 30, 30 });

        _ui.line();
        if (_ui.button("reroll")) {
            _textureSeed = _rand_engine();
            changed = true;
        }

        changed |= uiParam("noise size", texParams.noiseSize,
            texParams.noiseSize-1, texParams.noiseSize+1,
            3, texParams.texSize);
        changed |= uiParam("num textures", texParams.numTextures,
            texParams.numTextures-1, texParams.numTextures+1,
            1, 64);
        const int nModes = 6;
        changed |= uiParam("mode", texParams.mode,
            (texParams.mode+nModes-1) % nModes, (texParams.mode+1) % nModes,
            0, nModes-1);
        changed |= uiParam("noise scale", texParams.noiseScale,
            texParams.noiseScale-1, texParams.noiseScale+1,
            1, 10);
        changed |= uiParam("tex size", texParams.texSize,
            texParams.texSize/2, texParams.texSize*2,
            4, 512);
        changed |= uiParam("grid size", gridSize,
            gridSize/2, gridSize*2,
            1, 64);

        auto &steps = texParams.gradient.steps;
        for (auto step : steps) {
            _ui.rect(step.color, Vec2{32});
        }
        if (_ui.button("reset")) {
            texParams.gradient = Gradient{};
            changed = true;
            colorIdx = 0;
        }
        _ui.line();
        int nColors = steps.size();
        GradientStep &step = steps[colorIdx];
        uiParam("gradient idx", colorIdx,
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
            changed = true;
        }
        if (nColors > 2 && _ui.button("del")) {
            steps.erase(steps.begin()+colorIdx);
            colorIdx--;
            changed = true;
        }
        changed |= uiParam<float>("pos", step.pos,
            step.pos-0.02, step.pos+0.02,
            0, 1);
        changed |= uiColor(step.color);

        // maintain sort order of color steps when pos changes
        if (colorIdx > 0 && step.pos < steps[colorIdx-1].pos) {
            std::swap(steps[colorIdx-1], steps[colorIdx]);
            colorIdx--;
        }
        if (colorIdx+1 < steps.size() && step.pos > steps[colorIdx+1].pos) {
            std::swap(steps[colorIdx], steps[colorIdx+1]);
            colorIdx++;
        }

        if (changed) {
            generateTextures(renderer);
        }
    }

    Vec2 tileSize() const {
        return Vec2{(float)renderSize} / gridSize;
    }

    SDL_Texture* textureForIndex(int index) {
        // assert in case we get an underflowed `index` or something
        assert(_texIndices.size() < index+100'000,
            "don't generate more than 100,000 texture indices at a time pls");
        while (index >= _texIndices.size()) {
            _texIndices.push_back(randInt(_textures.size()));
        }
        return _textures[_texIndices[index]];
    }

    void render(Renderer *renderer) {
        int rep = gridSize;
        Vec2 ts = tileSize();
        for (int i = 0; i < rep; ++i) {
            for (int j = 0; j < rep; ++j) {
                renderer->drawImage(textureForIndex(i*rep + j),
                    Vec2{800, 40} + Vec2i{i, j}.to<float>()*tileSize(),
                    tileSize());
            }
        }

        int n = (int)ceil(sqrt(_textures.size()));
        for (int i = 0; i < _textures.size(); ++i) {
            float size = texParams.texSize;
            renderer->drawImage(_textures[i],
                {760-size*(n - i%n), 1000-size*(n - i/n)},
                {size, size});
        }

        _ui.render(renderer);
    }
};
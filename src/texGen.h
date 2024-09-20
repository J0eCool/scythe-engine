#pragma once

#include "color.h"
#include "serialize.h"
#include "vec.h"

#include <alloca.h>
#include <fstream>
#include <random>

#include <SDL2/SDL.h>

class Rng {
    std::ranlux24_base _rand_engine;
public:
    Rng() {}

    void seed() {
        // random_device seems to give a consistent value in dll, so offset by
        // ticks elapsed since program start to get new results on each reload
        std::random_device r;
        auto seed = r() + SDL_GetTicks();
        _rand_engine = std::ranlux24_base(seed);
    }
    void seed(uint32_t s) {
        _rand_engine.seed(s);
    }

    int Int(int limit = INT32_MAX) {
        return std::uniform_int_distribution<int>(0, limit-1)(_rand_engine);
    }
    float Float(float limit = 1.0) {
        return std::uniform_real_distribution<float>(0, limit)(_rand_engine);
    }
    float Float(float lo, float hi) {
        return lerp(Float(), lo, hi);
    }
    Uint8 Byte() {
        return Int(0x100);
    }
    /// @brief Generate a random `true` or `false` value
    /// @param p probability of a `true` result
    bool Bool(float p = 0.5) {
        return Float() < p;
    }
    /// @brief Approximate a normal distribution by taking the average of 4 rolls.
    /// This biases the distribution towards 0.5
    /// @return A value between 0 and 1
    float Normalish() {
        return (Float() + Float() + Float() + Float()) / 4;
    }
};

// data per grid cell
struct NoiseSample {
    float v; // scalar noise value
    Color color; // RGB color data
    Vec2f pos; // offset within the grid cell, expected to be in [0, 1)
};
NoiseSample operator*(float s, NoiseSample n);
NoiseSample operator+(NoiseSample a, NoiseSample b);
// ok this one's pretty dubious... used to let us slerp noise samples
float dot(NoiseSample a, NoiseSample b);

struct TexParams {
    // seed used to generate textures
    uint32_t seed;

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

    float gradAnimScale = 0;
    float noiseAnimScale = 0;
    float tileAnimScale = 0;
};
Serialize<TexParams> serialize(TexParams &params);

class TexGen {
    std::vector<SDL_Texture*> _textures;
    std::vector<int> _texIndices;

    Rng rng;

public:
    TexParams texParams;

    float gradAnimTime;
    float noiseAnimTime;
    float tileAnimTime;

    ~TexGen() {
        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
    }

    void seed() {
        rng.seed();
    }

private:
    void generateNoise(NoiseSample* noise, int n) {
        for (int y = 0; y < n; ++y) {
            for (int x = 0; x < n; ++x) {
                NoiseSample &s = noise[y*n + x];
                s = {
                    rng.Float(0.0, 1.0),
                    { rng.Byte(), rng.Byte(), rng.Byte(), 0xff },
                    { rng.Float(0.3, 1.0),
                      rng.Float(0.3, 1.0),
                    },
                };
                s.v = sin(PI*(s.v-0.5) + TAU*noiseAnimTime)/2+0.5f;
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

                        float a = smoothstep(uv.x, ul_s.v, ur_s.v);
                        float b = smoothstep(uv.x, bl_s.v, br_s.v);
                        float c = smoothstep(uv.y, a, b);
                        // given 4 samples and a UV coordinate, interpolate
                        Color *pixel = (Color*)((Uint8*)surface->pixels
                            + j*surface->pitch
                            + i*surface->format->BytesPerPixel);
                        if (texParams.mode == 0) {
                            *pixel = ul_s.color;
                        } else if (texParams.mode == 1) {
                            Color a = smoothstep(uv.x, ul_s.color, ur_s.color);
                            Color b = smoothstep(uv.x, bl_s.color, br_s.color);
                            *pixel = smoothstep(uv.y, a, b);
                        } else {
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
                                c = abs(fmod(c+gradAnimTime,2.0f)-1);
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

public:
    void generateTextures(Renderer *renderer) {
        Tracer("Game::generateTextures");
        Timer timer;

        rng.seed(texParams.seed);

        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
        _textures.clear();
        _texIndices.clear();
        // noise width/height
        int n = texParams.noiseSize;
        std::vector<NoiseSample*> noises; // a list of NxN grids of noise

        // generate the noise grids
        NoiseSample boundary[n*n];
        generateNoise(boundary, n);
        for (int i = 0; i < texParams.numTextures; ++i) {
            NoiseSample *noise = (NoiseSample*)alloca(n*n*sizeof(NoiseSample));
            generateNoise(noise, n);
            if (i == (int)(tileAnimTime/2) % texParams.numTextures) {
                // actual boundary is lerped between the different tiles'
                for (int j = 0; j < n*n; ++j) {
                    float t = 2*abs(fmod(tileAnimTime,2.0)/2 - 0.5);
                    boundary[j] = slerp(t, noise[j], boundary[j]);
                }
            }
            noises.push_back(noise);
        }
        
        for (int i = 0; i < texParams.numTextures; ++i) {
            NoiseSample noise[n*n];
            int idx = (i+(int)tileAnimTime) % texParams.numTextures;
            NoiseSample *prev = noises[idx];
            NoiseSample *next = noises[(idx+1) % texParams.numTextures];
            for (int j = 0; j < n*n; ++j) {
                float t = frac(tileAnimTime);
                t = sin((t-0.5f)*PI)/2+0.5;
                noise[j] = lerp(t, prev[j], next[j]);
            }
            // set the generated noise's boundary to be equal to the boundary noise
            for (int j = 0; j < n; ++j) {
                /* [x][0]   */ noise[j] = boundary[j];
                /* [x][n-1] */ noise[j+n*(n-1)] = boundary[j+n];
                /* [0][y]   */ noise[j*n] = boundary[j*n];
                /* [n-1][y] */ noise[j*n+n-1] = boundary[j*n+n-1];
            }
            SDL_Surface *surface = generateSurface(noise, n, texParams.texSize);
            auto sdl = renderer->sdl();
            _textures.push_back(SDL_CreateTextureFromSurface(sdl, surface));
            SDL_FreeSurface(surface);
        }

        if (!isAnimating()) {
            log("Generated %d textures in %dms", _textures.size(), timer.elapsedMs());
        }
    }

    bool isAnimating() const {
        return texParams.gradAnimScale > 0 || texParams.noiseAnimScale > 0 || texParams.tileAnimScale > 0;
    }

    void update(float dt) {
        noiseAnimTime += texParams.noiseAnimScale*dt;
        gradAnimTime += texParams.gradAnimScale*dt;
        tileAnimTime += texParams.tileAnimScale*dt;
    }

    void reroll() {
        noiseAnimTime = 0;
        gradAnimTime = 0;
        tileAnimTime = 0;
        texParams.seed = rng.Int();
    }

    SDL_Texture* textureForIndex(int index) {
        // assert in case we get an underflowed `index` or something
        assert(_texIndices.size() < index+100'000,
            "don't generate more than 100,000 texture indices at a time pls");
        while (index >= _texIndices.size()) {
            _texIndices.push_back(rng.Int(_textures.size()));
        }
        return _textures[_texIndices[index]];
    }

    // debug view, visualize all generated textures
    void renderAtlas(Renderer* renderer, Vec2 pos) {
        int n = (int)ceil(sqrt(_textures.size()));
        for (int i = 0; i < _textures.size(); ++i) {
            float size = texParams.texSize;
            renderer->drawImage(_textures[i],
                pos + Vec2 {size*(i%n), size*(i/n)},
                {size, size});
        }
    }
};

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "ui.h"
#include "vec.h"

#include <math.h>
#include <random>
#include <string.h>
#include <vector>

#include <SDL2/SDL.h>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

const Vec2 screenSize { 1920, 1080 };
const float groundHeight = screenSize.y - 250;

struct Entity {
    Vec2 _pos;
    Vec2 _vel;
    Vec2 _size;

    Entity(Vec2 pos, Vec2 vel, Vec2 size)
        : _pos(pos), _vel(vel), _size(size) {
    }

    virtual void update(float dt) = 0;

    void render(Renderer* renderer) {
        renderer->drawRect(_pos, _size);
    }
};

struct Bullet : public Entity {
    float _lifespan;
    float _lived;

    Bullet(Vec2 pos, Vec2 vel, float lifespan)
        : Entity(pos, vel, {20, 20}),
        _lifespan(lifespan), _lived(0) {
    }

    void update(float dt) override {
        _pos += dt*_vel;
        _lived += dt;
    }
    bool shouldRemove() const {
        if (_lived >= _lifespan) {
            return true;
        }
        // if offscreen, remove early
        if (_pos.x < -_size.x/2 || _pos.x > screenSize.x + _size.x/2) {
            return true;
        }
        if (_pos.y < -_size.y/2 || _pos.y > screenSize.y + _size.y/2) {
            return true;
        }
        return false;
    }
};

struct Player : public Entity {
    bool _facingRight = true;
    bool _isOnGround = false;

    /// HACK: we set this in game->update, which is a terrible way to do this
    const Input* _input;
    Player() : Entity(
        /*pos*/ {300, 400},
        /*vel*/ {0, 0},
        /*size*/ {28, 52}) {
    }

    void update(float dt) override {
        float speed = 300;
        const float accel = 600;
        const float gravity = 2000;
        float jumpHeight = 150;

        float inDir = _input->getAxis("left", "right");
        if (inDir > 0) {
            _facingRight = true;
        } else if (inDir < 0) {
            _facingRight = false;
        }
        if (sign(inDir) != sign(_vel.x)) {
            inDir -= sign(_vel.x) * 1.5;
        }
        float delta = dt*accel * inDir;
        if (delta*_vel.x < 0 && abs(delta) > abs(_vel.x)) {
            _vel.x = 0;
        } else {
            _vel.x += delta;
        }
        _vel.x = clamp(_vel.x, -speed, speed);
        
        _vel.y += gravity*dt;
        if (_input->didPress("jump") && _isOnGround) {
            _vel.y = -sqrt(2*gravity*jumpHeight);
            _isOnGround = false;
        }
        if (_input->didRelease("jump") && _vel.y < 0) {
            _vel.y = 0.3*_vel.y;
        }

        _pos += dt*_vel;
        if (_pos.y + _size.y > groundHeight) {
            _pos.y = groundHeight - _size.y;
            _vel.y = 0;
            _isOnGround = true;
        }
    }
};

struct Program {
    Allocator* _allocator;
    SDL_Window* _window;
    Renderer* _renderer;
    Input _input;
    float t = 0.0;
    std::vector<SDL_Texture*> _textures;
    bool _quit = false;
    UI _ui;
    bool _showUI = false;

    std::ranlux24_base _rand_engine;
    // latest seed used to generate textures
    Uint32 _textureSeed;

    Player _player;
    std::vector<Bullet> _bullets;
    std::vector<int> _texIndices;

    Program(Allocator* allocator) : _allocator(allocator), _ui(_allocator, &_input) {
        _window = SDL_CreateWindow(
            "This Game Is My Second Chance",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            screenSize.x, screenSize.y,
            SDL_WINDOW_SHOWN);
        assert_SDL(_window, "window creation failed");

        SDL_Surface* screen = SDL_GetWindowSurface(_window);
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
        SDL_UpdateWindowSurface(_window);

        SDL_Renderer* renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
        assert_SDL(renderer, "renderer creation failed");
        /// HACK: we should probably allocate _renderer directly as a struct member
        /// we heap-allocate it because it's easier than dealing with constructor shenanigans
        _renderer = (Renderer*)_allocator->calloc(1, sizeof(Renderer));
        new (_renderer) Renderer(renderer);
    }

    ~Program() {
        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
        SDL_DestroyRenderer(_renderer->sdl());
        SDL_DestroyWindow(_window);

        _allocator->free(_renderer);
    }

    // data per grid cell
    struct NoiseSample {
        float v; // scalar noise value
        Color color; // RGB color data
        Vec2f pos; // offset within the grid cell, expected to be in [0, 1)
    };

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
        int mode = 3;
        int noiseScale = 2; // range of the scalar noise values
        int texSize = 32; // NxN pixel size of generated texture
    } texParams;

    // params affecting display
    int renderSize = 1024; // NxN size of total display area on screen
    int gridSize = 16; // render an NxN grid of textures

    /// @brief Called before unloading the dll. Clear any state that can't be
    /// persisted across reloads.
    void onUnload() {
        _ui.unload();
    }

    /// @brief Called after loading the dll, and on each reload.
    /// Useful for iterating configs at the moment
    void onLoad() {
        std::random_device r;
        // random_device seems to give a consistent value in dll, so offset by
        // ticks elapsed since program start to get new results on each reload
        _textureSeed = r();
        _rand_engine = std::ranlux24_base(_textureSeed);

        _input.addKeybind("quit", SDLK_ESCAPE);
        _input.addKeybind("reload", SDLK_r);
        _input.addKeybind("logging", SDLK_l);

        _input.addKeybind("left", SDLK_a);
        _input.addKeybind("right", SDLK_d);
        _input.addKeybind("up", SDLK_w);
        _input.addKeybind("down", SDLK_s);
        _input.addKeybind("jump", SDLK_k);
        _input.addKeybind("shoot", SDLK_j);

        _input.addKeybind("1", SDLK_1);
        _input.addKeybind("2", SDLK_2);
        _input.addKeybind("3", SDLK_3);
        _input.addKeybind("4", SDLK_4);
        _input.addKeybind("5", SDLK_5);

        _input.addMouseBind("click", 1);
        _input.addMouseBind("rclick", 3);

        generateTextures();
    }

    bool shouldQuit() {
        return _quit;
    }

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

    void generateTextures() {
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
                noise[j].color.g = noise[j].color.r; noise[j].color.b = noise[j].color.r;
                noise[j+n*(n-1)].color.g = noise[j+n*(n-1)].color.r; noise[j+n*(n-1)].color.b = noise[j+n*(n-1)].color.r;
                noise[j*n].color.g = noise[j*n].color.r; noise[j*n].color.b = noise[j*n].color.r;
                noise[j*n+n-1].color.g = noise[j*n+n-1].color.r; noise[j*n+n-1].color.b = noise[j*n+n-1].color.r;
            }
            SDL_Surface *surface = generateSurface(noise, n, texParams.texSize);
            auto sdl = _renderer->sdl();
            _textures.push_back(SDL_CreateTextureFromSurface(sdl, surface));
            SDL_FreeSurface(surface);
        }

        log("Generated %d textures in %dms", _textures.size(), timer.elapsedMs());
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
                                *pixel = {Uint8(cc), Uint8(cc), Uint8(cc), 0xff};
                            } else if (texParams.mode == 4) {
                                if ((cc/0x100) % 2 == 0) {
                                    cc = (0xff - cc%0x100);
                                } else {
                                    cc %= 0x100;
                                }
                                const float r = 360.0f * (9/43.0f);
                                *pixel = (cc/255.0f) * hsvColor(r*_textures.size(), 1, 1);
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

    void update(float dt) {
        int stackval = 0;
        Tracer trace("Game::update");
        trace("stack addr: %x (val=%d)", &stackval, stackval);

        _input.update();

        trace("input handling");
        if (_input.didPress("quit")) {
            trace("quitting");
            _quit = true;
            return;
        }

        if (_input.didPress("shoot")) {
            Vec2 vel { (_player._facingRight ? 1.0f : -1.0f) * 2000.0f, 0.0f };
            Bullet bullet {_player._pos, vel, 1.5};
            _bullets.push_back(bullet);
        }

        trace("ui update");
        updateUI();

        trace("bullet removal");
        int numToRemove = 0;
        for (int i = _bullets.size() - 1; i >= 0; --i) {
            auto& bullet = _bullets[i];
            bullet.update(dt);
            if (bullet.shouldRemove()) {
                // handle removal by swapping each bullet-to-remove to the end of the list
                ++numToRemove;
                std::swap(_bullets[i], _bullets[_bullets.size() - numToRemove]);
            }
        }
        for (int i = 0; i < numToRemove; ++i) {
            _bullets.pop_back();
        }

        _player._input = &_input;
        _player.update(dt);
    }

    void uiParam(const char* text, int &val, int lo, int hi) {
        _ui.line();
        _ui.labels(text, ": ");
        int set = val;
        if (_ui.button("<")) {
            set = lo;
        }
        _ui.label(val);
        if (_ui.button(">")) {
            set = hi;
        }
        if (set != val) {
            val = set;
            generateTextures();
        }
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

    void updateUI() {
        _ui.startUpdate({ 30, 30 });
        uiToggle(_showUI, "hide", "show");
        if (!_showUI) {
            return;
        }

        _ui.line();
        if (_ui.button("reroll")) {
            _textureSeed = _rand_engine();
            generateTextures();
        }

        uiParam("noise size", texParams.noiseSize,
            max(3, texParams.noiseSize-1), texParams.noiseSize+1);
        uiParam("num textures", texParams.numTextures,
            max(1, texParams.numTextures-1), texParams.numTextures+1);
        const int nModes = 5;
        uiParam("mode", texParams.mode,
            (texParams.mode+nModes-1) % nModes, (texParams.mode+1) % nModes);
        uiParam("noise scale", texParams.noiseScale,
            max(1, texParams.noiseScale-1), texParams.noiseScale+1);
        uiParam("tex size", texParams.texSize,
            max(4, texParams.texSize/2), min(4096, texParams.texSize*2));
        uiParam("grid size", gridSize,
            max(1, gridSize/2), min(64, gridSize*2));
    }

    void render() {
        Tracer trace("Game::render");

        _renderer->setColor(0.3, 0.2, 0.1, 1);
        _renderer->drawRect(0, groundHeight, screenSize.x, groundHeight);

        int rep = gridSize;
        float tileSize = renderSize / rep;
        for (int i = 0; i < rep; ++i) {
            for (int j = 0; j < rep; ++j) {
                int idxIdx = i*rep + j;
                while (idxIdx >= _texIndices.size()) {
                    _texIndices.push_back(randInt(_textures.size()));
                }
                _renderer->drawImage(_textures[_texIndices[idxIdx]],
                    800 + tileSize*i, 40 + tileSize*j,
                    tileSize, tileSize);
            }
        }

        int n = (int)ceil(sqrt(_textures.size()));
        for (int i = 0; i < _textures.size(); ++i) {
            float size = texParams.texSize;
            _renderer->drawImage(_textures[i],
                {760-size*(n+0.5f - i%n), screenSize.y-size*(n+0.5f - i/n)},
                {size, size});
        }

        _renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(_renderer);
        }

        _renderer->setColor(0, 1, 1, 1);
        _player.render(_renderer);

        _ui.render(_renderer);

        // only trace for one frame per reload to minimize spam
        trace("end"); // we're about to disable tracing so, make it match lol
        debug_isTracing = false;
    }
};

extern "C" {

__declspec(dllexport)
Program* newGame(Allocator* allocator) {
    Program* game = (Program*)allocator->calloc(1, sizeof(Program));
    new (game) Program(allocator);
    return game;
}
__declspec(dllexport)
void freeGame(Program* game, Allocator* allocator) {
    game->~Program();
    allocator->free(game);
}

__declspec(dllexport)
void onUnload(Program* game) {
    game->onUnload();
}

__declspec(dllexport)
void onLoad(Program* game) {
    game->onLoad();
}

__declspec(dllexport)
bool shouldQuit(Program* game) {
    return game->shouldQuit();
}

__declspec(dllexport)
void update(Program* game, float dt) {
    game->t += dt;
    game->update(dt);
}

__declspec(dllexport)
const void renderScene(Program* game) {
    SDL_Renderer* renderer = game->_renderer->sdl();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
    // SDL_RenderDrawRect(renderer, nullptr);
    SDL_RenderClear(renderer);

    game->render();

    SDL_RenderPresent(renderer);
}

} // extern "C"

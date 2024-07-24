#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "vec.h"

#include <math.h>
#include <random>
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

struct Color {
    Uint8 r, g, b, a;

    Color operator+(Color c) const {
        return {Uint8(r+c.r), Uint8(g+c.g), Uint8(b+c.b), Uint8(a+c.a)};
    }
    Color operator*(float s) const {
        return {Uint8(r*s), Uint8(g*s), Uint8(b*s), Uint8(a*s)};
    }
};
Color operator*(float s, Color c) {
    return c*s;
}

struct Game {
    SDL_Window* _window;
    Renderer* _renderer;
    Input _input;
    int gameMode = 0;
    float t = 0.0;
    std::vector<SDL_Texture*> _textures;
    bool _quit = false;

    Player _player;
    std::vector<Bullet> _bullets;

    Game(void* (*_calloc)(size_t, size_t)) {
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
        _renderer = (Renderer*)_calloc(1, sizeof(Renderer));
        new (_renderer) Renderer(renderer);
    }

    ~Game() {
        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
        SDL_DestroyRenderer(_renderer->sdl());
        SDL_DestroyWindow(_window);

        // _free(_renderer);
        // just leak it for now, should only be when exiting the program so nbd
    }

    std::mt19937 _rand_engine;

    /// @brief Called after loading the dll, and on each reload.
    /// Useful for iterating configs at the moment
    void onLoad() {
        std::random_device r;
        std::seed_seq seed {r(), r(), r(), r(), r(), r(), r(), r()};
        _rand_engine = std::mt19937(seed);

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

    struct NoiseSample {
        float v;
        Color color;
        Vec2f pos;
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

    std::vector<int> _texIndices;
    void generateTextures() {
        Tracer("Game::generateTextures");
        // note that reloading the dll gives us a new rand seed each time
        // which on reflection should be properly alarming
        // srand(21337);

        for (auto tex : _textures) {
            SDL_DestroyTexture(tex);
        }
        _textures.clear();
        _texIndices.clear();
        // noise width/height
        int n = 9;
        NoiseSample boundary[n*n];
        generateNoise(boundary, n);

        int numTextures = 16;
        for (int i = 0; i < numTextures; ++i) {
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
            int texSize = 64;
            SDL_Surface *surface = generateSurface(noise, n, texSize, 2);
            auto sdl = _renderer->sdl();
            _textures.push_back(SDL_CreateTextureFromSurface(sdl, surface));
            SDL_FreeSurface(surface);
        }
    }

    SDL_Surface* generateSurface(NoiseSample* noise, int n, int texSize, int xx) {
        int bpp = 32;
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
                        int mode = 2;
                        Color *pixel = (Color*)((Uint8*)surface->pixels
                            + j*surface->pitch
                            + i*surface->format->BytesPerPixel);
                        if (mode == 0) {
                            *pixel = ul_s.color;
                        } else if (mode == 1) {
                            Color a = lerp(uv.x, ul_s.color, ur_s.color);
                            Color b = lerp(uv.x, bl_s.color, br_s.color);
                            Color c = lerp(uv.y, a, b);
                            *pixel = c;
                        } else {
                            float a = lerp(uv.x, ul_s.v, ur_s.v);
                            float b = lerp(uv.x, bl_s.v, br_s.v);
                            float c = lerp(uv.y, a, b);
                            int cc = 0xff*(xx*c);
                            if (mode == 2) {
                                *pixel = {0, 0, 0, 0xff};
                                if (cc >= 0x200) {
                                    pixel->g = 0xff-(cc%0x100);
                                } else if (cc >= 0x100) {
                                    pixel->r = (cc%0x100);
                                } else {
                                    pixel->b = 0xff-(cc%0x100);
                                }
                            } else if (mode == 3) {
                                if ((cc/0x100) % 2 == 0) {
                                    cc = (0xff-cc) % 0x100;
                                }
                                *pixel = {Uint8(cc), Uint8(cc), Uint8(cc), 0xff};
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

        if (_input.didPress("1") || _input.didPress("rclick")) {
            generateTextures();
        }
        if (_input.didPress("click")) {
            Vec2 mouse = _input.getMousePos();
            Vec2 dir { randFloat(-1,1), randFloat(-1,1) };
            Bullet bullet {mouse, 1500.0f*dir, 1.5};
            _bullets.push_back(bullet);
        }

        if (_input.didPress("shoot")) {
            Vec2 vel { (_player._facingRight ? 1.0f : -1.0f) * 2000.0f, 0.0f };
            Bullet bullet {_player._pos, vel, 1.5};
            _bullets.push_back(bullet);
        }

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

    void render() {
        Tracer trace("Game::render");

        _renderer->setColor(1.0, 1, 0, 1.0);
        float x = sin(t*TAU*0.3) * 200 + 400;
        float y = cos(t*TAU*0.23) * 200 + 400;
        _renderer->drawRect(x, y, 60, 60);
        _renderer->drawBox(x+5, y+5, 50, 50);

        _renderer->setColor(0.0, 0.3, 1.0, 1.0);
        _renderer->drawRect(75, 150, 75, 100);

        _renderer->setColor(0.8, 0.1, 0.7, 1.0);
        _renderer->drawBox(
            sin(4.0*t)*300 + 350,
            cos(3.3*t)*100 + 150,
            40, 40);

        _renderer->drawText("HP: 1il/e1337", 40, 40);
        _renderer->drawText("Look at you", 226, 90);
        _renderer->drawText("maestro", 137, 244);

        _renderer->setColor(0.3, 0.2, 0.1, 1);
        _renderer->drawRect(0, groundHeight, screenSize.x, groundHeight);

        int texSize = 1024;
        int texRep = 8;
        int tileSize = texSize / texRep;
        for (int i = 0; i < texRep; ++i) {
            for (int j = 0; j < texRep; ++j) {
                int idxIdx = i*texRep + j;
                while (idxIdx >= _texIndices.size()) {
                    _texIndices.push_back(randInt(_textures.size()));
                }
                SDL_Rect destRect { 800 + tileSize*i, 40 + tileSize*j, tileSize, tileSize };
                SDL_RenderCopy(_renderer->sdl(), _textures[_texIndices[idxIdx]], nullptr, &destRect);
                char buffer[16];
                sprintf(buffer, "%d", _texIndices[idxIdx]);
                _renderer->drawText(buffer, destRect.x + 10, destRect.y + 10);
            }
        }

        _renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(_renderer);
        }

        _renderer->setColor(0, 1, 1, 1);
        _player.render(_renderer);

        _renderer->drawText("h3h3h3h3heh3h3", 1337, 42);

        // only trace for one frame per reload to minimize spam
        trace("end"); // we're about to disable tracing so, make it match lol
        debug_isTracing = false;
    }
};

extern "C" {

__declspec(dllexport)
Game* newGame(void* (*_calloc)(size_t, size_t)) {
    Game* game = (Game*)_calloc(1, sizeof(Game));
    new (game) Game(_calloc);
    return game;
}
__declspec(dllexport)
void freeGame(Game* game, void (*_free)(void*)) {
    game->~Game();
    _free(game);
}

__declspec(dllexport)
void onLoad(Game* game) {
    game->onLoad();
}

__declspec(dllexport)
bool shouldQuit(Game* game) {
    return game->shouldQuit();
}

__declspec(dllexport)
void update(Game* game, float dt) {
    game->t += dt;
    game->update(dt);
}

__declspec(dllexport)
const void renderScene(Game* game) {
    SDL_Renderer* renderer = game->_renderer->sdl();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
    // SDL_RenderDrawRect(renderer, nullptr);
    SDL_RenderClear(renderer);

    game->render();

    SDL_RenderPresent(renderer);
}

} // extern "C"

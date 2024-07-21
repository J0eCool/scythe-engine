#include "common.h"
#include "input.h"
#include "render_sdl.h"
#include "vec.h"

#include <math.h>
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
    int gameMode = 0;
    float t = 0.0;
    SDL_Texture *_texture = nullptr;

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
        SDL_DestroyTexture(_texture);
        SDL_DestroyRenderer(_renderer->sdl());
        SDL_DestroyWindow(_window);

        // _free(_renderer);
        // just leak it for now, should only be when exiting the program so nbd
    }

    float randFloat() {
        return (float)rand() / RAND_MAX;
    }
    float randFloat(float lo, float hi) {
        return lerp(randFloat(), lo, hi);
    }
    Uint8 randByte() {
        return rand() % 0x100;
    }
    /// @brief Generate a random `true` or `false` value
    /// @param p probability of a `true` result
    bool randBool(float p = 0.5) {
        return randFloat() < p;
    }

    void createTexture() {
        // srand(1337);
        auto sdl = _renderer->sdl();
        if (_texture) {
            SDL_DestroyTexture(_texture);
        }
        // noise width/height
        int nw = 8;
        int nh = 8;
        struct Sample {
            float v;
            Color color;
            Vec2f pos;
        };
        Sample noise[nh][nw]; // stored HxW for cache friendliness later (lol)
        for (int y = 0; y < nh; ++y) {
            for (int x = 0; x < nw; ++x) {
                noise[y][x] = {
                    randFloat(0.0, 1.0),
                    { randByte(), randByte(), randByte(), 0xff },
                    { randFloat(0.0, 1.0),
                      randFloat(0.0, 1.0) },
                };
            }
        }

        // image width/height
        int iw = 128;
        int ih = 128;
        int bpp = 32;
        SDL_Surface *surface = SDL_CreateRGBSurface(
            0, iw, ih, bpp,
            // RGBA bitmasks; A mask is special
            0xff, 0xff << 8, 0xff << 16, 0);
        Vec2f ntoi { float(iw)/nw, float(ih)/nh };
        Vec2f iton { float(nw)/iw, float(nh)/ih };
        // for each noise cell
        for (int y = -1; y < nh; ++y) {
            for (int x = -1; x < nw; ++x) {
                // sample each corner 
                // _i for index
                // _n for noise-coord position
                Vec2f p_n = {float(x), float(y)};
                Vec2i ul_i = Vec2i{x+0, y+0};
                Vec2i ur_i = Vec2i{x+0, y+1};
                Vec2i bl_i = Vec2i{x+1, y+0};
                Vec2i br_i = Vec2i{x+1, y+1};
                Sample ul_s = noise[(ul_i.y+nh) % nh][(ul_i.x+nw) % nw];
                Sample ur_s = noise[(ur_i.y+nh) % nh][(ur_i.x+nw) % nw];
                Sample bl_s = noise[(bl_i.y+nh) % nh][(bl_i.x+nw) % nw];
                Sample br_s = noise[(br_i.y+nh) % nh][(br_i.x+nw) % nw];
                Vec2f ul = ul_i.to<float>() + ul_s.pos;
                Vec2f ur = ur_i.to<float>() + ur_s.pos;
                Vec2f bl = bl_i.to<float>() + bl_s.pos;
                Vec2f br = br_i.to<float>() + br_s.pos;
                Vec2i bound_lo = floorv(ntoi*min(ul, min(ur, bl)));
                Vec2i bound_hi =  ceilv(ntoi*max(br, max(ur, bl)));
                assert(bound_lo.x <= bound_hi.x, "invalid noise bounds");
                assert(bound_lo.y <= bound_hi.y, "invalid noise bounds");

                // iterate over the pixels in the AABB
                for (int j = max(0, bound_lo.y); j < min(ih, bound_hi.y); ++j) {
                    for (int i = max(0, bound_lo.x); i < min(iw, bound_hi.x); ++i) {
                        Vec2 p = iton * Vec2 { float(i), float(j) };

                        // convert to UV coordinates inside the quad
                        // if either U or V is out of [0, 1], then we're not inside the quad
                        // https://iquilezles.org/articles/ibilinear/

                        Vec2 e = ur-ul;
                        Vec2 f = bl-ul;
                        Vec2 g = ul-ur+br-bl;
                        Vec2 h = p - ul;
                        // float k2 = g.x*f.y - g.y*f.x;
                        // float k1 = e.x*f.y - e.y*f.x + h.x*g.y - h.y*g.x;
                        // float k0 = h.x*e.y - h.y*e.x;
                        float k2 = g.cross(f);
                        float k1 = e.cross(f) + h.cross(g);
                        float k0 = h.cross(e);
                        float disc = k1*k1 - 4*k0*k2;
                        Color *pixel = (Color*)((Uint8*)surface->pixels
                            + j*surface->pitch
                            + i*surface->format->BytesPerPixel);
                        if (abs(k2) < 0.001) {
                            // if perfectly parallel
                            /// TODO: linear interpolation
                            *pixel = {0xff, 0x00, 0xff, 0xff};
                            continue;
                        }
                        if (disc < 0) {
                            continue;
                        }

                        float v = (-k1 - sqrt(disc))/(2*k2);
                        float u = (h.x - f.x*v)/(e.x + g.x*v);
                        if (u < 0 || u > 1 || v < 0 || v > 1) {
                            v = (-k1 + sqrt(disc))/(2*k2);
                            u = (h.x - f.x*v)/(e.x + g.x*v);
                        }
                        if (u < 0 || u > 1 || v < 0 || v > 1) {
                            // *pixel = {0xff, 0x00, 0x00, 0xff};
                            continue;
                        }

                        bool doColor = false;

                        if (doColor) {
                            Color a = lerp(u, ul_s.color, ur_s.color);
                            Color b = lerp(u, bl_s.color, br_s.color);
                            Color c = lerp(v, a, b);
                            *pixel = c;
                        } else {
                            float a = lerp(u, ul_s.v, ur_s.v);
                            float b = lerp(u, bl_s.v, br_s.v);
                            float c = lerp(v, a, b);
                            int cc = 0xff*(1*c);
                            *pixel = {0, 0, 0, 0xff};
                            if (cc >= 0x200) {
                                pixel->g = 0xff-(cc%0x100);
                            } else if (cc >= 0x100) {
                                pixel->r = (cc%0x100);
                            } else {
                                pixel->b = 0xff-(cc%0x100);
                            }
                            *pixel = {Uint8(cc), Uint8(cc), Uint8(cc), 0xff};
                        }
                    }
                }
            }
        }
        _texture = SDL_CreateTextureFromSurface(sdl, surface);
        SDL_FreeSurface(surface);
    }

    void update(float dt, const Input* input) {
        static bool justLoaded = true;
        if (justLoaded) {
            // detects when the dll was reloaded
            // ... we may want to explicitly model this as a function but, details
            justLoaded = false;
            createTexture();
        }

        if (input->didPress("1")) {
            createTexture();
        }

        if (input->didPress("shoot")) {
            Vec2 vel { (_player._facingRight ? 1.0f : -1.0f) * 2000.0f, 0.0f };
            Bullet bullet {_player._pos, vel, 1.5};
            _bullets.push_back(bullet);
        }
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

        _player._input = input;
        _player.update(dt);
    }
    void render() {
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

        _renderer->drawText("You are big silly", 40, 40);
        _renderer->drawText("Look at you", 226, 90);
        _renderer->drawText("maestro", 137, 244);

        _renderer->setColor(0.3, 0.2, 0.1, 1);
        _renderer->drawRect(0, groundHeight, screenSize.x, groundHeight);

        int texSize = 256;
        int texRep = 4;
        for (int i = 0; i < texRep; ++i) {
            for (int j = 0; j < texRep; ++j) {
                SDL_Rect destRect { 800 + texSize*i, 40 + texSize*j, texSize, texSize };
                SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);
            }
        }

        _renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(_renderer);
        }

        _renderer->setColor(0, 1, 1, 1);
        _player.render(_renderer);
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
void quitGame(Game* game, void (*_free)(void*)) {
    game->~Game();
    _free(game);
}

__declspec(dllexport)
void update(Game* game, float dt, const Input* input) {
    game->t += dt;
    game->update(dt, input);
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

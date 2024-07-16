#include "common.h"
#include "input.h"
#include "render_sdl.h"
#include "vec.h"

#include <math.h>
#include <vector>

#include <SDL2/SDL.h>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

const Vec2 screenSize { 800, 600 };
const float groundHeight = 450;

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

struct Game {
    SDL_Window* _window;
    Renderer* _renderer;
    int gameMode = 0;
    float t = 0.0;
    SDL_Texture *_texture = nullptr;

    Player _player;
    std::vector<Bullet> _bullets;

    const int screenWidth = 800;
    const int screenHeight = 600;
    Game(void* (*_calloc)(size_t, size_t)) {
        _window = SDL_CreateWindow(
            "This Game Is My Second Chance",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            screenWidth, screenHeight,
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
    // idk if this is useful anywhere else, or what to call it
    float lerp_between(float x, float a, float b, float lo, float hi) {
        float t = (x - a) / (b - a);
        return lerp(t, lo, hi);
    }
    void createTexture() {
        // srand(1337);
        auto sdl = _renderer->sdl();
        if (_texture) {
            SDL_DestroyTexture(_texture);
        }
        // noise width/height
        int nw = 4;
        int nh = 4;
        struct Sample {
            // value, x-offset, y-offset
            float v, x, y;
        };
        Sample noise[nh][nw]; // stored HxW for cache friendliness later (lol)
        for (int y = 0; y < nh; ++y) {
            for (int x = 0; x < nw; ++x) {
                noise[y][x] = {
                    randFloat(0.0, 1.0),
                    // todo: reenable position scattering
                    randFloat(0.0, 0.0),
                    randFloat(0.0, 0.0),
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
        struct Color {
            Uint8 r, g, b, a;
        };
        for (int y = 0; y < ih; ++y) {
            for (int x = 0; x < iw; ++x) {
                Color *pixel = (Color*)((Uint8*)surface->pixels
                    + y*surface->pitch
                    + x*surface->format->BytesPerPixel);
                // raw position in noise-space
                Vec2 t0 { float(x)*nw/iw, float(y)*nh/ih };
                // sample indices
                Vec2i n00 = Vec2i{ (int)t0.x, (int)t0.y };
                Vec2 t = t0 - n00.to<float>();
                Sample s00 = noise[n00.y][n00.x];
                // note: in general anything using sample x/y is half-baked
                // it mostly works, but there are artifacts; deal w/ it later
                if (s00.x > t.x) {
                    n00.x = (n00.x-1 + nw) % nw;
                    s00 = noise[n00.y][n00.x];
                    t = t0 - n00.to<float>();
                }
                if (s00.y > t.y) {
                    n00.y = (n00.y-1 + nh) % nh;
                    s00 = noise[n00.y][n00.x];
                    t = t0 - n00.to<float>();
                }
                Vec2i n01 = (n00 + Vec2i{0, 1}) % Vec2i{nw, nh};
                Vec2i n10 = (n00 + Vec2i{1, 0}) % Vec2i{nw, nh};
                Vec2i n11 = (n00 + Vec2i{1, 1}) % Vec2i{nw, nh};
                Sample s01 = noise[n01.y][n01.x];
                Sample s10 = noise[n10.y][n10.x];
                Sample s11 = noise[n11.y][n11.x];
                float a = lerp_between(t0.x, n00.x+s00.x, n10.x+s10.x, s00.v, s10.v);
                float b = lerp_between(t0.x, n01.x+s01.x, n11.x+s11.x, s01.v, s11.v);
                Uint8 c = 0xff*lerp_between(t0.y, n00.y+s00.y, n01.y+s01.y, a, b);
                // Vec2i xx = (float(iw/nw)*(Vec2f{n00.x+s00.x, n00.y+s00.y})).to<int>();
                // Uint8 c = ((xx.x == x) && (xx.y == y)) ? 0xff : 0x00;
                *pixel = { c, c, c, 0xff };
                // *pixel = {Uint8(0xff*s00.x), Uint8(0xff*s00.y), 0, 0xff};
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
        // texture funsies
        SDL_Rect destRect { 400, 40, 3*128, 3*128 };
        SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);

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

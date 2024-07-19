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
        struct Color {
            Uint8 r, g, b, a;
        };
        // noise width/height
        int nw = 4;
        int nh = 4;
        struct Sample {
            // value, x-offset, y-offset
            Color color;
            // float v;
            Vec2f pos;
        };
        Sample noise[nh][nw]; // stored HxW for cache friendliness later (lol)
        for (int y = 0; y < nh; ++y) {
            for (int x = 0; x < nw; ++x) {
                noise[y][x] = {
                    {randByte(), randByte(), randByte(), 0xff},
                    { randFloat(0.0, 1.0),
                      randFloat(0.0, 1.0) },
                };
            }
        }

        // image width/height
        int iw = 512;
        int ih = 512;
        int bpp = 32;
        SDL_Surface *surface = SDL_CreateRGBSurface(
            0, iw, ih, bpp,
            // RGBA bitmasks; A mask is special
            0xff, 0xff << 8, 0xff << 16, 0);
        for (int y = 0; y < ih; ++y) {
            for (int x = 0; x < iw; ++x) {
                // raw position in noise-space
                Vec2 p { float(x)*nw/iw, float(y)*nh/ih };

                // sample the current noise cell
                Vec2i a_i = Vec2i{ (int)p.x, (int)p.y };
                Sample a_s = noise[a_i.y][a_i.x];
                Vec2 a = a_i.to<float>() + a_s.pos;

                // next we need to determine which quadrant of the cell we're in
                // b,c,d,e are right,down,left,up cells, 
                Vec2i n_bounds = Vec2i { nw, nh };
                Vec2i b_i = (a_i + Vec2i{1, 0});
                Vec2i c_i = (a_i + Vec2i{0, 1});
                Vec2i d_i = (a_i - Vec2i{1, 0});
                Vec2i e_i = (a_i - Vec2i{0, 1});
                Sample b_s = noise[(b_i.y+nh) % nh][(b_i.x+nw) % nw];
                Sample c_s = noise[(c_i.y+nh) % nh][(c_i.x+nw) % nw];
                Sample d_s = noise[(d_i.y+nh) % nh][(d_i.x+nw) % nw];
                Sample e_s = noise[(e_i.y+nh) % nh][(e_i.x+nw) % nw];
                Vec2 b = b_i.to<float>() + b_s.pos;
                Vec2 c = c_i.to<float>() + c_s.pos;
                Vec2 d = d_i.to<float>() + d_s.pos;
                Vec2 e = e_i.to<float>() + e_s.pos;

                const int nColors = 7;
                Color colors[nColors] = {
                    { 0xff, 0x00, 0x00, 0xff },
                    { 0xff, 0xff, 0x00, 0xff },
                    { 0x00, 0xff, 0x00, 0xff },
                    { 0x00, 0xff, 0xff, 0xff },
                    { 0x00, 0x00, 0xff, 0xff },
                    { 0xff, 0x00, 0xff, 0xff },
                    { 0xff, 0xff, 0xff, 0xff },
                };

                // determine which quadrant we're in using cross prodcuts to compare angles
                // this doesn't actually work for the case of concave quads, which is possible
                // given that we're precomputing this in software, it's probably simpler and more accurate to just
                // iterate over the noise cells *directly*, so we'll try that next
                auto pa = p-a;
                auto ba = b-a;
                auto ca = c-a;
                auto da = d-a;
                auto ea = e-a;
                int i = a_i.x + (nw+1)*a_i.y;
                Sample ul, ur, bl, br;
                if (sign(ba.cross(pa)) == sign(pa.cross(ca)) && ba.cross(pa) >= 0) {
                    // bottom-right;
                    ul = noise[(a_i.y-0+nh) % nh][(a_i.x-0+nw) % nw];
                    // ul = a;
                } else if (sign(pa.cross(ca)) == sign(da.cross(pa)) && pa.cross(ca) < 0) {
                    // bottom-left
                    ul = noise[(a_i.y-0+nh) % nh][(a_i.x-1+nw) % nw];
                    // ul = d;
                } else if (sign(pa.cross(da)) == sign(ea.cross(pa)) && pa.cross(da) < 0) {
                    // top-left
                    ul = noise[(a_i.y-1+nh) % nh][(a_i.x-1+nw) % nw];
                } else {
                    // top-right
                    ul = noise[(a_i.y-1+nh) % nh][(a_i.x-0+nw) % nw];
                    // ul = a;
                }

                // finally, set the pixel color
                Color *pixel = (Color*)((Uint8*)surface->pixels
                    + y*surface->pitch
                    + x*surface->format->BytesPerPixel);
                *pixel = ul.color;
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

        int texSize = 512;
        SDL_Rect destRect { 800, 40, texSize, texSize };
        SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);
        destRect.x += texSize;
        SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);
        destRect.y += texSize;
        SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);
        destRect.x -= texSize;
        SDL_RenderCopy(_renderer->sdl(), _texture, nullptr, &destRect);

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

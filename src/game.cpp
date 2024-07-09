#include "common.h"
#include "input.h"
#include "render.h"
#include "vec.h"

#include <math.h>
#include <vector>

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
    int gameMode = 0;
    float t = 0.0;

    // mode 1
    Player _player;
    std::vector<Bullet> _bullets;

    // mode 2
    std::vector<std::string> _journal;

    Game() {
        _journal.push_back("");
    }

    void update(float dt, const Input* input) {
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
    void render(Renderer* renderer) {
        renderer->setColor(1.0, 1, 0, 1.0);
        float x = sin(t*TAU*0.3) * 200 + 400;
        float y = cos(t*TAU*0.23) * 200 + 400;
        renderer->drawRect(x, y, 60, 60);
        renderer->drawBox(x+5, y+5, 50, 50);

        renderer->setColor(0.0, 0.3, 1.0, 1.0);
        renderer->drawRect(75, 150, 75, 100);

        renderer->setColor(0.8, 0.1, 0.7, 1.0);
        renderer->drawBox(
            sin(4.0*t)*300 + 350,
            cos(3.3*t)*100 + 150,
            40, 40);

        renderer->drawText("You are big stupid", 40, 40);
        renderer->drawText("Look at you", 226, 90);
        renderer->drawText("monster", 137, 244);

        renderer->setColor(0.3, 0.2, 0.1, 1);
        renderer->drawRect(0, groundHeight, screenSize.x, groundHeight);

        renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(renderer);
        }

        renderer->setColor(0, 1, 1, 1);
        _player.render(renderer);
    }
};

extern "C" {

__declspec(dllexport)
Game* newGame(void* (*_calloc)(size_t, size_t)) {
    Game* game = (Game*)_calloc(1, sizeof(Game));
    new (game) Game;
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
const void renderScene(Game* game, Renderer* renderer) {
    game->render(renderer);
}

} // extern "C"

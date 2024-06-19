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
        const float gravity = 3000;
        float jumpHeight = 250;

        _vel.x = speed * _input->getAxis("left", "right");
        if (_vel.x > 0) {
            _facingRight = true;
        } else if (_vel.x < 0) {
            _facingRight = false;
        }
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
    float t = 0.0;
    Player player;
    std::vector<Bullet> bullets;
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

    if (input->didPress("shoot")) {
        Vec2 vel { (game->player._facingRight ? 1.0f : -1.0f) * 1500.0f, 0.0f };
        Bullet bullet {game->player._pos, vel, 1.5};
        game->bullets.push_back(bullet);
        printf("firing, now at %d bullets\n", game->bullets.size());
    }
    int numToRemove = 0;
    for (int i = game->bullets.size() - 1; i >= 0; --i) {
        auto& bullet = game->bullets[i];
        bullet.update(dt);
        if (bullet.shouldRemove()) {
            // handle removal by swapping each bullet-to-remove to the end of the list
            ++numToRemove;
            std::swap(game->bullets[i], game->bullets[game->bullets.size() - numToRemove]);
        }
    }
    for (int i = 0; i < numToRemove; ++i) {
        game->bullets.pop_back();
    }

    game->player._input = input;
    game->player.update(dt);
}

__declspec(dllexport)
const void renderScene(Game* game, Renderer* renderer) {
    auto t = game->t;
    renderer->setColor(1.0, 1, 0, 1.0);
    float x = sin(t*TAU*0.3) * 200 + 400;
    float y = cos(t*TAU*0.23) * 200 + 400;
    renderer->drawRect(x, y, 60, 60);
    renderer->drawBox(x+5, y+5, 50, 50);

    renderer->setColor(0.0, 0.3, 1.0, 1.0);
    renderer->drawRect(75, 150, 75, 100);

    renderer->setColor(0.8, 0.1, 0.7, 1.0);
    renderer->drawBox(
        sin(4*t)*300 + 350,
        cos(3.3*t)*100 + 150,
        40, 40);

    renderer->drawText("Wow Dang", 40, 40);

    renderer->setColor(0.3, 0.2, 0.1, 1);
    renderer->drawRect(0, groundHeight, screenSize.x, groundHeight);

    renderer->setColor(1, 1, 0, 1);
    for (auto& bullet : game->bullets) {
        bullet.render(renderer);
    }

    renderer->setColor(0, 1, 1, 1);
    game->player.render(renderer);
}

} // extern "C"

#include "input.h"
#include "render.h"
#include "vec.h"

#include <math.h>
#include <vector>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

struct Bullet {
    Vec2 _pos;
    Vec2 _vel;
    Vec2 _size;
    float _lifespan;
    float _lived;

    Bullet(Vec2 pos, Vec2 vel, float lifespan)
        : _pos(pos), _vel(vel), _size{20, 20},
        _lifespan(lifespan), _lived(0) {
    }

    void update(float dt) {
        _pos += dt*_vel;
        _lived += dt;
    }
    bool shouldRemove() const {
        if (_lived >= _lifespan) {
            return true;
        }
        // if offscreen, remove early
        Vec2 screen { 800, 600 };
        if (_pos.x < -_size.x || _pos.x > screen.x) {
            return true;
        }
        return false;
    }
};

struct Game {
    float t = 0.0;
    Vec2 pos { 300, 200 };
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

    float speed = 250;
    Vec2 move {
        input->getAxis("left", "right"),
        input->getAxis("up", "down"),
    };
    move = move.normalized();
    game->pos += dt*speed * move;
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

    renderer->drawText("Wow Dang", 40, 500);

    renderer->setColor(1, 1, 0, 1);
    renderer->drawRect(game->pos.x - 15, game->pos.y - 15, 30, 30);
}

} // extern "C"

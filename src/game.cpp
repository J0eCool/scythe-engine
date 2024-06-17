#include "input.h"
#include "render.h"
#include "vec.h"

#include <math.h>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

struct Game {
    float t = 0.0;
    Input* input;
};

extern "C" {

__declspec(dllexport)
Game* newGame(Input* input, void* (*_calloc)(size_t, size_t)) {
    Game* game = (Game*)_calloc(1, sizeof(Game));
    new (game) Game;
    game->input = input;
    return game;
}
__declspec(dllexport)
void quitGame(Game* game, void (*_free)(void*)) {
    game->~Game();
    _free(game);
}

__declspec(dllexport)
void update(Game* game, float dt) {
    game->t += dt;
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

    Vec2 pos = game->input->getMousePos();
    renderer->setColor(1, 1, 0, 1);
    renderer->drawRect(pos.x - 15, pos.x - 15, 30, 30);
}

} // extern "C"

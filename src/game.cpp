#include "render.h"

#include <math.h>

extern "C" {

float t = 0.0;

__declspec(dllexport)
void update(float dt) {
    t += dt;
}

__declspec(dllexport)
const void renderScene(Renderer* renderer) {
    renderer->setColor(0.0, 0.3, 1.0, 1.0);
    renderer->drawRect(75, 150, 75, 100);

    renderer->setColor(0.8, 0.1, 0.7, 1.0);
    renderer->drawBox(
        sin(4*t)*300 + 350,
        cos(3.3*t)*100+150,
        40, 40);
}

} // extern "C"

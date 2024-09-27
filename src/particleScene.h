#pragma once

#include "color.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "rng.h"
#include "scene.h"
#include "ui.h"
#include "vec.h"

#include <vector>

struct Particle {
    Vec2 pos;
    Vec2 vel;
    float lifetime;

    float age;

    void update(float dt);
};

class ParticleScene : public Scene {
    UI _ui;
    Input* _input;
    Rng _rng;

    std::vector<Particle> _particles;

    Color bgColor {0x10, 0x20, 0x40};

    bool _shouldGenerate = true;
    struct Params {
        int numParticles = 512;
        float duration = 0.3;
        float speed = 500;
    } _params;

    float _emitTimer = 0.0;

public:
    ParticleScene(Allocator *alloc, Input *input); 

    void update(float dt) override;
    void render(Renderer *renderer) override;

private:
    void createParticles();
};

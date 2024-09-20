#include "particleScene.h"

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "serialize.h"
#include "ui.h"
#include "uilib.h"
#include "vec.h"

#include <math.h>
#include <vector>

/// @brief Deletes an element from an array in O(1) time without preserving order
template <typename T>
void quickRemove(std::vector<T> &elems, size_t idx) {
    // this probably wants to live in common.h or smth
    std::swap(elems[idx], elems[elems.size()-1]);
    elems.pop_back();
}

void Particle::update(float dt) {
    pos += vel*dt;
    age += dt;
}

ParticleScene::ParticleScene(Allocator *alloc, Input *input) :
        _input(input), _ui(alloc, input) {
}

void ParticleScene::update(float dt) {
    _ui.startUpdate({ 120, 30 });
    if (_ui.button("boop")) {
        createParticles();
    }
    _ui.labels((int)_particles.size(), "\n");

    uiParam(_ui, "num", _params.numParticles, 1, 1, 1000);

    std::vector<int> toRemove;
    for (int i = 0; i < _particles.size(); ++i) {
        auto &p = _particles[i];
        p.update(dt);
        if (p.age >= p.lifetime) {
            toRemove.push_back(i);
        }
    }
    for (int i = toRemove.size()-1; i >= 0; --i) {
        quickRemove(_particles, toRemove[i]);
    }
}

void ParticleScene::render(Renderer *renderer) {
    renderer->background(bgColor);

    renderer->setColor(1.0, 0.23, 0.05);
    for (auto p : _particles) {
        renderer->drawRect(p.pos, {10, 10});
    }

    _ui.render(renderer);
}

void ParticleScene::createParticles() {
    for (int i = 0; i < _params.numParticles; ++i) {
        Vec2 pos { 900, 512 };
        float ang = TAU*i/_params.numParticles;
        Vec2 vel = Vec2 { cos(ang), sin(ang) } * 500;
        float life = 1.25;
        _particles.push_back({pos, vel, life, 0.0});
    }
}

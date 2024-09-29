#include "particleScene.h"

#include "common.h"
#include "uilib.h"

#include <math.h>

/// @brief Deletes an element from an array in O(1) time without preserving order
template <typename T>
void quickRemove(std::vector<T> &elems, size_t idx) {
    // this probably wants to live in common.h or smth
    std::swap(elems[idx], elems[elems.size()-1]);
    elems.pop_back();
}

void Particle::update(float dt) {
    vel.y += gravity*dt;
    pos += vel*dt;
    age += dt;
}

ParticleScene::ParticleScene(Allocator *alloc, Input *input) :
        _input(input), _ui(alloc, input) {
}

void ParticleScene::update(float dt) {
    _ui.startUpdate({ 120, 30 });
    if (_input->didPress("click")) {
        createParticles();
    }
    _ui.labels((int)_particles.size(), "\n");

    uiParam(_ui, "num", _params.numParticles, 1, 1, 1024);
    uiParam<float>(_ui, "duration", _params.duration, 0.1, 0.0, 5.0);
    uiParam<float>(_ui, "speed", _params.speed, 100, 0, 2500);
    uiParam<float>(_ui, "gravity", _params.gravity, 250, 0, 7500);
    uiParam<float>(_ui, "size", _params.size, 1, 0, 100);

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
        renderer->drawRect(p.pos, {p.size});
    }

    _ui.render(renderer);
}

void ParticleScene::createParticles() {
    int n = _params.numParticles;
    for (int i = 0; i < n; ++i) {
        Vec2 pos = _input->getMousePos();
        float ang = TAU*i/n;
        Particle p;
        p.pos = pos;
        p.vel = Vec2 { cos(ang), sin(ang) } * _params.speed * _rng.Normalish(0.5, 2.0);
        p.gravity = _params.gravity;
        p.lifetime = _params.duration;
        p.size = _params.size;
        _particles.push_back(p);
    }
}

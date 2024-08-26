// GameScene - actual gameplay logic

#pragma once

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "texGenScene.h"
#include "vec.h"

#include <math.h>
#include <vector>

const Vec2 screenSize { 1920, 1080 };
const float groundHeight = 250;
const float groundY = screenSize.y - groundHeight;

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

        float inDir = _input->getAxis("horizontal");
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
        if (_pos.y + _size.y > groundY) {
            _pos.y = groundY - _size.y;
            _vel.y = 0;
            _isOnGround = true;
        }
    }
};

class GameScene : public Scene {
    Player _player;
    std::vector<Bullet> _bullets;

    Input* _input;
    TexGenScene* _texScene;

public:
    GameScene(Input* input, TexGenScene* texScene) :
        _input(input), _texScene(texScene) {
    }

    void update(float dt) override {
        if (_input->didPress("shoot")) {
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

        _player._input = _input;
        _player.update(dt);
    }

    void render(Renderer* renderer) override {
        Vec2 tileSize = _texScene->tileSize();
        int w = ceil(screenSize.x/tileSize.x);
        int h = ceil(groundHeight/tileSize.y);
        for (int i = 0; i < w; ++i) {
            for (int j = 0; j < h; ++j) {
                renderer->drawImage(_texScene->textureForIndex(i + j*w),
                    {i*tileSize.x, j*tileSize.y + groundY},
                    tileSize);
            }
        }

        renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(renderer);
        }

        renderer->setColor(0, 1, 1, 1);
        _player.render(renderer);

        renderer->drawText("did everything go", 1100, 145);
        renderer->drawText("better than expected?", 1140, 190);
    }
};

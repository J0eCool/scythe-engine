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
    Vec3 _pos;
    Vec3 _vel;
    Vec2 _size;

    Entity(Vec3 pos, Vec3 vel, Vec2 size)
        : _pos(pos), _vel(vel), _size(size) {
    }

    virtual void update(float dt) = 0;

    virtual void render(Renderer* renderer) {
        renderer->drawRect(_pos.xy(), _size);
    }
};

struct Bullet : public Entity {
    float _lifespan;
    float _lived;

    Bullet(Vec3 pos, Vec3 vel, float lifespan)
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
        /*pos*/ {300, groundY},
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

        _vel.y = speed*_input->getAxis("vertical");
        
        _vel.z += gravity*dt;
        if (_input->didPress("jump") && _isOnGround) {
            _vel.z = -sqrt(2*gravity*jumpHeight);
            _isOnGround = false;
        }
        if (_input->didRelease("jump") && _vel.z < 0) {
            _vel.z = 0.3*_vel.z;
        }

        _pos += dt*_vel;
        if (_pos.z > 0) {
            _pos.z = 0;
            _vel.z = 0;
            _isOnGround = true;
        }
    }

    void render(Renderer* renderer) override {
        renderer->setColor(0.2, 0.2, 0.2, 1);
        float shadow = 10;
        renderer->drawRect(_pos.xy() + Vec2 {-shadow, _size.y-shadow},
            {_size.x + 2*shadow, 2*shadow});
        renderer->setColor(0, 1, 1, 1);
        renderer->drawRect({_pos.x, _pos.y+_pos.z}, _size);
    }
};

class GameScene : public Scene {
    Player _player;
    std::vector<Bullet> _bullets;

    Input* _input;
    TexGen* _texGen;
    TexGenScene* _texScene;

public:
    GameScene(Input* input, TexGen *texGen, TexGenScene* texScene) :
        _input(input), _texGen(texGen), _texScene(texScene) {
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
        int h = ceil(screenSize.y/tileSize.y);
        for (int i = 0; i < w; ++i) {
            for (int j = 0; j < h; ++j) {
                renderer->drawImage(_texGen->textureForIndex(i + j*w),
                    {i*tileSize.x, j*tileSize.y},
                    tileSize);
            }
        }

        renderer->setColor(1, 1, 0, 1);
        for (auto& bullet : _bullets) {
            bullet.render(renderer);
        }

        _player.render(renderer);

        renderer->drawText("Did everything go", 1100, 145);
        renderer->drawText("better than expected?", 1140, 190);
    }
};

#include "gameScene.h"

Entity::Entity(Vec3 pos, Vec3 vel, Vec2 size)
    : _pos(pos), _vel(vel), _size(size) {
}

/// @brief Helper function; projects worldspace to screenspace
Vec2 project(Vec3 v) {
    return { v.x, v.y*0.5f + v.z};
}

void Entity::render(Renderer* renderer) {
    renderer->drawRect(project(_pos), _size);
}

Bullet::Bullet(Vec3 pos, Vec3 vel, float lifespan)
    : Entity(pos, vel, {20, 20}),
    _lifespan(lifespan), _lived(0) {
}

void Bullet::update(float dt) {
    _pos += dt*_vel;
    _lived += dt;
}
bool Bullet::shouldRemove() const {
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

Player::Player() : Entity(
    /*pos*/ {300, groundY},
    /*vel*/ {0, 0},
    /*size*/ {28, 52}) {
}

void Player::update(float dt) {
    float speed = 300;
    const float accel = 600;
    const float gravity = 2000;
    float jumpHeight = 150;

    _spinT += dt;

    Vec3 inDir = {_input->getAxis("horizontal"), _input->getAxis("vertical"), 0};
    if (inDir.x > 0) {
        _facingRight = true;
    } else if (inDir.x < 0) {
        _facingRight = false;
    }
    // dampen movement when 

    const float dampening = 1.5;
    for (int i = 0; i < 2; ++i) {
        inDir[i] -= (sign(inDir[i]) != sign(_vel[i])) * sign(_vel[i])*dampening;

        Vec3 delta = dt*accel * inDir;
        if (delta.x*_vel[i] < 0 && abs(delta[i]) > abs(_vel[i])) {
            _vel[i] = 0;
        } else {
            _vel[i] += delta[i];
        }
        _vel[i] = clamp(_vel[i], -speed, speed);
    }
    
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

void Player::render(Renderer* renderer) {
    // shadow (draws first because it's in the background)
    renderer->setColor(0.2, 0.2, 0.2, 1);
    float shadow = 10;
    renderer->drawRect(project(_pos.xy()) + Vec2 {-shadow, _size.y-shadow},
        {_size.x + 2*shadow, 2*shadow});

    // player
    renderer->setColor(0, 1, 1, 1);
    renderer->drawRect(project(_pos), _size);

    // spinny widget
    renderer->setColor(1, 0, 1, 1);
    renderer->drawRect(project(widgetPos()), {25,25});
}

Vec3 Player::widgetPos() const {
    return _pos + 50.0f*Vec2::unit(0.15f*_spinT*TAU);
}

GameScene::GameScene(Input* input, TexGen *texGen, TexGenScene* texScene) :
    _input(input), _texGen(texGen), _texScene(texScene) {
}

void GameScene::update(float dt) {
    if (_input->didPress("shoot")) {
        Vec2 vel { (_player._facingRight ? 1.0f : -1.0f) * 2000.0f, 0.0f };
        Bullet bullet {_player.widgetPos(), vel, 1.5};
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

void GameScene::render(Renderer* renderer) {
    Vec2 tileSize = _texScene->tileSize();
    tileSize.y /= 2;
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

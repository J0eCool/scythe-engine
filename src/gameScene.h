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

    Entity(Vec3 pos, Vec3 vel, Vec2 size);

    virtual void update(float dt) = 0;
    virtual void render(Renderer* renderer);
};

struct Bullet : public Entity {
    float _lifespan;
    float _lived;

    Bullet(Vec3 pos, Vec3 vel, float lifespan);

    void update(float dt) override;
    bool shouldRemove() const;
};

struct Player : public Entity {
    bool _facingRight = true;
    bool _isOnGround = false;
    float _spinT = 0.0;

    /// HACK: we set this in game->update, which is a terrible way to do this
    const Input* _input;
    Player();

    void update(float dt) override;
    void render(Renderer* renderer) override;

    Vec3 widgetPos() const;
};

class GameScene : public Scene {
    Player _player;
    std::vector<Bullet> _bullets;

    Input* _input;
    TexGen* _texGen;
    TexGenScene* _texScene;

public:
    GameScene(Input* input, TexGen *texGen, TexGenScene* texScene);

    void update(float dt) override;
    void render(Renderer* renderer) override;
};

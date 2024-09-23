// RpgScene - actual gameplay logic

#pragma once

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "ui.h"
#include "vec.h"

#include <math.h>
#include <vector>

struct Fighter {
    const char* name;
    int hp, maxHp;
    int attack, defense;
    int speed;

    // counts down from 1.0 to 0.0
    float action;
};

class RpgScene : public Scene {
    Allocator* _allocator;
    Input* _input;
    UI _ui;

    Fighter _player, _enemy;
    int _xp;

public:
    RpgScene(Allocator* allocator, Input* input) :
            _allocator(allocator), _input(input),
            _ui(allocator, input) {
        initBattle();
    }

    void onUnload() override {
        _ui.unload();
    }

    void update(float dt) override {
        _ui.startUpdate();

        _ui.region({240, 120});
        uiFighter(_player);
        _ui.labels("xp: ", _xp, "\n");
        if (_ui.button("attack")) {
            _enemy.hp -= max(0, _player.attack - _enemy.defense);
            if (_enemy.hp <= 0) {
                _xp += _enemy.attack;
                _enemy.maxHp += 5;
                _enemy.hp = _enemy.maxHp;
                _enemy.attack += 2;
                _enemy.defense += 1;
                _enemy.speed += 3;
            } else {
                _player.hp -= max(0, _enemy.attack - _player.defense);
                if (_player.hp <= 0) {
                    initBattle();
                }
            }
        }
        if (_ui.button("flee")) {
            initBattle();
        }

        _ui.region({800, 120});
        uiFighter(_enemy);
    }

    void uiFighter(Fighter& fighter) {
        _ui.labels(fighter.name, "\n");
        _ui.labels("HP: ", fighter.hp, "/", fighter.maxHp, "\n");
        _ui.labels("ATK: ", fighter.attack, "\n");
        _ui.labels("DEF: ", fighter.defense, "\n");
    }

    void render(Renderer* renderer) override {
        renderer->background({0x10, 0x20, 0xc0});
        _ui.render(renderer);

        renderer->drawText("Remember, you are Free", {800, 900});
        renderer->drawText("FREE", {1200, 950});
    }

    void initBattle() {
        _player = { "Player", 100, 100, 7, 3, 100, 0.0 };
        _enemy = { "Emeney", 25, 25, 6, 2, 90, 0.0 };
    }

    void uiCursor() {}
};

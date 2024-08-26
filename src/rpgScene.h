// RpgScene - actual gameplay logic

#pragma once

#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "ui.h"
#include "vec.h"

#include <math.h>
#include <vector>

struct Fighter {
    int hp, maxHp;
    int attack, defense;
    int speed;

    // counts down from 1.0 to 0.0
    float action;
};

class RpgScene {
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

    void update(float dt) {
        _ui.startUpdate({ 240, 120 });
        _ui.label("Player");
        _ui.line();
        _ui.labels("HP: ", _player.hp, "/", _player.maxHp);
        _ui.line();
        _ui.labels("xp: ", _xp);
        _ui.line();
        _ui.label(" "); // hack for newline
        _ui.line();
        _ui.label("Enemy");
        _ui.line();
        _ui.labels("HP: ", _enemy.hp, "/", _enemy.maxHp);
        _ui.line();
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
    }
    void render(Renderer* renderer) {
        _ui.render(renderer);
    }

    void initBattle() {
        _player = { 100, 100, 7, 3, 100, 0.0 };
        _enemy = { 25, 25, 6, 2, 90, 0.0 };
    }

    void uiCursor() {}
};

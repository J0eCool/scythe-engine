#include "common.h"
#include "gameScene.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "texGenScene.h"
#include "ui.h"
#include "vec.h"

#include <math.h>
#include <random>
#include <string.h>
#include <vector>

#include <SDL2/SDL.h>

struct Program {
    Allocator* _allocator;
    SDL_Window* _window;
    Renderer* _renderer;
    Input _input;
    float t = 0.0;
    bool _quit = false;

    UI _menu;
    GameScene _gameScene;
    TexGenScene _texScene;
    void* _curScene;

    Program(Allocator* allocator)
            : _allocator(allocator), _texScene(_allocator, &_input),
            _menu(allocator, &_input) {
        _curScene = &_texScene;
        _window = SDL_CreateWindow(
            "I Heard You Liked Video Games",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            screenSize.x, screenSize.y,
            SDL_WINDOW_SHOWN);
        assert_SDL(_window, "window creation failed");

        SDL_Surface* screen = SDL_GetWindowSurface(_window);
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
        SDL_UpdateWindowSurface(_window);

        SDL_Renderer* renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
        assert_SDL(renderer, "renderer creation failed");
        /// HACK: we should probably allocate _renderer directly as a struct member
        /// we heap-allocate it because it's easier than dealing with constructor shenanigans
        _renderer = (Renderer*)_allocator->calloc(1, sizeof(Renderer));
        new (_renderer) Renderer(renderer);
    }

    ~Program() {
        SDL_DestroyRenderer(_renderer->sdl());
        SDL_DestroyWindow(_window);

        _allocator->free(_renderer);
    }

    /// @brief Called after loading the dll, and on each reload.
    /// Useful for iterating configs at the moment
    void onLoad() {
        _texScene.onLoad();

        _input.resetBindings();

        _input.addKeybind("quit", SDLK_ESCAPE);

        _input.addKeybind("left", SDLK_a);
        _input.addKeybind("right", SDLK_d);
        _input.addKeybind("up", SDLK_w);
        _input.addKeybind("down", SDLK_s);
        _input.addKeybind("jump", SDLK_k);
        _input.addKeybind("shoot", SDLK_j);
        _input.addAxisPair("horizontal", "left", "right");
        _input.addAxisPair("vertical", "up", "down");

        _input.addControllerBind("jump", SDL_CONTROLLER_BUTTON_A);
        _input.addControllerBind("shoot", SDL_CONTROLLER_BUTTON_X);
        _input.addControllerBind("left", SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        _input.addControllerBind("right", SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        _input.addControllerAxis("horizontal", SDL_CONTROLLER_AXIS_LEFTX);
        _input.addControllerAxis("vertical", SDL_CONTROLLER_AXIS_LEFTY);

        _input.addKeybind("1", SDLK_1);
        _input.addKeybind("2", SDLK_2);
        _input.addKeybind("3", SDLK_3);
        _input.addKeybind("4", SDLK_4);
        _input.addKeybind("5", SDLK_5);

        _input.addMouseBind("click", SDL_BUTTON_LEFT);
        _input.addMouseBind("rclick", SDL_BUTTON_RIGHT);

        _texScene.generateTextures(_renderer);
    }

    /// @brief Called before unloading the dll. Clear any state that can't be
    /// persisted across reloads.
    void onUnload() {
        _texScene.onUnload();
    }

    bool shouldQuit() {
        return _quit;
    }

    void update(float dt) {
        int stackval = 0;
        Tracer trace("Game::update");
        trace("stack addr: %x (val=%d)", &stackval, stackval);

        _input.update();

        if (_input.didPress("1")) {
            log("njoy: %d", SDL_NumJoysticks());
        }

        trace("input handling");
        if (_input.didPress("quit")) {
            trace("quitting");
            _quit = true;
            return;
        }

        // change current scene
        _menu.startUpdate({30, 30});
        if (_menu.button("texgen")) {
            _curScene = &_texScene;
        }
        _menu.line();
        if (_menu.button("game")) {
            _curScene = &_gameScene;
        }

        // update current scene
        if (_curScene == &_texScene) {
            trace("texture generation update");
            _texScene.update(dt, _renderer);
        } else if (_curScene == &_gameScene) {
            trace("gameScene update");
            _gameScene.update(&_input, dt);
        }

    }

    void render() {
        Tracer trace("Game::render");

        if (_curScene == &_texScene) {
            trace("texture generation render");
            _texScene.render(_renderer);
        } else if (_curScene == &_gameScene) {
            trace("gameScene render");
            _gameScene.render(_renderer, &_texScene);
        }

        _menu.render(_renderer);

        // only trace for one frame per reload to minimize spam
        trace("end"); // we're about to disable tracing so, make it match lol
        debug_isTracing = false;
    }
};

extern "C" {

__declspec(dllexport)
Program* newGame(Allocator* allocator) {
    Program* game = (Program*)allocator->calloc(1, sizeof(Program));
    new (game) Program(allocator);
    return game;
}
__declspec(dllexport)
void freeGame(Program* game, Allocator* allocator) {
    game->~Program();
    allocator->free(game);
}

__declspec(dllexport)
void onUnload(Program* game) {
    game->onUnload();
}

__declspec(dllexport)
void onLoad(Program* game) {
    game->onLoad();
}

__declspec(dllexport)
bool shouldQuit(Program* game) {
    return game->shouldQuit();
}

__declspec(dllexport)
void update(Program* game, float dt) {
    game->t += dt;
    game->update(dt);
}

__declspec(dllexport)
const void renderScene(Program* game) {
    SDL_Renderer* renderer = game->_renderer->sdl();
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
    // SDL_RenderDrawRect(renderer, nullptr);
    SDL_RenderClear(renderer);

    game->render();

    SDL_RenderPresent(renderer);
}

} // extern "C"

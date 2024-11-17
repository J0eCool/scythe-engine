#include "common.h"
#include "input_sdl.h"
#include "render_sdl.h"
#include "scene.h"
#include "ui.h"
#include "vec.h"

#include "audioScene.h"
#include "eyeGenScene.h"
#include "gameScene.h"
#include "particleScene.h"
#include "rpgScene.h"
#include "texGenScene.h"

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
    TexGen _texGen;

    struct SceneDesc {
        const char* name;
        Scene* scene;
    };
    std::vector<SceneDesc> _scenes;
    // stored as an index for ease of serializing state
    int _curScene = 0;

    Program(Allocator* allocator) :
            _allocator(allocator),
            _menu(allocator, &_input) {
        _window = SDL_CreateWindow(
            "I Heard You Liked Video Games",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            screenSize.x, screenSize.y,
            SDL_WINDOW_SHOWN);
        assert_SDL(_window, "window creation failed");

        SDL_Surface* screen = SDL_GetWindowSurface(_window);
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
        SDL_UpdateWindowSurface(_window);

        SDL_Renderer* sdlRenderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED);
        assert_SDL(sdlRenderer, "renderer creation failed");
        /// HACK: we should probably allocate _renderer directly as a struct member
        /// we heap-allocate it because it's easier than dealing with constructor shenanigans
        _renderer = (Renderer*)_allocator->calloc(1, sizeof(Renderer));
        new (_renderer) Renderer(sdlRenderer);
    }

    ~Program() {
        SDL_DestroyRenderer(_renderer->sdl());
        SDL_DestroyWindow(_window);

        _allocator->free(_renderer);
    }

    /// @brief Called after loading the dll, and on each reload.
    /// Useful for iterating configs at the moment
    void onLoad() {
        auto tex = _allocator->knew<TexGenScene>(&_texGen, _allocator, &_input);
        _scenes.push_back({"texgen", tex});
        _scenes.push_back({"eyegen",
            _allocator->knew<EyeGenScene>(_allocator, &_input)});
        _scenes.push_back({"game",
            _allocator->knew<GameScene>(&_input, &_texGen, tex)});
        _scenes.push_back({"rpg",
            _allocator->knew<RpgScene>(_allocator, &_input)});
        _scenes.push_back({"particles",
            _allocator->knew<ParticleScene>(_allocator, &_input)});
        _scenes.push_back({"audio",
            _allocator->knew<AudioScene>(_allocator, &_input)});
        for (auto desc : _scenes) {
            desc.scene->onLoad();
        }

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
        _input.addControllerBind("up", SDL_CONTROLLER_BUTTON_DPAD_UP);
        _input.addControllerBind("down", SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        _input.addControllerAxis("horizontal", SDL_CONTROLLER_AXIS_LEFTX);
        _input.addControllerAxis("vertical", SDL_CONTROLLER_AXIS_LEFTY);

        _input.addKeybind("1", SDLK_1);
        _input.addKeybind("2", SDLK_2);
        _input.addKeybind("3", SDLK_3);
        _input.addKeybind("4", SDLK_4);
        _input.addKeybind("5", SDLK_5);

        _input.addMouseBind("click", SDL_BUTTON_LEFT);
        _input.addMouseBind("rclick", SDL_BUTTON_RIGHT);

        _texGen.generateTextures(_renderer);
    }

    /// @brief Called before unloading the dll. Clear any state that can't be
    /// persisted across reloads.
    void onUnload() {
        // unload in reverse of load order, in case of interscene dependencies,
        // which we probably shouldn't encourage, but this seems saner than not
        for (int i = _scenes.size()-1; i >= 0; i--) {
            auto scene = _scenes[i].scene;
            scene->onUnload();
            _allocator->del(scene);
        }
        _scenes.clear();
    }

    bool shouldQuit() {
        return _quit;
    }

    Scene* scene() {
        return _scenes[_curScene].scene;
    }

    void update(float dt) {
        int stackval = 0;
        Tracer trace("Game::update");
        trace("stack addr: %x (val=%d)", &stackval, stackval);

        _input.update();

        trace("input handling");
        if (_input.didPress("quit")) {
            trace("quitting");
            _quit = true;
            return;
        }

        // change current scene
        _menu.startUpdate({30, 30});
        for (int i = 0; i < _scenes.size(); ++i) {
            _menu.line();
            if (_menu.button(_scenes[i].name)) {
                _curScene = i;
            }
        }

        // update current scene
        scene()->update(dt);
    }

    void render() {
        Tracer trace("Game::render");
        _renderer->startFrame();

        scene()->render(_renderer);
        _menu.render(_renderer);

        _renderer->endFrame();

        // only trace for one frame per reload to minimize spam
        trace("end"); // we're about to disable tracing so, make it match lol
        debug_isTracing = false;
    }
};

extern "C" {

__declspec(dllexport)
Program* newGame(Allocator* allocator) {
    return allocator->knew<Program>(allocator);
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
    game->render();
}

} // extern "C"

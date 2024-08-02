// dylib.h - code for loading/reloading dynamic libraries; abstracts over platform differences

#pragma once

#include "common.h"

#include <SDL2/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

struct GameDylib {
private:
    void* _gameLib;
    const char* _filename;

public:
    typedef void* (__cdecl *newGame_t)(Allocator*);
    newGame_t newGame;
    typedef void (__cdecl *freeGame_t)(void*, Allocator*);
    freeGame_t freeGame;
    typedef void (__cdecl *onUnload_t)(void*);
    onUnload_t onUnload;
    typedef void (__cdecl *onLoad_t)(void*);
    onLoad_t onLoad;
    typedef bool (__cdecl *shouldQuit_t)(void*);
    shouldQuit_t shouldQuit;
    typedef int (__cdecl *update_t)(void*, float);
    update_t update;
    typedef const void (__cdecl *renderScene_t)(void*);
    renderScene_t renderScene;

    GameDylib(const char* filename) : _filename(filename) {
        load();
    }

    ~GameDylib() {
        unload();
    }

    void reload() {
        unload();
        load();
    }

private:
    void load();
    void unload();
};

void GameDylib::load() {
    Tracer trace("GameDylib::load");
#ifdef _WIN32
    // first we make a copy, so that we can overwrite the original without
    // windows yelling at us
    static const char* copyDllName = "_copy_game.dll";
    if (!CopyFile(_filename, copyDllName, /*failIfExists*/ false)) {
        printf("!!couldn't copy %s\n", _filename);
        printf("  Fatal Error: %d\n", GetLastError());
        exit(1);
    }
#endif

    _gameLib = SDL_LoadObject(copyDllName);
    trace("_gameLib addr=%x", _gameLib);
    assert(_gameLib, "_gameLib failed to load");

    newGame = (newGame_t)SDL_LoadFunction(_gameLib, "newGame");
    trace("newGame addr=%x", newGame);
    assert(newGame, "newGame didn't load");

    freeGame = (freeGame_t)SDL_LoadFunction(_gameLib, "freeGame");
    trace("freeGame addr=%x", freeGame);
    assert(freeGame, "freeGame didn't load");

    onUnload = (onUnload_t)SDL_LoadFunction(_gameLib, "onUnload");
    trace("onUnload addr=%x", onUnload);
    assert(onUnload, "onUnload didn't load");

    onLoad = (onLoad_t)SDL_LoadFunction(_gameLib, "onLoad");
    trace("onLoad addr=%x", onLoad);
    assert(onLoad, "onLoad didn't load");

    shouldQuit = (shouldQuit_t)SDL_LoadFunction(_gameLib, "shouldQuit");
    trace("shouldQuit addr=%x", shouldQuit);
    assert(shouldQuit, "shouldQuit didn't load");

    update = (update_t)SDL_LoadFunction(_gameLib, "update");
    trace("update addr=%x", update);
    assert(update, "update didn't load");

    renderScene = (renderScene_t)SDL_LoadFunction(_gameLib, "renderScene");
    trace("renderScene addr=%x", renderScene);
    assert(renderScene, "renderScene didn't load");
}

void GameDylib::unload() {
    SDL_UnloadObject(_gameLib);
}

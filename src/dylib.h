// dylib.h - code for loading/reloading dynamic libraries; abstracts over platform differences

#pragma once

#include "common.h"
#include "input.h"

#include <SDL2/SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

typedef void* (*allocator_t)(size_t, size_t); // move this to a common.h or smth

struct GameDylib {
private:
    void* _gameLib;
    const char* _filename;

public:
    typedef void* (__cdecl *newGame_t)(allocator_t _calloc);
    newGame_t newGame;
    typedef void (__cdecl *freeGame_t)(void*, void (*_free)(void*));
    freeGame_t freeGame;
    typedef void (__cdecl *onLoad_t)(void*);
    onLoad_t onLoad;
    typedef bool (__cdecl *shouldQuit_t)(void*);
    shouldQuit_t shouldQuit;
    typedef int (__cdecl *update_t)(void*, float, Input*);
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
    assert(_gameLib, "_gameLib failed to load");
    newGame = (newGame_t)SDL_LoadFunction(_gameLib, "newGame");
    assert(newGame, "newGame didn't load");
    freeGame = (freeGame_t)SDL_LoadFunction(_gameLib, "freeGame");
    assert(freeGame, "freeGame didn't load");
    onLoad = (onLoad_t)SDL_LoadFunction(_gameLib, "onLoad");
    assert(onLoad, "onLoad didn't load");
    shouldQuit = (shouldQuit_t)SDL_LoadFunction(_gameLib, "shouldQuit");
    assert(shouldQuit, "shouldQuit didn't load");
    update = (update_t)SDL_LoadFunction(_gameLib, "update");
    assert(update, "update didn't load");
    renderScene = (renderScene_t)SDL_LoadFunction(_gameLib, "renderScene");
    assert(renderScene, "renderScene didn't load");
}

void GameDylib::unload() {
    SDL_UnloadObject(_gameLib);
}

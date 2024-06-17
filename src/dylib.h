// dylib.h - code for loading/reloading dynamic libraries; abstracts over platform differences

#pragma once

#include "common.h"
#include "render.h"
#include "input.h"

#ifdef _WIN32
#include <windows.h>

typedef HMODULE DylibHandle;
#elif __linux__
#error not yet implemented
#endif

typedef void* (*allocator_t)(size_t, size_t); // move this to a common.h or smth

struct Dylib {
private:
    DylibHandle _gameLib;
    const char* _filename;

public:
    typedef void* (__cdecl *newGame_t)(allocator_t _calloc);
    newGame_t newGame;
    typedef int (__cdecl *handleInput_t)(void*, Input*);
    handleInput_t handleInput;
    typedef int (__cdecl *update_t)(void*, float);
    update_t update;
    typedef const void (__cdecl *renderScene_t)(void*, Renderer*);
    renderScene_t renderScene;

    Dylib(const char* filename) : _filename(filename) {
        load();
    }

    ~Dylib() {
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

#ifdef _WIN32

static const char* copyDllName = "_copy_game.dll";
void Dylib::load() {
    // first we make a copy, so that we can overwrite the original without
    // windows yelling at us
    if (!CopyFile(_filename, copyDllName, /*failIfExists*/ false)) {
        printf("!!couldn't copy %s\n", _filename);
        printf("  Error: %d\n", GetLastError());
    }

    _gameLib = LoadLibrary(copyDllName);
    check(_gameLib, "_gameLib failed to load");
    newGame = (newGame_t)GetProcAddress(_gameLib, "newGame");
    check(newGame, "newGame didn't load");
    update = (update_t)GetProcAddress(_gameLib, "update");
    check(update, "update didn't load");
    renderScene = (renderScene_t)GetProcAddress(_gameLib, "renderScene");
    check(renderScene, "renderScene didn't load");
}

void Dylib::unload() {
    FreeLibrary(_gameLib);
}

#endif

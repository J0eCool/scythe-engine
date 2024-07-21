// Scythe Engine main file

#include "common.h"
#include "dylib.h"
#include "input_sdl.h"

#include <math.h>
#include <windows.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

int main(int argc, char** argv) {
    // let's focus on geting a thing up and running

    assert_SDL(SDL_Init(SDL_INIT_EVERYTHING) >= 0, "sdl_init failed");
    auto imgFlags = IMG_INIT_PNG;
    if (IMG_Init(imgFlags) != imgFlags) {
        printf("SDL_image failed to load\n%s\n", IMG_GetError());
        exit(1);
    }

    log("SDL loaded");

    // Load game.dll
    const char* dllName = "game.dll";
    GameDylib dll(dllName);

    Input_SDL input;

    input.addKeybind("quit", SDLK_ESCAPE);
    input.addKeybind("reload", SDLK_r);
    input.addKeybind("logging", SDLK_l);

    input.addKeybind("left", SDLK_a);
    input.addKeybind("right", SDLK_d);
    input.addKeybind("up", SDLK_w);
    input.addKeybind("down", SDLK_s);
    input.addKeybind("jump", SDLK_k);
    input.addKeybind("shoot", SDLK_j);

    input.addKeybind("1", SDLK_1);
    input.addKeybind("2", SDLK_2);
    input.addKeybind("3", SDLK_3);
    input.addKeybind("4", SDLK_4);
    input.addKeybind("5", SDLK_5);

    log("setup complete");

    bool quit = false;
    void* game = dll.newGame(calloc);
    DWORD lastDateTime = 0;
    // Main game loop
    while (!quit) {
        // check game.cpp for changes
        FILETIME modified;
        const char* filename = "../src/game.cpp";
        HANDLE file = CreateFile(filename, 0, 0, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(file != INVALID_HANDLE_VALUE, "failed to load file %s", filename);
        assert(GetFileTime(file, nullptr, nullptr, &modified),
            "couldn't get file time for %s", filename);
        CloseHandle(file);
        if (lastDateTime == 0) {
            // don't rebuild on first launch
            lastDateTime = modified.dwLowDateTime;
        }
        if (modified.dwLowDateTime != lastDateTime) {
            log("rebuilding...");
            fflush(stdout);
            // `system` runs a command from where the exe is, so we cwd to root
            system("bash -c \"cd ..; ./build-game.sh\"");

            dll.reload();
            lastDateTime = modified.dwLowDateTime;
        }

        // Handle Input
        input.update();

        if (input.didPress("quit")) {
            quit = true;
        }

        // Update logic
        float dt = 0.01;
        dll.update(game, dt, &input);

        dll.renderScene(game);

        // End-of-frame bookkeeping
        fflush(stdout);
        SDL_Delay(10);
    }

    dll.quitGame(game, free);

    SDL_Quit();

    log("everything went better than expected");
    return 0;
}

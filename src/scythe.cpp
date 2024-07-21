// Scythe Engine main file

#include "common.h"
#include "dylib.h"

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

    log("setup complete");

    void* game = dll.newGame(calloc);
    dll.onLoad(game);
    DWORD lastDateTime = 0;
    // Main game loop
    while (!dll.shouldQuit(game)) {
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
            lastDateTime = modified.dwLowDateTime;

            log("rebuilding...");
            fflush(stdout);
            // `system` runs a command from where the exe is, so we cwd to root
            system("bash -c \"cd ..; ./build-game.sh\"");

            dll.reload();
            dll.onLoad(game);
        }

        // Update logic
        float dt = 0.01;
        dll.update(game, dt);

        dll.renderScene(game);

        // End-of-frame bookkeeping
        fflush(stdout);
        SDL_Delay(10);
    }

    dll.freeGame(game, free);

    SDL_Quit();

    log("everything went better than expected");
    return 0;
}

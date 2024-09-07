// Scythe Engine main file

#include "builder.h"
#include "common.h"
#include "dylib.h"

#include <math.h>
#include <map>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


int main(int argc, char** argv) {
    log("Loading SDL...");
    assert_SDL(SDL_Init(SDL_INIT_EVERYTHING) >= 0, "sdl_init failed");
    auto imgFlags = IMG_INIT_PNG;
    if (IMG_Init(imgFlags) != imgFlags) {
        printf("SDL_image failed to load\n%s\n", IMG_GetError());
        exit(1);
    }

    log("Building game.dll...");
    // initialize last modified times for each file we care about
    for (auto filename : filesToScan) {
        lastModifiedTimes[filename] = getFileModifiedTime(filename);
    }
    assert(buildGame(), "failed to build game.dll on launch");
    const char* dllName = "game.dll";
    GameDylib dll(dllName);

    log("setup complete");

    Allocator allocator { malloc, calloc, free };
    void* game = dll.newGame(&allocator);
    dll.onLoad(game);
    // Main game loop
    while (!dll.shouldQuit(game)) {
        // check program.cpp for changes
        if (shouldRebuild()) {
            Timer buildTime;

            log("rebuilding game.dll...");
            if (check(buildGame(), "failed to build dll, continuing with old code")) {
                log("  game built in %fs", buildTime.elapsed());
                dll.onUnload(game);
                dll.reload();
                dll.onLoad(game);
            }
        }

        // Update logic
        float dt = 0.01;
        dll.update(game, dt);

        dll.renderScene(game);

        // End-of-frame bookkeeping
        fflush(stdout);
        SDL_Delay(10);
    }

    dll.freeGame(game, &allocator);

    SDL_Quit();

    log("everything went better than expected");
    return 0;
}

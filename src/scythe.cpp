// Scythe Engine main file

#include "common.h"
#include "dylib.h"

#include <math.h>
#include <map>
#include <vector>
#include <windows.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

// List of filenames to watch for changes to rebuild game.dll
// TODO: should probably this to the file watching API before this list gets
// too big, at 8 files watched I can already see 0.5% additional CPU usage :(
std::vector<const char*> filesToScan {
    "../src/common.h",
    "../src/input_sdl.h",
    "../src/gameScene.h",
    "../src/render_sdl.h",
    "../src/texGenScene.h",
    "../src/ui.h",
    "../src/vec.h",

    "../src/program.cpp",
};
// The last time each file was modified
std::map<const char*, FILETIME> lastModifiedTimes;

/// @brief Get the time that a file was last modified
FILETIME getFileModifiedTime(const char* filename) {
    FILETIME modified;
    HANDLE file = CreateFile(filename, 0, 0, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(file != INVALID_HANDLE_VALUE, "failed to load file %s", filename);
    assert(GetFileTime(file, nullptr, nullptr, &modified),
        "couldn't get file time for %s", filename);
    CloseHandle(file);
    return modified;
}

/// @brief Whether to rebuild the game dll
/// @return true if any source files have changed
bool shouldRebuild() {
    bool changed = false;
    for (auto filename : filesToScan) {
        FILETIME last = lastModifiedTimes[filename];
        FILETIME modified = getFileModifiedTime(filename);
        if (modified.dwLowDateTime != last.dwLowDateTime ||
                modified.dwHighDateTime != last.dwHighDateTime) {
            lastModifiedTimes[filename] = modified;
            // don't return early to update all changed file times in one build
            changed = true;
        }
    }
    return changed;
}

/// @brief Builds the game.dll from source
/// @return true iff the build succeeded
bool buildGame() {
    fflush(stdout);
    const char* cmd = "g++ -o game.dll ../src/program.cpp"
        " -s -shared -lmingw32 -lSDL2 -lSDL2_image"
        " -fdiagnostics-color=always -g"
        ;
    return system(cmd) == 0;
}

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

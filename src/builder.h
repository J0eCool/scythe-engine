#pragma once

#include "common.h"

#include <map>
#include <stdio.h>
#include <vector>
#include <windows.h>

// List of filenames to watch for changes to rebuild game.dll
// TODO: should probably this to the file watching API before this list gets
// too big, at 8 files watched I can already see 0.5% additional CPU usage :(
// updating this is starting to get obnoxious... prolly worth at least scanning
// for .h files or smth istg
std::vector<const char*> filesToScan {
    "../src/color.h",
    "../src/common.h",
    "../src/input_sdl.h",
    "../src/render_sdl.h",
    "../src/scene.h",
    "../src/serialize.h",
    "../src/texGen.h",
    "../src/ui.h",
    "../src/vec.h",

    "../src/eyeGenScene.h",
    "../src/gameScene.h",
    "../src/rpgScene.h",
    "../src/texGenScene.h",

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

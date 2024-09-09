#pragma once

#include "common.h"

#include <map>
#include <set>
#include <stdio.h>
#include <vector>
#include <windows.h>

// we want to track every source file and any transitive dependencies
// so create a graph by scanning files for #includes

class Builder {
    // List of filenames to watch for changes to rebuild game.dll
    // TODO: should probably this to the file watching API before this list gets
    // too big, at 8 files watched I can already see 0.5% additional CPU usage :(
    // updating this is starting to get obnoxious... prolly worth at least scanning
    // for .h files or smth istg
    std::vector<const char*> filesToScan {
        "../src/color.h",
        "../src/common.h",
        "../src/common.cpp",
        "../src/input_sdl.h",
        "../src/render_sdl.h",
        "../src/scene.h",
        "../src/serialize.h",
        "../src/texGen.h",
        "../src/ui.h",
        "../src/vec.h",

        "../src/eyeGenScene.h",
        "../src/eyeGenScene.cpp",
        "../src/gameScene.h",
        "../src/rpgScene.h",
        "../src/texGenScene.h",

        "../src/program.cpp",
    };
    // The last time each file was modified
    std::map<const char*, FILETIME> lastModifiedTimes;

public:
    Builder() {
        // initialize last modified times for each file we care about
        for (auto filename : filesToScan) {
            lastModifiedTimes[filename] = getFileModifiedTime(filename);
        }
    }

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

    /// @brief rebuilds the game if possible
    /// @return true if we rebuilt successfully
    bool tryRebuild() {
        // check program.cpp for changes
        if (shouldRebuild()) {
            log("rebuilding game.dll...");
            return check(buildGame(), "failed to build dll, continuing with old code");
        }
        // no build means nothing to do downstream either
        return false;
    }

    /// @brief Builds the game.dll from source
    /// @return true iff the build succeeded
    bool buildGame() {
        Timer buildTime;

        std::string flags = " -s -shared -lmingw32 -lSDL2 -lSDL2_image"
            " -fdiagnostics-color=always -g";
        std::vector<std::string> objs {
            "common",
            "eyeGenScene",
            "program",
        };

        // build
        for (auto obj : objs) {
            std::string cmd = "g++ -c -o "+obj+".o ../src/"+obj+".cpp" + flags;
            Timer time;
            log("building obj %s: %s", obj.c_str(), cmd.c_str());
            // nonzero status means something failed
            fflush(stdout);
            auto success = system(cmd.c_str()) == 0;
            log("  build time: %fs", time.elapsed());
            if (!success) {
                return false;
            }
        }

        // link
        std::string cmd = "g++ -o game.dll";
        for (auto obj : objs) {
            cmd += " "+obj+".o";
        }
        cmd += flags;
        Timer linkTime;
        log("linking game.dll: %s", cmd.c_str());
        fflush(stdout);
        log("  link time: %fs", linkTime.elapsed());
        bool success = system(cmd.c_str()) == 0;
        log("Total build time: %fs", buildTime.elapsed());
        return success;
    }
};

// corresponds to 1 source file, .cpp or .h
struct SourceFile {
    std::string filename;
    std::set<SourceFile*> includes;

    void scanForIncludes() {
        //TODO: move elsewhere because we want references to existing source files
        includes.clear();
        // pseudocode
        // for (auto line : open(filename).lines()) {
        //     if (line.startsWith("#include \"") && line.endsWith("\"\n")) {
        //         includes.insert(line.substring(9, -2));
        //     }
        // }
    }

    bool needsRebuild(std::set<std::string> const& changed) {
        if (changed.find(filename) != changed.end()) {
            return true;
        }
        for (auto inc : includes) {
            if (inc->needsRebuild(changed)) {
                return true;
            }
        }
        return false;
    }
};

// corresponds to 1 built file, .o or .exe
struct BuildObject {
    std::string cmd;
};

#include "test.h"

TEST(buildLogic, {
    std::vector<SourceFile> files;
    SourceFile foo {"foo.h", {}};
    SourceFile bar {"bar.h", {&foo}};
    SourceFile baz {"baz.h", {&bar}};

    TEST_EQ(foo.needsRebuild({}), false);
    TEST_EQ(foo.needsRebuild({"foo.h"}), true);
    TEST_EQ(bar.needsRebuild({"foo.h"}), true);
    TEST_EQ(foo.needsRebuild({"bar.h"}), false);
    TEST_EQ(bar.needsRebuild({"bar.h"}), true);
    TEST_EQ_MSG(baz.needsRebuild({"foo.h"}), true, "transitive dependencies");

})

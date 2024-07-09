// Scythe Engine main file

#include "common.h"
#include "dylib.h"
#include "input_sdl.h"

#include <math.h>

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
    // Main game loop
    while (!quit) {
        // Handle Input
        input.update();

        if (input.didPress("quit")) {
            quit = true;
        }
        if (input.didPress("reload")) {
            log("rebuilding...");
            fflush(stdout);
            // `system` runs a command from where the exe is, so we cwd to root
            system("bash -c \"cd ..; ./build-game.sh\"");

            log("reloading...");
            dll.reload();
        }
        if (input.didPress("logging")) {
            // two logging statements here, logging_enabled
            // will only be true for one of them
            log("logging: DISABLED");
            logging_enabled = !logging_enabled;
            log("logging: ENABLED");
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

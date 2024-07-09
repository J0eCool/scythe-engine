// Scythe Engine main file

#include "common.h"
#include "dylib.h"
#include "input_sdl.h"
#include "render_sdl.h"

#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

void assert_SDL(bool cond, const char* msg) {
    if (!cond) {
        printf("Fatal Error: %s\nSDL_Error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}

int main(int argc, char** argv) {
    // let's focus on geting a thing up and running

    assert_SDL(SDL_Init(SDL_INIT_EVERYTHING) >= 0, "sdl_init failed");
    auto imgFlags = IMG_INIT_PNG;
    if (IMG_Init(imgFlags) != imgFlags) {
        printf("SDL_image failed to load\n%s\n", IMG_GetError());
        exit(1);
    }

    int screenWidth = 800;
    int screenHeight = 600;
    SDL_Window* window = SDL_CreateWindow(
        "Scythe",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screenWidth, screenHeight,
        SDL_WINDOW_SHOWN);
    assert_SDL(window, "window creation failed");

    SDL_Surface* screen = SDL_GetWindowSurface(window);
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);

    auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    assert_SDL(renderer, "renderer creation failed");

    log("SDL loaded");

    // Load game.dll
    const char* dllName = "game.dll";
    GameDylib dll(dllName);

    Input_SDL input;
    Renderer_SDL dllRenderer(renderer);

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

        // Drawing
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
        // SDL_RenderDrawRect(renderer, nullptr);
        SDL_RenderClear(renderer);

        dll.renderScene(game, &dllRenderer);

        SDL_RenderPresent(renderer);

        // End-of-frame bookkeeping
        fflush(stdout);
        SDL_Delay(10);
    }

    free(game);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    log("everything went better than expected");
    return 0;
}

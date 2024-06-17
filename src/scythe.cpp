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
    // for now just hardcode SDL+OpenGL
    // later on we'll really design the game-level abstractions we feel like
    // sticking with, but until then let's just get a thing up and running
    printf("Hello world\n");

    assert_SDL(SDL_Init(SDL_INIT_VIDEO) >= 0, "sdl_init failed");
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

    printf("SDL loaded\n");

    // Load game.dll
    const char* dllName = "game.dll";
    Dylib dll(dllName);

    Input_SDL input;
    Renderer_SDL dllRenderer(renderer);

    printf("setup complete\n");

    bool quit = false;
    void* game = dll.newGame(&input, calloc);
    // Main game loop
    while (!quit) {
        // Handle Input
        input.update();

        if (input.wasPressed("quit")) {
            quit = true;
        }
        if (input.wasPressed("reload")) {
            printf("rebuilding...\n");
            fflush(stdout);
            // `system` runs a command from where the exe is, so we cwd to root
            system("bash -c \"cd ..; ./build-game.sh\"");

            printf("reloading...\n");
            dll.reload();
        }
        if (input.wasPressed("logging")) {
            logging_enabled = !logging_enabled;
        }

        // Update logic
        float dt = 0.01;
        dll.update(game, dt);

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

    printf("everything went better than expected\n");
    return 0;
}

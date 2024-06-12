// Scythe Engine main file

#include "common.h"
#include "dylib.h"
#include "render.h"
#include "render_sdl.h"

#include <math.h>

#include <SDL2/SDL.h>

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

    Renderer_SDL dllRenderer(renderer);

    printf("setup complete\n");

    SDL_Event event;
    bool quit = false;
    void* game = dll.newGame(calloc);
    // Main game loop
    while (!quit) {
        // Handle Input
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_r:
                    // printf("rebuilding...\n");
                    // system("./build-game.sh");

                    printf("reloading...\n");
                    dll.reload();
                    break;
                case SDLK_l:
                    logging_enabled = !logging_enabled;
                    break;
                }
            }
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

    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("everything went better than expected\n");
    return 0;
}

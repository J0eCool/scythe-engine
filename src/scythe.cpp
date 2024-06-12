// Scythe Engine main file

#include "render.h"
#include "render_sdl.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <windows.h>

// a check is a nonfatal assert
void check(bool cond, const char* msg) {
    if (!cond) {
        printf("Error: %s\n");
    }
}
void assert_SDL(bool cond, const char* msg) {
    if (!cond) {
        printf("Fatal Error: %s\nSDL_Error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}

bool logging_enabled = true;
template <typename... Ts>
void log(const char* fmt, Ts... args) {
    if (logging_enabled) {
        printf(fmt, args...);
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
    // first we make a copy, so that we can overwrite the original without
    // windows yelling at us
    const char* dllName = "game.dll";
    const char* copyDllName = "_copy_game.dll";
    if (!CopyFile(dllName, copyDllName, /*failIfExists*/ false)) {
        printf("!!couldn't copy game.dll\n  Error: %d\n", GetLastError());
    }
    HMODULE gameLib = LoadLibrary(copyDllName);
    check(gameLib, "well the game lib didn't load\n");
    typedef void* (*allocator_t)(size_t, size_t); // move this to a common.h or smth
    typedef void* (__cdecl *newGame_t)(allocator_t _calloc);
    newGame_t newGame = (newGame_t)GetProcAddress(gameLib, "newGame");
    check(newGame, "newGame didn't load\n");
    typedef int (__cdecl *update_t)(void*, float);
    update_t update = (update_t)GetProcAddress(gameLib, "update");
    check(update, "update didn't load\n");
    typedef const void (__cdecl *renderScene_t)(void*, Renderer*);
    renderScene_t renderScene = (renderScene_t)GetProcAddress(gameLib, "renderScene");
    check(renderScene, "renderScene didn't load\n");

    Renderer_SDL dllRenderer(renderer);

    printf("setup complete\n");

    SDL_Event event;
    bool quit = false;
    void* game = newGame(calloc);
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
                    printf("reloading\n");
                    FreeLibrary(gameLib);

                    CopyFile(dllName, copyDllName, /*failIfExists*/ false);
                    gameLib = LoadLibrary(copyDllName);
                    newGame = (newGame_t)GetProcAddress(gameLib, "newGame");
                    update = (update_t)GetProcAddress(gameLib, "update");
                    renderScene = (renderScene_t)GetProcAddress(gameLib, "renderScene");
                    break;
                case SDLK_l:
                    logging_enabled = !logging_enabled;
                    break;
                }
            }
        }

        // Update logic
        float dt = 0.01;
        update(game, dt);

        // Drawing
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
        // SDL_RenderDrawRect(renderer, nullptr);
        SDL_RenderClear(renderer);

        renderScene(game, &dllRenderer);

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

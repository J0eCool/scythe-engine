// Scythe Engine main file

#include "render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <windows.h>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

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
    typedef int (__cdecl *update_t)(float);
    update_t update = (update_t)GetProcAddress(gameLib, "update");
    check(update, "update didn't load\n");
    typedef const RenderInstr* (__cdecl *renderScene_t)();
    renderScene_t renderScene = (renderScene_t)GetProcAddress(gameLib, "renderScene");
    check(renderScene, "renderScene didn't load\n");

    printf("setup complete\n");

    SDL_Event event;
    bool quit = false;
    float t = 0;
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
        t += dt;
        update(dt);

        // Drawing
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
        // SDL_RenderDrawRect(renderer, nullptr);
        SDL_RenderClear(renderer);

        SDL_Rect rect;
        rect.x = sin(t*TAU*0.3) * 200 + 400;
        rect.y = cos(t*TAU*0.35) * 200 + 400;
        rect.w = 60;
        rect.h = 60;
        // SDL_Color({255, 0, 0, 255});
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
        SDL_Rect r2 = {rect.x+5, rect.y+5, 50, 50};
        SDL_RenderFillRect(renderer, &r2);

        SDL_SetRenderDrawColor(renderer, 0, 200, 255, 255);
        const RenderInstr* instrs = renderScene();
        uint32_t n_instrs = instrs[0];
        log("Rendering %d instructions\n", n_instrs);
        for (int i = 1; i < n_instrs+1; ++i) {
            auto instr = instrs[i];
            log("Rendering i=%d, instr=%d: ", i, instr);
            switch(instr) {
            case DrawBox:
                log("Transparent");
            case DrawRect: {
                log("DrawRect");
                SDL_Rect rect;
                rect.x = mem_itof(instrs[i+1]);
                rect.y = mem_itof(instrs[i+2]);
                rect.w = mem_itof(instrs[i+3]);
                rect.h = mem_itof(instrs[i+4]);

                i += 4;
                log(" args=%d,%d,%d,%d", rect.x, rect.y, rect.w, rect.h);
                log(" raw=%d,%d,%d,%d", instrs[i+1], instrs[i+2], instrs[i+3], instrs[i+4]);
                if (instr == DrawRect) {
                    SDL_RenderFillRect(renderer, &rect);
                } else {
                    SDL_RenderDrawRect(renderer, &rect);
                }
                break;
            }
            default: {
                log("ERROR_UNKNOWN_INSTR");
                break;
            }
            }
            log("\n");
        }
        free((void*)instrs);

        SDL_RenderPresent(renderer);

        // End-of-frame bookkeeping
        fflush(stdout);
        SDL_Delay(10);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("everything went better than expected\n");
    return 0;
}

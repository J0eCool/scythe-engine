// Scythe Engine main file

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include <windows.h>

void checkSDL(bool cond, const char* msg) {
    if (!cond) {
        printf("%s\nSDL_Error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}

int main(int argc, char** argv) {
    // for now just hardcode SDL+OpenGL
    // later on we'll really design the game-level abstractions we feel like
    // sticking with, but until then let's just get a thing up and running
    printf("Hello world\n");

    checkSDL(SDL_Init(SDL_INIT_VIDEO) >= 0, "sdl_init failed");

    int screenWidth = 800;
    int screenHeight = 600;
    SDL_Window* window = SDL_CreateWindow(
        "Scythe",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        screenWidth, screenHeight,
        SDL_WINDOW_SHOWN);
    checkSDL(window, "window creation failed");

    SDL_Surface* screen = SDL_GetWindowSurface(window);
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0xff, 0xff, 0xff));
    SDL_UpdateWindowSurface(window);

    printf("setup complete\n");

    // Load game.dll
    // first we make a copy, so that we can overwrite the original without
    // windows yelling at us
    const char* dllName = "game.dll";
    const char* copyDllName = "_copy_game.dll";
    if (!CopyFile(dllName, copyDllName, /*failIfExists*/ false)) {
        printf("!!couldn't copy game.dll\n  Error: %d\n", GetLastError());
    }
    HMODULE gameLib = LoadLibrary(copyDllName);
    if (!gameLib) {
        printf("!!well the game lib didn't load\n");
    }
    int (__cdecl *getNum)() = (int(*)())GetProcAddress(gameLib, "getNum");
    if (!getNum) {
        printf("!!getNum didn't load\n");
    }

    SDL_Event event;
    bool quit = false;
    while (!quit) {
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
                    getNum = (int(*)())GetProcAddress(gameLib, "getNum");
                    break;
                case SDLK_p:
                    printf("dynamic num is: %d\n", getNum());
                    break;
                }
            }
        }

        fflush(stdout);
        SDL_Delay(100);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("everything went better than expected\n");
    return 0;
}

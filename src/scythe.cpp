// Scythe Engine main file

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

void checkSDL(bool cond, const char* msg) {
    if (!cond) {
        printf("%s\nSDL_Error: %s", msg, SDL_GetError());
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
    fflush(stdout);

    SDL_Event event;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(100);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    printf("everything went better than expected\n");
    return 0;
}

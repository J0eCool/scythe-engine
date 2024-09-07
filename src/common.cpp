#include "common.h"

bool logging_enabled = true;
bool debug_isTracing = false;

void assert_SDL(bool cond, const char* msg) {
    if (!cond) {
        printf("Fatal Error: %s\nSDL_Error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}

float frac(float t) {
    return fmodf(t, 1.0f);
}

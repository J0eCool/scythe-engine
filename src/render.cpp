#include "render.h"

#include <stdio.h>

static_assert(sizeof(RenderInstr) == sizeof(float));
RenderInstr mem_ftoi(float x) {
    RenderInstr ret;
    memcpy(&ret, (RenderInstr*)&x, sizeof(float));
    return ret;
}
float mem_itof(RenderInstr x) {
    float ret;
    memcpy(&ret, (float*)&x, sizeof(RenderInstr));
    return ret;
}

bool did_print = false;
void print_test() {
    // having this be stateful lets me test what happens if I reload the lib
    // it indeed reloads btw
    if (!did_print) {
        printf("testing dylib shenanigans\n");
        did_print = true;
    }
}

// render.h
#pragma once

#include <cstdint>
#include <cstring>

// figure out how to abstract over this better later
enum RenderInstr : uint32_t {
    DrawRect,
    DrawBox,
};

RenderInstr mem_ftoi(float x);
float mem_itof(RenderInstr x);

void print_test();

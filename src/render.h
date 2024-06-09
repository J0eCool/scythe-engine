// render.h
#pragma once

#include <cstdint>
#include <cstring>

// figure out how to abstract over this better later
enum RenderInstr : uint32_t {
    DrawRect,
    DrawBox,
};

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

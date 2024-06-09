#include "render.h"

#include <cstring>
#include <malloc.h>
#include <vector>

extern "C" {

float t = 0.0;

__declspec(dllexport)
void update(float dt) {
    t += dt;
}

__declspec(dllexport)
const RenderInstr* renderScene() {
    std::vector<RenderInstr> render;

    render.push_back(DrawRect);
    render.push_back(mem_ftoi(75));
    render.push_back(mem_ftoi(150));
    render.push_back(mem_ftoi(75));
    render.push_back(mem_ftoi(100));
    // render.push_back((RenderInstr)50);
    // render.push_back((RenderInstr)150);
    // render.push_back((RenderInstr)75);
    // render.push_back((RenderInstr)100);

    RenderInstr* data = (RenderInstr*)calloc(render.size()+1, sizeof(RenderInstr));
    data[0] = (RenderInstr)render.size();
    memcpy(data+1, render.data(), sizeof(RenderInstr)*render.size());
    return data;
}

} // extern "C"

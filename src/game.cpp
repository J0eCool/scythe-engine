#include "render.h"

#include <cstring>
#include <malloc.h>
#include <math.h>
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

    print_test();
    
    render.push_back(DrawRect);
    render.push_back(mem_ftoi(75));
    render.push_back(mem_ftoi(150));
    render.push_back(mem_ftoi(75));
    render.push_back(mem_ftoi(100));

    render.push_back(DrawRect);
    render.push_back(mem_ftoi(sin(4*t)*300 + 350));
    render.push_back(mem_ftoi(cos(3.3*t)*100+150));
    render.push_back(mem_ftoi(40));
    render.push_back(mem_ftoi(40));

    RenderInstr* data = (RenderInstr*)calloc(render.size()+1, sizeof(RenderInstr));
    data[0] = (RenderInstr)render.size();
    memcpy(data+1, render.data(), sizeof(RenderInstr)*render.size());
    return data;
}

} // extern "C"

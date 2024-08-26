#pragma once

#include "render_sdl.h"

/// @brief abstract base class for
class Scene {
public:
    virtual void update(float dt) = 0;
    virtual void render(Renderer* renderer) = 0;
};

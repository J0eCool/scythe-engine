#include "vec.h"

Vec2i floorv(Vec2f v) {
    return { (int)floor(v.x), (int)floor(v.y) };
}
Vec2i ceilv(Vec2f v) {
    return { (int)ceil(v.x), (int)ceil(v.y) };
}

bool in_rect(Vec2 p, Vec2 rPos, Vec2 rSize) {
    return p.x >= rPos.x && p.x <= rPos.x + rSize.x &&
        p.y >= rPos.y && p.y <= rPos.y + rSize.y;
}

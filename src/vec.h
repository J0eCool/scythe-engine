#pragma once

template <typename T>
struct Vec2x {
    T x, y;
    Vec2x(T _x, T _y) : x(_x), y(_y) {}

    Vec2x operator+(Vec2x const& v) const {
        return Vec2x {x + v.x, y + v.y};
    }
};

using Vec2i = Vec2x<int>;
using Vec2f = Vec2x<float>;

using Vec2 = Vec2f;

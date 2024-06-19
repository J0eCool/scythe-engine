#pragma once

#include <math.h>

/// @brief  Storage-generic 2D vectors with common math operations and operators
/// @tparam T type of each coordinate
template <typename T>
struct Vec2x {
    T x, y;
    Vec2x() : x(0), y(0) {}
    Vec2x(T _x, T _y) : x(_x), y(_y) {}

    /// @brief Returns the magnitude of the vector
    T len() const {
        return sqrt(len2());
    }
    /// @brief Returns the magnitude of the vector, squared.
    /// Preferred in cases where we only need relative magnitude, skipping a sqrt
    T len2() const {
        return x*x + y*y;
    }

    /// @brief Normalize into a unit vector pointing in the same direction (colinear)
    /// @return Returns a new vector with len=1; if len=0 return a zero vector
    Vec2x normalized() const {
        float mag = len();
        if (mag == 0) {
            return {0, 0};
        }
        return *this / mag;
    }

    /// @brief dot product: a.x*b.x + a.y*b.y
    T dot(Vec2x const& v) const {
        return x*v.x + y*v.y;
    }
    static inline T dot(Vec2x const& a, Vec2x const& b) {
        return a.dot(b);
    }

#ifdef BIN_OP
#error "if you were using BIN_OP for something, we use it here then undefine it"
#endif

#define BIN_OP(op) \
    Vec2x operator op(Vec2x const& v) const { \
        return Vec2x {x op v.x, y op v.y};\
    } \
    Vec2x operator op(T s) const { \
        return Vec2x {x op s, y op s};\
    } \
    Vec2x& operator op##=(Vec2x const& v) { \
        x op##= v.x; \
        y op##= v.y; \
        return *this; \
    }
    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)
#undef BIN_OP
};
#define BIN_OP(op) \
    template <typename T> \
    Vec2x<T> operator op(T s, Vec2x<T> const& v) { \
        return Vec2x<T>(s op v.x, s op v.y);\
    }
BIN_OP(+)
BIN_OP(-)
BIN_OP(*)
BIN_OP(/)
#undef BIN_OP


using Vec2i = Vec2x<int>;
using Vec2f = Vec2x<float>;

using Vec2 = Vec2f;

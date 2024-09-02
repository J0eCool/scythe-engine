#pragma once

#include "common.h"

#include <math.h>

/// @brief  Storage-generic 2D vectors with common math operations and operators
/// @tparam T type of each coordinate
template <typename T>
struct Vec2x {
    T x, y;
    Vec2x() : x(0), y(0) {}
    Vec2x(T s) : x(s), y(s) {}
    Vec2x(T _x, T _y) : x(_x), y(_y) {}

    template <typename U>
    Vec2x<U> to() const {
        return Vec2x<U>{U(x), U(y)};
    }

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
    /// @return a positive scalar value 
    T dot(Vec2x const& v) const {
        return x*v.x + y*v.y;
    }
    static inline T dot(Vec2x const& a, Vec2x const& b) {
        return a.dot(b);
    }

    /// @brief 2D cross product
    /// @return a scalar value where sign indicates direction
    T cross(Vec2x const& v) const {
        return x*v.y - y*v.x;
    }
    static inline T cross(Vec2x const& a, Vec2x const& b) {
        return a.cross(b);
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
    BIN_OP(%)
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
BIN_OP(%)
#undef BIN_OP

template <typename T>
T dot(Vec2x<T> a, Vec2x<T> b) {
    return a.dot(b);
}

using Vec2i = Vec2x<int>;
using Vec2f = Vec2x<float>;

using Vec2 = Vec2f;

template <typename T>
Vec2x<T> min(Vec2x<T> a, Vec2x<T> b) {
    return {
        min(a.x, b.x),
        min(a.y, b.y)
    };
}

template <typename T>
Vec2x<T> max(Vec2x<T> a, Vec2x<T> b) {
    return {
        max(a.x, b.x),
        max(a.y, b.y)
    };
}

Vec2i floorv(Vec2f v) {
    return { (int)floor(v.x), (int)floor(v.y) };
}
Vec2i ceilv(Vec2f v) {
    return { (int)ceil(v.x), (int)ceil(v.y) };
}

struct Rect {
    Vec2 pos;
    Vec2 size;
};

/// @brief Determines whether a point is within an axis-aligned rectangle
/// @param p point to test
/// @param rPos upper-left corner of the rectangle
/// @param rSize rectangle's size
/// @return `true` iff the point is within the rect
bool in_rect(Vec2 p, Vec2 rPos, Vec2 rSize) {
    return p.x >= rPos.x && p.x <= rPos.x + rSize.x &&
        p.y >= rPos.y && p.y <= rPos.y + rSize.y;
}

/// @brief Determines whether a point is within an axis-aligned rectangle
/// @tparam R any type with Vec2 fields `pos` and `size`
/// @param p point to test
/// @param rect rectangular object
/// @return `true` iff the point is within the rect
template <typename R>
bool in_rect(Vec2 p, R rect) {
    return in_rect(p, rect.pos, rect.size);
}

// -----------------------------------------------------------------------------
// 3D vectorstuff - basically just a copy of 2D

/// @brief  Storage-generic 3D vectors with common math operations and operators
/// @tparam T type of each coordinate
template <typename T>
struct Vec3x {
    T x, y, z;
    Vec3x() : x(0), y(0), z(0) {}
    Vec3x(T s) : x(s), y(s), z(s) {}
    Vec3x(T _x, T _y, T _z=0) : x(_x), y(_y), z(_z) {}
    Vec3x(Vec2x<T> v) : x(v.x), y(v.y), z(0) {}

    template <typename U>
    Vec3x<U> to() const {
        return Vec3x<U>{U(x), U(y), U(z)};
    }

    /// @brief Returns the magnitude of the vector
    T len() const {
        return sqrt(len2());
    }
    /// @brief Returns the magnitude of the vector, squared.
    /// Preferred in cases where we only need relative magnitude, skipping a sqrt
    T len2() const {
        return x*x + y*y + z*z;
    }

    /// @brief Normalize into a unit vector pointing in the same direction (colinear)
    /// @return Returns a new vector with len=1; if len=0 return a zero vector
    Vec3x normalized() const {
        float mag = len();
        if (mag == 0) {
            return {0, 0, 0};
        }
        return *this / mag;
    }

    /// @brief dot product: a.x*b.x + a.y*b.y
    /// @return a positive scalar value 
    T dot(Vec3x const& v) const {
        return x*v.x + y*v.y + z*v.z;
    }
    static inline T dot(Vec3x const& a, Vec3x const& b) {
        return a.dot(b);
    }

    /// @brief cross product
    /// @return a vector orthogonal to its inputs
    Vec3x cross(Vec3x const& v) const {
        ///TODO: this
        assert(false, "implement me pls");
        return Vec3x{};
    }
    static inline Vec3x cross(Vec3x const& a, Vec3x const& b) {
        return a.cross(b);
    }

    // swizzling functions
    Vec2x<T> xy() const { return Vec2x<T> {x, y}; }
    Vec2x<T> xz() const { return Vec2x<T> {x, z}; }
    Vec2x<T> xx() const { return Vec2x<T> {x, x}; }
    Vec2x<T> yy() const { return Vec2x<T> {y, y}; }

#define BIN_OP(op) \
    Vec3x operator op(Vec3x const& v) const { \
        return Vec3x {x op v.x, y op v.y, z op v.z};\
    } \
    Vec3x operator op(T s) const { \
        return Vec3x {x op s, y op s, z op s};\
    } \
    Vec3x& operator op##=(Vec3x const& v) { \
        x op##= v.x; \
        y op##= v.y; \
        z op##= v.z; \
        return *this; \
    }
    BIN_OP(+)
    BIN_OP(-)
    BIN_OP(*)
    BIN_OP(/)
    BIN_OP(%)
#undef BIN_OP
};
#define BIN_OP(op) \
    template <typename T> \
    Vec3x<T> operator op(T s, Vec3x<T> const& v) { \
        return Vec3x<T>(s op v.x, s op v.y, s op v.z);\
    }
BIN_OP(+)
BIN_OP(-)
BIN_OP(*)
BIN_OP(/)
BIN_OP(%)
#undef BIN_OP

template <typename T>
T dot(Vec3x<T> a, Vec3x<T> b) {
    return a.dot(b);
}

using Vec3i = Vec3x<int>;
using Vec3f = Vec3x<float>;

using Vec3 = Vec3f;

template <typename T>
Vec3x<T> min(Vec3x<T> a, Vec3x<T> b) {
    return {
        min(a.x, b.x),
        min(a.y, b.y),
        min(a.z, b.z)
    };
}

template <typename T>
Vec3x<T> max(Vec3x<T> a, Vec3x<T> b) {
    return {
        max(a.x, b.x),
        max(a.y, b.y),
        max(a.z, b.z)
    };
}

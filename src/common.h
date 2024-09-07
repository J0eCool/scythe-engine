// common.h - this stuff gets included everywhere
#pragma once

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

static const float PI = 3.1415926535;
static const float TAU = 2*PI;

struct Allocator {
    void* (*malloc)(size_t);
    void* (*calloc)(size_t, size_t);
    void (*free)(void*);

    template <typename T, typename ...Args>
    T* knew(Args ...args) {
        T* ptr = (T*)calloc(1, sizeof(T));
        new (ptr) T(args...);
        return ptr;
    }

    template <typename T>
    void del(T* ptr) {
        ptr->~T();
        free(ptr);
    }
};

/// @brief a check is a nonfatal assert
/// @param cond the condition we hope is `true`
/// @param msg a printf-style format string to display if the check fails
/// @param ...args additional arguments to print
/// @return passthrough return of `cond`
template <typename... Ts>
bool check(bool cond, const char* msg, Ts... args) {
    if (!cond) {
        printf("Error: ");
        printf(msg, args...);
        puts("");
    }
    return cond;
}
template <typename... Ts>
void assert(bool cond, const char* msg, Ts... args) {
    if (!cond) {
        printf("Fatal Error: ");
        printf(msg, args...);
        puts("");
        exit(1);
    }
}

void assert_SDL(bool cond, const char* msg);

extern bool logging_enabled;

template <typename... Ts>
void log(const char* fmt, Ts... args) {
    if (logging_enabled) {
        printf(fmt, args...);
        puts("");
    }
}

extern bool debug_isTracing;
// for debugging crashes
class Tracer {
    const char* _label;
public:
    Tracer(const char* label) : _label(label) {
        (*this)("start");
    }
    ~Tracer() {
        (*this)("end");
    }

    template <typename... Ts>
    void operator()(const char* msg, Ts... args) {
        if (debug_isTracing) {
            printf("[trace] %s | ", _label);
            printf(msg, args...);
            puts("");
            fflush(stdout);
        }
    }
};

class Timer {
    Uint32 _start;
public:
    Timer() : _start(SDL_GetTicks()) {}

    /// @brief Time elapsed since timer creation, in milliseconds
    Uint32 elapsedMs() const {
        return SDL_GetTicks() - _start;
    }
    /// @brief Time elapsed since timer creation, in seconds
    /// (with millisecond precision)
    float elapsed() const {
        return elapsedMs() / 1000.0f;
    }
};

/// Math stuff

/// @brief Constrains a value within two bounds
/// @tparam T numeric type parameter; anything that implements `<` and `>`
/// @param val the value to clamp
/// @param lo smallest value to return
/// @param hi largest value to return
template <typename T>
T clamp(T val, T lo = 0, T hi = 1) {
    if (val < lo) {
        return lo;
    } else if (val > hi) {
        return hi;
    } else {
        return val;
    }
}

template <typename T>
T min(T a, T b) {
    if (a < b) {
        return a;
    } else {
        return b;
    }
}

template <typename T>
T max(T a, T b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

template <typename T>
T sign(T val) {
    if (val < 0) {
        return -1;
    } else if (val > 0) {
        return 1;
    } else {
        return 0;
    }
}

/// @brief Linear interpolation between two values
/// @param t how far along we're interpolating, between [0, 1] (will clamp if oob)
/// @param lo lower bound
/// @param hi upper bound
/// @return a value between [lo, hi]
template <typename T>
T lerp(float t, T lo, T hi) {
    t = clamp(t, 0.0f, 1.0f);
    return (1-t)*lo + t*hi;
}

template <typename T>
T smoothstep(float t, T lo, T hi) {
    t = clamp(t, 0.0f, 1.0f);
    // 3x^2 - 2x^3
    float s = t * t * (3 - 2*t);
    return (1-s)*lo + s*hi;
}

/// @brief The fractional part of some number
/// @return value between [0.0, 1.0]
float frac(float t);

template <typename T>
T slerp(float t, T a, T b) {
    const float eps = 0.0001; // epsilon; arbitrary small number
    t = clamp(t, 0.0f, 1.0f);
    float amag = sqrt(a.v*a.v+a.pos.len2());
    float bmag = sqrt(b.v*b.v+b.pos.len2());
    float p = acos(dot(a,b) / (amag*bmag));
    if (abs(sin(p)) < eps || abs(p) < eps) {
        return lerp(t, a, b);
    }
    if (isnan(p)) {
        return a;
    }
    auto ret = sin((1-t)*p)/sin(p)*a + sin(t*p)/sin(p)*b;
    if (ret.v <= 0) {
        return a;
    }
    if (ret.pos.x <= 0 || ret.pos.y <= 0) {
        return a;
    }
    return ret;
}

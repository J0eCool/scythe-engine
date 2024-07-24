// common.h - this stuff gets included everywhere
#pragma once

#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

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

void assert_SDL(bool cond, const char* msg) {
    if (!cond) {
        printf("Fatal Error: %s\nSDL_Error: %s\n", msg, SDL_GetError());
        exit(1);
    }
}
bool logging_enabled = true;
template <typename... Ts>
void log(const char* fmt, Ts... args) {
    if (logging_enabled) {
        printf(fmt, args...);
        puts("");
    }
}

bool debug_isTracing = false;
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

/// Math stuff

/// @brief Constrains a value between two extremes
/// @tparam T numeric type parameter; anything that implements `<` and `>`
/// @param val the value to clamp
/// @param lo smallest value to return
/// @param hi largest value to return
template <typename T>
T clamp(T val, T lo, T hi) {
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

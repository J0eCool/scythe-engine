// common.h - this stuff gets included everywhere
#pragma once

#include <stdio.h>
#include <stdlib.h>

// a check is a nonfatal assert
template <typename... Ts>
void check(bool cond, const char* msg, Ts... args) {
    if (!cond) {
        printf("Error: ");
        printf(msg, args...);
        puts("");
    }
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

bool logging_enabled = true;
template <typename... Ts>
void log(const char* fmt, Ts... args) {
    if (logging_enabled) {
        printf(fmt, args...);
        puts("");
    }
}

/// Math stuff

/// @brief Constrains a value between two extremes
/// @tparam T numeric type parameter; anything that implements `<` and `>`
/// @param val the value to clamp
/// @param lo smallest value to return
/// @param hi largets value to return
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
T sign(T val) {
    if (val < 0) {
        return -1;
    } else if (val > 0) {
        return 1;
    } else {
        return 0;
    }
}

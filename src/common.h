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
    }
}

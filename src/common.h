// common.h - this stuff gets included everywhere
#pragma once

#include <stdio.h>
#include <stdlib.h>

// a check is a nonfatal assert
void check(bool cond, const char* msg) {
    if (!cond) {
        printf("Error: %s\n");
    }
}

bool logging_enabled = true;
template <typename... Ts>
void log(const char* fmt, Ts... args) {
    if (logging_enabled) {
        printf(fmt, args...);
    }
}

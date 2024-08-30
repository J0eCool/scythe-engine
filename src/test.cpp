// the test app is always built with testing enabled
#define TESTING

#include "serialize.h"

#include <stdio.h>

int main(int argc, char** argv) {
    for (auto &tc : gTestCases) {
        printf("test %s ... ", tc.name.c_str());
        auto result = tc.testFn();
        if (result.status == Passed) {
            printf("pass\n");
        } else if (result.status == Failed) {
            printf("[FAILED]: %s\n", result.message.c_str());
        }
    }
    return 0;
}

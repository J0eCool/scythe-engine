#pragma once

#ifndef TESTING

// nop test definitions for non-test mode
#define TEST(name, ...)

#else // TESTING

#include <string>
#include <vector>

enum TestStatus {
    Passed,
    Failed,
};
struct TestResult {
    TestStatus status;
    std::string message;
};

struct TestCase {
    std::string name;
    TestResult (*testFn)();
};
std::vector<TestCase> gTestCases;
// this function gets called to give us static initializers
int push_test_case(TestCase tc) {
    gTestCases.push_back(tc);
    return gTestCases.size();
}

#define TEST(name, ...) \
    TestResult test__##name() { \
        __VA_ARGS__ \
        return TestResult { Passed, "" }; \
    } \
    static int test__##name##__id = push_test_case(TestCase {#name, test__##name});

#define TEST_EQ(actual, expected) \
    TEST_EQ_MSG(actual, expected, #actual " != " #expected)
#define TEST_EQ_MSG(actual, expected, message) \
    if ((actual) != (expected)) { \
        return TestResult { Failed, message }; \
    }

#endif // TESTING

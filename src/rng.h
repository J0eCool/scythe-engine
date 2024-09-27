#pragma once

#include "common.h"

#include <ctime>
#include <random>

class Rng {
    std::ranlux24_base _rand_engine;
public:
    Rng();

    void seed();
    void seed(uint32_t s);

    int Int(int limit = INT32_MAX);
    float Float(float limit = 1.0);
    float Float(float lo, float hi);
    Uint8 Byte();

    /// @brief Generate a random `true` or `false` value
    /// @param p probability of a `true` result
    bool Bool(float p = 0.5);

    /// @brief Approximate a normal distribution by taking the average of 4 rolls.
    /// This biases the distribution towards 0.5
    /// @return A value between 0 and 1
    float Normalish();
    float Normalish(float lo, float hi);
};

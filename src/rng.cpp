#include "rng.h"

Rng::Rng() {
}

void Rng::seed() {
    // random_device seems to give a consistent value in dll, so offset by
    // ticks elapsed since program start to get new results on each reload
    std::random_device r;
    auto seed = std::time(nullptr);
    _rand_engine = std::ranlux24_base(seed);
}
void Rng::seed(uint32_t s) {
    _rand_engine.seed(s);
}

int Rng::Int(int limit) {
    return std::uniform_int_distribution<int>(0, limit-1)(_rand_engine);
}
float Rng::Float(float limit) {
    return std::uniform_real_distribution<float>(0, limit)(_rand_engine);
}
float Rng::Float(float lo, float hi) {
    return lerp(Float(), lo, hi);
}
Uint8 Rng::Byte() {
    return Int(0x100);
}
bool Rng::Bool(float p) {
    return Float() < p;
}
float Rng::Normalish() {
    return (Float() + Float() + Float() + Float()) / 4;
}
float Rng::Normalish(float a, float b) {
    return (Float(a,b) + Float(a,b) + Float(a,b) + Float(a,b)) / 4;
}
    

#pragma once

#include "color.h"
#include "vec.h"

#include <fstream>

// data per grid cell
struct NoiseSample {
    float v; // scalar noise value
    Color color; // RGB color data
    Vec2f pos; // offset within the grid cell, expected to be in [0, 1)
};
NoiseSample operator*(float s, NoiseSample n) {
    return { s*n.v, s*n.color, s*n.pos };
}
NoiseSample operator+(NoiseSample a, NoiseSample b) {
    return { a.v+b.v, a.color+b.color, a.pos+b.pos };
}
// ok this one's pretty dubious... used to let us slerp noise samples
float dot(NoiseSample a, NoiseSample b) {
    // I mean in principle this isn't too bad
    // we're computing a measure of how well-aligned two samples are in a
    // 3D space of x,y,v
    return a.v*b.v + dot(a.pos, b.pos);
}

struct TexParams {
    uint32_t seed;

    // params afffecting generation
    int noiseSize = 11; // generate an NxN grid of samples
    int numTextures = 8; // generate N texture variations
    // `mode` is how to display the noise data
    //   0 - display each grid cell using the upper-left point's color
    //   1 - interpolate between the color values of each
    //   2 - map noise value onto a color gradient
    //   3 - map noise value onto a grayscale gradient
    //   4 - same as mode 3, but color-coded per tile variant
    //   5 - gradient-mapped color values (soon the default)
    int mode = 5;
    int noiseScale = 2; // range of the scalar noise values
    int texSize = 32; // NxN pixel size of generated texture
    Gradient gradient;

    float gradAnimScale = 0;
    float noiseAnimScale = 0;
    float tileAnimScale = 0;
};
std::ostream& operator<<(std::ostream &stream, TexParams const& params) {
    return stream
        << params.seed << '\n'
        << params.noiseSize << '\n'
        << params.numTextures << '\n'
        << params.mode << '\n'
        << params.noiseScale << '\n'
        << params.texSize << '\n'
        << params.gradient << '\n'
        << params.gradAnimScale << '\n'
        << params.noiseAnimScale << '\n'
        << params.tileAnimScale << '\n'
        ;
}
std::istream& operator>>(std::istream &stream, TexParams &params) {
    return stream
        >> params.seed
        >> params.noiseSize
        >> params.numTextures
        >> params.mode
        >> params.noiseScale
        >> params.texSize
        >> params.gradient
        >> params.gradAnimScale
        >> params.noiseAnimScale
        >> params.tileAnimScale
        ;
}

class TexGen {
    ///TODO: put texture generation logic here
};

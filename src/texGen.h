#pragma once

#include "color.h"
#include "serialize.h"
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
Serialize<TexParams> serialize(TexParams &params) {
    Serialize<TexParams> serial(params);
    serial.addField("seed", params.seed);
    serial.addField("noiseSize", params.noiseSize);
    serial.addField("numTextures", params.numTextures);
    serial.addField("mode", params.mode);
    serial.addField("noiseScale", params.noiseScale);
    serial.addField("texSize", params.texSize);
    serial.addField("gradient", params.gradient);
    serial.addField("gradAnimScale", params.gradAnimScale);
    serial.addField("noiseAnimScale", params.noiseAnimScale);
    serial.addField("tileAnimScale", params.tileAnimScale);
    return serial;
}

class TexGen {
    ///TODO: put texture generation logic here
};

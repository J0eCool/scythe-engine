#include "texGen.h"

NoiseSample operator*(float s, NoiseSample n) {
    return { s*n.v, s*n.color, s*n.pos };
}
NoiseSample operator+(NoiseSample a, NoiseSample b) {
    return { a.v+b.v, a.color+b.color, a.pos+b.pos };
}

float dot(NoiseSample a, NoiseSample b) {
    // I mean in principle this isn't too bad
    // we're computing a measure of how well-aligned two samples are in a
    // 3D space of x,y,v
    return a.v*b.v + dot(a.pos, b.pos);
}

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

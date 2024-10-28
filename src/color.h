#pragma once

#include "common.h"

#include <math.h>
#include <fstream>
#include <vector>

struct Color {
    Uint8 r, g, b, a;

    Color() : r(0), g(0), b(0), a(0) {}
    Color(Uint8 _r, Uint8 _g, Uint8 _b, Uint8 _a = 0xff)
        : r(_r), g(_g), b(_b), a(_a) {}

    Color operator+(Color c) const {
        return {Uint8(r+c.r), Uint8(g+c.g), Uint8(b+c.b), Uint8(a+c.a)};
    }
    Color operator*(Color c) const {
        return {Uint8(r*c.r), Uint8(g*c.g), Uint8(b*c.b), Uint8(a*c.a)};
    }
    Color operator*(float s) const {
        return {Uint8(r*s), Uint8(g*s), Uint8(b*s), Uint8(a*s)};
    }

    static const Color black;
    static const Color white;
    static const Color red;
    static const Color green;
    static const Color blue;
};

Color operator*(float s, Color c);
Color rgbColor(float r, float g, float b, float a = 1.0f);

Color hsvColor(float h, float s, float v);
std::ostream& operator<<(std::ostream &stream, Color const& c);
std::istream& operator>>(std::istream &stream, Color &c);

struct GradientStep {
    Color color;
    float pos; // [0, 1], what value of the gradient is set to this color
};

bool operator<(GradientStep a, GradientStep b);

struct Gradient {
    std::vector<GradientStep> steps;

    Gradient() {
        steps.push_back({Color::black, 0});
        steps.push_back({Color::white, 1});
    }

    Color sample(float t) {
        t = frac(t);
        // we assume that the steps are sorted
        int i = 0;
        for (; i < steps.size()-1; ++i) {
            if (steps[i+1].pos >= t) {
                break;
            }
        }
        float s = (t-steps[i].pos) / (steps[i+1].pos-steps[i].pos);
        return lerp(s, steps[i].color, steps[i+1].color);
    }

    // todo:
    /// @brief Ensures steps are monotonically increasing in pos by sorting them
    // void sort();
};

std::ostream& operator<<(std::ostream &stream, Gradient const& gradient);
std::istream& operator>>(std::istream &stream, Gradient &gradient);

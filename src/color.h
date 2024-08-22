#pragma once

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
const Color Color::black { 0x00, 0x00, 0x00 };
const Color Color::white { 0xff, 0xff, 0xff };
const Color Color::red   { 0xff, 0x00, 0x00 };
const Color Color::green { 0x00, 0xff, 0x00 };
const Color Color::blue  { 0x00, 0x00, 0xff };

Color operator*(float s, Color c) {
    return c*s;
}

Color rgbColor(float r, float g, float b, float a = 1.0f) {
    return {Uint8(0xff*r), Uint8(0xff*g), Uint8(0xff*b), Uint8(0xff*a)};
}

Color hsvColor(float h, float s, float v) {
    // algorithm from https://www.rapidtables.com/convert/color/hsv-to-rgb.html
    const float c = v*s;
    const float x = c*(1 - fabs(fmod(h/60, 2) - 1));
    const float m = v - c;
    Color rgb;
    h = fmod(h, 360);
    if (h < 60)       { rgb = rgbColor(c, x, 0); }
    else if (h < 120) { rgb = rgbColor(x, c, 0); }
    else if (h < 180) { rgb = rgbColor(0, c, x); }
    else if (h < 240) { rgb = rgbColor(0, x, c); }
    else if (h < 300) { rgb = rgbColor(x, 0, c); }
    else              { rgb = rgbColor(c, 0, x); }
    rgb.r += m;
    rgb.g += m;
    rgb.b += m;
    return rgb;
}
std::ostream& operator<<(std::ostream &stream, Color const& c) {
    return stream << (int32_t)c.r << ' ' << (int32_t)c.g << ' ' << (int32_t)c.b << ' ' << (int32_t)c.a;
}
std::istream& operator>>(std::istream &stream, Color &c) {
    int32_t r, g, b, a;
    stream >> r >> g >> b >> a;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return stream;
}

struct GradientStep {
    Color color;
    float pos; // [0, 1], what value of the gradient is set to this color
};

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
};
std::ostream& operator<<(std::ostream &stream, Gradient const& gradient) {
    stream << gradient.steps.size() << ' ';
    for (auto &step : gradient.steps) {
        stream << step.color << ' ' << step.pos << '\n';
    }
    return stream;
}
std::istream& operator>>(std::istream &stream, Gradient &gradient) {
    int nSteps;
    stream >> nSteps;
    gradient.steps.clear();
    for (int i = 0; i < nSteps; ++i) {
        GradientStep step;
        stream >> step.color >> step.pos;
        gradient.steps.push_back(step);
    }
    return stream;
}

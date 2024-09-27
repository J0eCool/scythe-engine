#include "ui.h"

template <>
float sliderLerp<float>(float t, float lo, float hi) {
    return lerp(t, lo, hi);
}

/* -*- C++ -*-
 *
 *  This file is part of RawTherapee.
 *
 *  Halation: the red-orange glow film produces around bright highlights when
 *  light bounces off the film base back into the red-sensitive layer.
 *  Implemented as a thresholded, blurred, tinted copy of the highlights added
 *  back onto the linear RGB image, before exposure and tone curves.
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <algorithm>
#include <cmath>

#include "array2D.h"
#include "gauss.h"
#include "imagefloat.h"
#include "improcfun.h"
#include "procparams.h"
#include "rt_math.h"

namespace rtengine
{

namespace
{

// hue in degrees, full saturation / value -> rgb weights in [0,1]
void hueToRgb(float hue, float& r, float& g, float& b)
{
    hue = std::fmod(std::fmod(hue, 360.f) + 360.f, 360.f) / 60.f;
    const int i = static_cast<int>(hue);
    const float f = hue - i;

    switch (i) {
        case 0:  r = 1.f;     g = f;       b = 0.f;     break;
        case 1:  r = 1.f - f; g = 1.f;     b = 0.f;     break;
        case 2:  r = 0.f;     g = 1.f;     b = f;       break;
        case 3:  r = 0.f;     g = 1.f - f; b = 1.f;     break;
        case 4:  r = f;       g = 0.f;     b = 1.f;     break;
        default: r = 1.f;     g = 0.f;     b = 1.f - f; break;
    }
}

} // namespace

void ImProcFunctions::halation(Imagefloat* img, const procparams::HalationParams& p)
{
    if (!p.enabled || (p.strength <= 0 && p.bloom <= 0) || !img) {
        return;
    }

    const int W = img->getWidth();
    const int H = img->getHeight();

    if (W < 2 || H < 2) {
        return;
    }

    // Threshold is perceptual (display brightness), the data here is linear.
    const float thr = LIM(static_cast<float>(p.threshold) / 100.f, 0.f, 0.98f);
    const float invRange = 1.f / (1.f - thr);
    const float amountH = 1.2f * static_cast<float>(p.strength) / 100.f;
    const float amountB = 1.2f * static_cast<float>(p.bloom) / 100.f;
    const double sc = std::max(scale, 1.0);
    const double sigmaH = std::max(0.5, static_cast<double>(p.radius) / sc);
    const double sigmaB = std::max(0.5, static_cast<double>(p.bloomRadius) / sc);

    float cr, cg, cb;
    hueToRgb(static_cast<float>(p.hue), cr, cg, cb);

    array2D<float> glow(W, H);
    array2D<float> blurred(W, H);

#ifdef _OPENMP
    #pragma omp parallel for if (multiThread)
#endif
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            const float l = std::max(0.2126f * img->r(y, x) + 0.7152f * img->g(y, x) + 0.0722f * img->b(y, x), 0.f) / 65535.f;
            const float lp = std::pow(l, 1.f / 2.2f);
            const float m = std::max(lp - thr, 0.f) * invRange;
            glow[y][x] = m * m * l; // soft knee, weighted by actual light so brighter sources glow more
        }
    }

    if (amountH > 0.f) {
#ifdef _OPENMP
        #pragma omp parallel if (multiThread)
#endif
        {
            gaussianBlur(glow, blurred, W, H, sigmaH);
        }

#ifdef _OPENMP
        #pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float g = blurred[y][x] * amountH * 65535.f;
                img->r(y, x) += g * cr;
                img->g(y, x) += g * cg;
                img->b(y, x) += g * cb;
            }
        }
    }

    if (amountB > 0.f) {
#ifdef _OPENMP
        #pragma omp parallel if (multiThread)
#endif
        {
            gaussianBlur(glow, blurred, W, H, sigmaB);
        }

#ifdef _OPENMP
        #pragma omp parallel for if (multiThread)
#endif
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const float g = blurred[y][x] * amountB * 65535.f;
                img->r(y, x) += g;
                img->g(y, x) += g;
                img->b(y, x) += g;
            }
        }
    }
}

} // namespace rtengine

/*
Copyright (c) 2020 Electrosmith, Corp, Emilie Gillet

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

/** Helpful defines, functions, and other utilities for use in/with daisysp modules.
*/
#pragma once
#ifndef DSY_CORE_DSP
#define DSY_CORE_DSP
#include <cassert>
#include <cstdint>
#include <random>
#include <cmath>

/** PIs
*/
#define PI_F 3.1415927410125732421875f
#define TWOPI_F (2.0f * PI_F)
#define HALFPI_F (PI_F * 0.5f)
#define DSY_MIN(in, mn) (in < mn ? in : mn)
#define DSY_MAX(in, mx) (in > mx ? in : mx)
#define DSY_CLAMP(in, mn, mx) (DSY_MIN(DSY_MAX(in, mn), mx))
#define DSY_COUNTOF(_arr) (sizeof(_arr) / sizeof(_arr[0]))

namespace daisysp
{
//Avoids division for random floats. e.g. rand() * kRandFrac
static constexpr float kRandFrac = 1.f / (float)RAND_MAX;

//Convert from semitones to other units. e.g. 2 ^ (kOneTwelfth * x)
static constexpr float kOneTwelfth = 1.f / 12.f;

/** efficient floating point min/max
c/o stephen mccaul
*/
inline float fmax(float a, float b)
{
    float r;
#ifdef __arm__
    asm("vmaxnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a > b) ? a : b;
#endif // __arm__
    return r;
}

inline float fmin(float a, float b)
{
    float r;
#ifdef __arm__
    asm("vminnm.f32 %[d], %[n], %[m]" : [d] "=t"(r) : [n] "t"(a), [m] "t"(b) :);
#else
    r = (a < b) ? a : b;
#endif // __arm__
    return r;
}

/** quick fp clamp
*/
inline float fclamp(float in, float min, float max)
{
    return fmin(fmax(in, min), max);
}

/** From Musicdsp.org "Fast power and root estimates for 32bit floats)
Original code by Stefan Stenzel
These are approximations
*/
inline float fastpower(float f, int n)
{
    long *lp, l;
    lp = (long *)(&f);
    l  = *lp;
    l -= 0x3F800000;
    l <<= (n - 1);
    l += 0x3F800000;
    *lp = l;
    return f;
}

inline float fastroot(float f, int n)
{
    long *lp, l;
    lp = (long *)(&f);
    l  = *lp;
    l -= 0x3F800000;
    l >>= (n - 1);
    l += 0x3F800000;
    *lp = l;
    return f;
}

/** Significantly more efficient than fmodf(x, 1.0f) for calculating
 *  the decimal part of a floating point value.
 */
inline float fastmod1f(float x)
{
    return x - floorf(x);
}

/** From http://openaudio.blogspot.com/2017/02/faster-log10-and-pow.html
No approximation, pow10f(x) gives a 90% speed increase over powf(10.f, x)
*/
inline float pow10f(float f)
{
    return expf(2.302585092994046f * f);
}

/* Original code for fastlog2f by Dr. Paul Beckmann from the ARM community forum, adapted from the CMSIS-DSP library
About 25% performance increase over std::log10f
*/
inline float fastlog2f(float f)
{
    float frac;
    int   exp;
    frac = frexpf(fabsf(f), &exp);
    f    = 1.23149591368684f;
    f *= frac;
    f += -4.11852516267426f;
    f *= frac;
    f += 6.02197014179219f;
    f *= frac;
    f += -3.13396450166353f;
    f += exp;
    return (f);
}

inline float fastlog10f(float f)
{
    return fastlog2f(f) * 0.3010299956639812f;
}

/** Midi to frequency helper
*/
inline float mtof(float m)
{
    return powf(2, (m - 69.0f) / 12.0f) * 440.0f;
}


/** one pole lpf
out is passed by reference, and must be retained between
calls to properly filter the signal
coeff can be calculated:
coeff = 1.0 / (time * sample_rate) ; where time is in seconds
*/
inline void fonepole(float &out, float in, float coeff)
{
    out += coeff * (in - out);
}

/** Curves to use with the fmap function */
enum class Mapping
{
    LINEAR,
    EXP,
    LOG,
};

/** Maps a float between a specified range, using a specified curve.
 *
 *  \param in a value between 0 to 1 that will be mapped to the new range.
 *  \param min the new minimum value
 *  \param max the new maxmimum value
 *  \param curve a Mapping Value to adjust the response curve of the transformation
 *               defaults to Linear. @see Mapping
 *
 *  When using the log curve min and max, must be greater than zero.
 *
 *  \retval returns the transformed float within the new range.
*/
inline float
fmap(float in, float min, float max, Mapping curve = Mapping::LINEAR)
{
    switch(curve)
    {
        case Mapping::EXP:
            return fclamp(min + (in * in) * (max - min), min, max);
        case Mapping::LOG:
        {
            const float a = 1.f / log10f(max / min);
            return fclamp(min * powf(10, in / a), min, max);
        }
        case Mapping::LINEAR:
        default: return fclamp(min + in * (max - min), min, max);
    }
}

/** Simple 3-point median filter
c/o stephen mccaul
*/
template <typename T>
T median(T a, T b, T c)
{
    return (b < a) ? (b < c) ? (c < a) ? c : a : b
                   : (a < c) ? (c < b) ? c : b : a;
}

/** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
*/
inline float ThisBlepSample(float t)
{
    return 0.5f * t * t;
}

/** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
*/
inline float NextBlepSample(float t)
{
    t = 1.0f - t;
    return -0.5f * t * t;
}

/** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
*/
inline float NextIntegratedBlepSample(float t)
{
    const float t1 = 0.5f * t;
    const float t2 = t1 * t1;
    const float t4 = t2 * t2;
    return 0.1875f - t1 + 1.5f * t2 - t4;
}

/** Ported from pichenettes/eurorack/plaits/dsp/oscillator/oscillator.h
*/
inline float ThisIntegratedBlepSample(float t)
{
    return NextIntegratedBlepSample(1.0f - t);
}

/** Soft Limiting function ported extracted from pichenettes/stmlib */
inline float SoftLimit(float x)
{
    return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

/** Soft Clipping function extracted from pichenettes/stmlib */
inline float SoftClip(float x)
{
    if(x < -3.0f)
        return -1.0f;
    else if(x > 3.0f)
        return 1.0f;
    else
        return SoftLimit(x);
}

/** Quick check for Invalid float values (NaN, Inf, out of range)
 ** \param x value passed by reference, replaced by y if invalid.
 ** \param y value to replace x if invalidity is found.
 **
 ** When DEBUG is true in the build, this will halt
 ** execution for tracing the reason for the invalidity. */
inline void TestFloat(float &x, float y = 0.f)
{
    if(!std::isnormal(x) && x != 0)
    {
#if defined(__arm__) && defined(DEBUG)
        asm("bkpt 255");
#else
        x = y;
#endif
    }
}

/** Based on soft saturate from:
[musicdsp.org](musicdsp.org/en/latest/Effects/42-soft-saturation.html)
with 0 <= thresh <= 1
Bram de Jong (2002-01-17)
described as:
x <= a:
     f(x) = x
x > a:
     f(x) = a + (x-a)/(1+((x-a)/(1-a))^2)
x >= 1:
     f(x) = (a + 1)/2
*/
inline float soft_saturate(float in, float thresh)
{
    bool  flip;
    float val, out;
    //val = fabsf(in);
    out  = 0.f;
    flip = in < 0.0f;
    val  = flip ? -in : in;
    if(val <= thresh)
    {
        out = in;
    }
    else if(val >= 1.0f)
    {
        out = (thresh + 1.0f) / 2.0f;
        if(flip)
            out *= -1.0f;
    }
    else if(val > thresh)
    {
        float temp;
        temp = (val - thresh) / (1 - thresh);
        out  = thresh + (val - thresh) / (1.0f + (temp * temp));
        if(flip)
            out *= -1.0f;
    }
    return out;
}

constexpr bool is_power2(uint32_t x)
{
    return ((x - 1) & x) == 0;
}

/** Prior to C++14 constexpr functions were required to be a single return statement.
 *  So this clause guards against that behavior to allow the library, and this function
 *  to continue to work with C++11.
 *  The function itself is not currently (12 May 2021) used within the library itself.
 */
#if __cplusplus <= 201103L
inline uint32_t get_next_power2(uint32_t x)
#else
constexpr uint32_t get_next_power2(uint32_t x)
#endif
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;

    assert(is_power2(x));
    return x;
}

} // namespace daisysp
#endif

#ifdef DSY_CUSTOM_DSP
#include "custom_dsp.h"
#endif

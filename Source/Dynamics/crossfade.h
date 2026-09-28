/*
Copyright (c) 2020 Electrosmith, Corp, Paul Batchelor

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#pragma once
#ifndef DSY_CROSSFADE_H
#define DSY_CROSSFADE_H
#include <stdint.h>
#ifdef __cplusplus

namespace daisysp
{
/** Curve applied to the CrossFade
- LIN = linear
- CPOW = constant power
- LOG = logarithmic
- EXP  exponential
- LAST = end of enum (used for array indexing)
*/
enum
{
    CROSSFADE_LIN,
    CROSSFADE_CPOW,
    CROSSFADE_LOG,
    CROSSFADE_EXP,
    CROSSFADE_LAST,
};

/** Performs a CrossFade between two signals

    Original author: Paul Batchelor

    Ported from Soundpipe by Andrew Ikenberry

    added curve option for constant power, etc.
*/
class CrossFade
{
  public:
    CrossFade() {}
    ~CrossFade() {}
    /** Initializes CrossFade module
        Defaults
        - current position = .5
        - curve = linear
    */
    inline void Init(int curve)
    {
        pos_   = 0.5f;
        curve_ = curve < CROSSFADE_LAST ? curve : CROSSFADE_LIN;
    }

    /** Initialize with default linear curve 
    */
    inline void Init() { Init(CROSSFADE_LIN); }
    /** processes CrossFade and returns single sample
    */
    float Process(float &in1, float &in2);


    /** Sets position of CrossFade between two input signals
        Input range: 0 to 1
    */
    inline void SetPos(float pos) { pos_ = pos; }
    /** Sets current curve applied to CrossFade 
    Expected input: See [Curve Options](##curve-options)
    */
    inline void SetCurve(uint8_t curve) { curve_ = curve; }
    /** Returns current position
    */
    inline float GetPos() { return pos_; }
    /** Returns current curve
    */
    inline uint8_t GetCurve() { return curve_; }

  private:
    float   pos_;
    uint8_t curve_;
};
} // namespace daisysp
#endif
#endif

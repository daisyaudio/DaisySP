/*
Copyright (c) 2020 Electrosmith, Corp

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#pragma once
#ifndef ADENV_H
#define ADENV_H
#include <stdint.h>
#ifdef __cplusplus

namespace daisysp
{
/** Distinct stages that the phase of the envelope can be located in.
    @see AdEnv
*/
enum AdEnvSegment
{
    /** located at phase location 0, and not currently running */
    ADENV_SEG_IDLE,
    /** First segment of envelope where phase moves from MIN value to MAX value */
    ADENV_SEG_ATTACK,
    /** Second segment of envelope where phase moves from MAX to MIN value */
    ADENV_SEG_DECAY,
    /** The final segment of the envelope (currently decay) */
    ADENV_SEG_LAST,
};

/** Trigger-able envelope with adjustable min/max, and independent per-segment time control.

    \author shensley
    \todo - Add Cycling
    \todo - Implement Curve (its only linear for now).
    \todo - Maybe make this an ADsr_ that has AD/AR/Asr_ modes.
 */
class AdEnv
{
  public:
    AdEnv() {}
    ~AdEnv() {}
    /** Initializes the ad envelope.

        Defaults:
            - current segment = idle
            - curve = linear
            - phase = 0
            - min = 0
            - max = 1

        \param sample_rate      sample rate of the audio engine being run
    */
    void Init(float sample_rate);

    /** Processes the current sample of the envelope. This should be called once
        per sample period.
        \return the current envelope value.
    */
    float Process();

    /** Starts or retriggers the envelope. Default is a non-hard retrigger.*/
    inline void Trigger()
    {
        trigger_      = true;
        hard_trigger_ = false;
    }
    /** Starts or retriggers the envelope. If hard is true the envelope re-starts
        from 0, if hard is false it starts from the current envelope value. */
    inline void Trigger(bool hard)
    {
        trigger_      = true;
        hard_trigger_ = hard;
    }
    /** Sets the length of time (in seconds) for a specific segment. */
    inline void SetTime(uint8_t seg, float time) { segment_time_[seg] = time; }
    /** Sets the amount of curve applied. A positve value will create a log
        curve. Input range: -100 to 100.  (or more)
    */
    inline void SetCurve(float scalar) { curve_scalar_ = scalar; }
    /** Sets the minimum value of the envelope output.
        Input range: -FLTmax_, to FLTmax_
    */
    inline void SetMin(float min) { min_ = min; }
    /** Sets the maximum value of the envelope output.
        Input range: -FLTmax_, to FLTmax_
    */
    inline void SetMax(float max) { max_ = max; }
    /** Returns the current output value without processing the next sample */
    inline float GetValue() const { return (output_ * (max_ - min_)) + min_; }
    /** Returns the segment of the envelope that the phase is currently located
        in.
    */
    inline uint8_t GetCurrentSegment() { return current_segment_; }
    /** Returns true if the envelope is currently in any stage apart from idle.
    */
    inline bool IsRunning() const { return current_segment_ != ADENV_SEG_IDLE; }

  private:
    uint8_t  current_segment_, prev_segment_;
    float    segment_time_[ADENV_SEG_LAST];
    float    sample_rate_, min_, max_, output_, curve_scalar_;
    float    c_inc_, curve_x_, retrig_val_;
    uint32_t phase_;
    uint8_t  trigger_, hard_trigger_;
};

} // namespace daisysp
#endif
#endif

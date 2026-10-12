/*
Copyright (c) 2020 Electrosmith, Corp

Use of this source code is governed by an MIT-style
license that can be found in the LICENSE file or at
https://opensource.org/licenses/MIT.
*/

#pragma once
#ifndef DSY_OSCILLATOR_H
#define DSY_OSCILLATOR_H
#include <stdint.h>
#include "Utility/dsp.h"
#ifdef __cplusplus

namespace daisysp
{
/** Synthesis of several waveforms, including polyBLEP bandlimited waveforms.
*/
class Oscillator
{
  public:
    Oscillator() {}
    ~Oscillator() {}
    /** Choices for output waveforms, POLYBLEP are appropriately labeled. Others are naive forms.
    */
    enum
    {
        WAVE_SIN,
        WAVE_TRI,
        WAVE_SAW,
        WAVE_RAMP,
        WAVE_SQUARE,
        WAVE_POLYBLEP_TRI,
        WAVE_POLYBLEP_SAW,
        WAVE_POLYBLEP_SQUARE,
        WAVE_LAST,
    };


    /** Initializes the Oscillator

        \param sample_rate - sample rate of the audio engine being run, and the frequency that the Process function will be called.

        Defaults:
        - freq_ = 100 Hz
        - amp_ = 0.5
        - waveform_ = sine wave.
    */
    void Init(float sample_rate)
    {
        sr_                     = sample_rate;
        sr_recip_               = 1.0f / sample_rate;
        freq_                   = 100.0f;
        amp_                    = 0.5f;
        pw_                     = 0.5f;
        phase_                  = 0.0f;
        phase_inc_              = CalcPhaseInc(freq_);
        waveform_               = WAVE_SIN;
        eoc_                    = true;
        eor_                    = true;
        eoc_frac_               = 0.0f;
        sync_corr_              = 0.0f;
        skip_builtin_polyblep_  = false;
    }


    /** Changes the frequency of the Oscillator, and recalculates phase increment.
    */
    inline void SetFreq(const float f)
    {
        freq_      = f;
        phase_inc_ = CalcPhaseInc(f);
    }


    /** Sets the amplitude of the waveform.
    */
    inline void SetAmp(const float a) { amp_ = a; }
    /** Sets the waveform to be synthesized by the Process() function.
    */
    inline void SetWaveform(const uint8_t wf)
    {
        waveform_ = wf < WAVE_LAST ? wf : WAVE_SIN;
    }
    /** Sets the pulse width for WAVE_SQUARE and WAVE_POLYBLEP_SQUARE (range 0 - 1)
     */
    inline void SetPw(const float pw) { pw_ = fclamp(pw, 0.0f, 1.0f); }

    /** Returns true if cycle is at end of rise. Set during call to Process.
    */
    inline bool IsEOR() { return eor_; }

    /** Returns true if cycle is at end of cycle. Set during call to Process.
    */
    inline bool IsEOC() { return eoc_; }

    /** When IsEOC() is true, returns the fractional position within the
        just-processed sample at which the phase wrap occurred.
        0.0 = at the very start of the sample period, 1.0 = at the end.
        Used by a slave oscillator's SyncReset(frac) for sub-sample-accurate
        (BLEP-corrected) hard sync.
    */
    inline float GetEocFraction() const { return eoc_frac_; }

    /** Returns true if cycle rising.
    */
    inline bool IsRising() { return phase_ < 0.5f; }

    /** Returns true if cycle falling.
    */
    inline bool IsFalling() { return phase_ >= 0.5f; }

    /** Processes the waveform to be generated, returning one sample. This should be called once per sample period.
    */
    float Process();


    /** Adds a value 0.0-1.0 (equivalent to 0.0-TWO_PI) to the current phase. Useful for PM and "FM" synthesis.
    */
    void PhaseAdd(float _phase) { phase_ += _phase; }
    /** Resets the phase to the input argument. If no argumeNt is present, it will reset phase to 0.0;
    */
    void Reset(float _phase = 0.0f) { phase_ = _phase; }

    /** BLEP-corrected hard sync. `frac` is the sub-sample position (0..1)
        at which the master wrapped, typically obtained from
        `master.GetEocFraction()`. Produces a band-limited reset: the slave's
        phase is advanced as if it had been reset at the correct sub-sample
        instant, and a polyblep residual is stored to be applied to the next
        Process() output. Suppresses the built-in polyblep on that sample
        since its assumed step magnitude would be incorrect for a sync event.

        Intended usage:
            float m = master.Process();
            if(master.IsEOC()) slave.SyncReset(master.GetEocFraction());
            float s = slave.Process();
    */
    void SyncReset(float frac);

  private:
    float   CalcPhaseInc(float f);
    float   NaiveWaveformValue(float phase) const;
    uint8_t waveform_;
    float   amp_, freq_, pw_;
    float   sr_, sr_recip_, phase_, phase_inc_;
    float   last_out_, last_freq_;
    float   eoc_frac_, sync_corr_;
    bool    eor_, eoc_;
    bool    skip_builtin_polyblep_;
};
} // namespace daisysp
#endif
#endif

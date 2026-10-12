#include "dsp.h"
#include "oscillator.h"

using namespace daisysp;
static inline float Polyblep(float phase_inc, float t);

float Oscillator::Process()
{
    float out, t;
    switch(waveform_)
    {
        case WAVE_SIN: out = sinf(phase_ * TWOPI_F); break;
        case WAVE_TRI:
            t   = -1.0f + (2.0f * phase_);
            out = 2.0f * (fabsf(t) - 0.5f);
            break;
        case WAVE_SAW: out = -1.0f * (((phase_ * 2.0f)) - 1.0f); break;
        case WAVE_RAMP: out = ((phase_ * 2.0f)) - 1.0f; break;
        case WAVE_SQUARE: out = phase_ < pw_ ? (1.0f) : -1.0f; break;
        case WAVE_POLYBLEP_TRI:
            t   = phase_;
            out = phase_ < 0.5f ? 1.0f : -1.0f;
            if(!skip_builtin_polyblep_)
            {
                out += Polyblep(phase_inc_, t);
                out -= Polyblep(phase_inc_, fastmod1f(t + 0.5f));
            }
            // Leaky Integrator:
            // y[n] = A + x[n] + (1 - A) * y[n-1]
            out       = phase_inc_ * out + (1.0f - phase_inc_) * last_out_;
            last_out_ = out;
            out *= 4.f; // normalize amplitude after leaky integration
            break;
        case WAVE_POLYBLEP_SAW:
            t   = phase_;
            out = (2.0f * t) - 1.0f;
            if(!skip_builtin_polyblep_)
                out -= Polyblep(phase_inc_, t);
            out *= -1.0f;
            break;
        case WAVE_POLYBLEP_SQUARE:
            t   = phase_;
            out = phase_ < pw_ ? 1.0f : -1.0f;
            if(!skip_builtin_polyblep_)
            {
                out += Polyblep(phase_inc_, t);
                out -= Polyblep(phase_inc_, fastmod1f(t + (1.0f - pw_)));
            }
            out *= 0.707f; // ?
            break;
        default: out = 0.0f; break;
    }

    // Apply any pending sync-BLEP residual (set by SyncReset(frac) on the
    // previous sample). Consumed in one shot.
    out += sync_corr_;
    sync_corr_             = 0.0f;
    skip_builtin_polyblep_ = false;

    phase_ += phase_inc_;
    if(phase_ > 1.0f)
    {
        phase_ -= 1.0f;
        eoc_ = true;
        // Fractional position within this sample at which the wrap occurred.
        // Derivation: before advance, phase was p_before = phase_ + 1 - inc.
        // Wrap time f satisfies p_before + f*inc = 1, so
        //   f = (1 - p_before) / inc = 1 - phase_/inc.
        eoc_frac_ = (phase_inc_ > 0.0f) ? (1.0f - phase_ / phase_inc_) : 0.0f;
    }
    else
    {
        eoc_ = false;
    }
    eor_ = (phase_ - phase_inc_ < 0.5f && phase_ >= 0.5f);

    return out * amp_;
}

float Oscillator::NaiveWaveformValue(float phase) const
{
    float t;
    switch(waveform_)
    {
        case WAVE_SIN: return sinf(phase * TWOPI_F);
        case WAVE_TRI:
            t = -1.0f + (2.0f * phase);
            return 2.0f * (fabsf(t) - 0.5f);
        case WAVE_SAW: return -1.0f * ((phase * 2.0f) - 1.0f);
        case WAVE_RAMP: return (phase * 2.0f) - 1.0f;
        case WAVE_SQUARE: return phase < pw_ ? 1.0f : -1.0f;
        // For the polyblep variants we return the *pre-correction* naive value
        // in the same sign/scale convention as Process()'s output (minus the
        // polyblep residual and, for TRI, minus the leaky integrator).
        case WAVE_POLYBLEP_TRI: return phase < 0.5f ? 1.0f : -1.0f;
        case WAVE_POLYBLEP_SAW: return -1.0f * ((2.0f * phase) - 1.0f);
        case WAVE_POLYBLEP_SQUARE:
            return (phase < pw_ ? 1.0f : -1.0f) * 0.707f;
        default: return 0.0f;
    }
}

void Oscillator::SyncReset(float frac)
{
    // Clamp frac into [0, 1) defensively.
    if(frac < 0.0f) frac = 0.0f;
    if(frac > 1.0f) frac = 1.0f;

    // Slave's pre-sync continuous phase at the instant sync fired.
    float pre_phase = phase_ + frac * phase_inc_;
    if(pre_phase >= 1.0f) pre_phase -= 1.0f;
    float pre_val = NaiveWaveformValue(pre_phase);

    // Slave's phase at the next sample boundary: it reset to 0 at the sync
    // instant and has been running for (1 - frac) * phase_inc_ since.
    float post_phase = (1.0f - frac) * phase_inc_;
    float post_val   = NaiveWaveformValue(post_phase);

    // Step magnitude at the discontinuity.
    float step = post_val - pre_val;

    // Polyblep residual for a unit step at fractional offset `frac` into the
    // period [k-1, k], evaluated at sample k (the sample right after the
    // step). Standard 2-sample polyblep shape gives correction = -frac^2/2
    // per unit of step magnitude at sample k. The pre-step sample k-1 has
    // already been emitted, so its correction is lost (causality limit of
    // an online implementation).
    sync_corr_             = step * (-0.5f * frac * frac);
    skip_builtin_polyblep_ = true;

    phase_    = post_phase;
    last_out_ = 0.0f;
    eoc_      = true;
    eoc_frac_ = 0.0f;
}

float Oscillator::CalcPhaseInc(float f)
{
    return f * sr_recip_;
}

static float Polyblep(float phase_inc, float t)
{
    float dt = phase_inc;
    if(t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if(t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    else
    {
        return 0.0f;
    }
}

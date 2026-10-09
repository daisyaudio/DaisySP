#include <algorithm>
#include <limits>
#include <math.h>
#include "adenv.h"

using namespace daisysp;

//#define EXPF expf
// This causes with infinity with certain curves,
// which then causes NaN erros...
#define EXPF expf_fast

// To resolve annoying bugs when using this you can:
// if (val != val)
//     val = 0.0f; // This will un-NaN the value.

// Fast Exp approximation
// 8x multiply version
//inline float expf_fast(float x)
//{
//   x = 1.0f + x / 256.0f;
//   x *= x;
//   x *= x;
//   x *= x;
//   x *= x;
//   x *= x;
//   x *= x;
//   x *= x;
//   x *= x;
//   return x;
//}

// 10x multiply version
inline float expf_fast(float x)
{
    x = 1.0f + x / 1024.0f;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    x *= x;
    return x;
}

// Private Functions
void AdEnv::Init(float sample_rate)
{
    sample_rate_     = sample_rate;
    current_segment_ = ADENV_SEG_IDLE;
    curve_scalar_    = 0.0f; // full linear
    phase_           = 0;
    min_             = 0.0f;
    max_             = 1.0f;
    output_          = 0.0001f;
    for(uint8_t i = 0; i < ADENV_SEG_LAST; i++)
    {
        segment_time_[i] = 0.05f;
    }
}

float AdEnv::Process()
{
    uint32_t time_samps;
    float    val, out, end, beg, inc;

    // Handle Retriggering
    if(trigger_)
    {
        trigger_         = 0;
        current_segment_ = ADENV_SEG_ATTACK;
        phase_           = 0;
        curve_x_         = 0.0f;
        retrig_val_      = hard_trigger_ ? 0.0 : output_;
    }

    time_samps = (uint32_t)(segment_time_[current_segment_] * sample_rate_);

    // Fixed for now, but we could always make this a more flexible multi-segment envelope
    switch(current_segment_)
    {
        case ADENV_SEG_ATTACK:
            beg = retrig_val_;
            end = 1.0f;
            break;
        case ADENV_SEG_DECAY:
            beg = 1.0f;
            end = 0.0f;
            break;
        case ADENV_SEG_IDLE:
        default:
            beg = 0;
            end = 0;
            break;
    }

    if(prev_segment_ != current_segment_)
    {
        //Reset at segment beginning
        curve_x_ = 0;
        phase_   = 0;
    }

    //recalculate increment value
    if(curve_scalar_ == 0.0f)
    {
        c_inc_ = (end - beg) / time_samps;
    }
    else
    {
        c_inc_ = (end - beg) / (1.0f - EXPF(curve_scalar_));
    }

    if(c_inc_ >= 0.0f)
    {
        c_inc_ = std::max(c_inc_, std::numeric_limits<float>::epsilon());
    }
    else
    {
        c_inc_ = std::min(c_inc_, -std::numeric_limits<float>::epsilon());
    }

    // update output
    val = output_;
    inc = c_inc_;
    out = val;
    if(curve_scalar_ == 0.0f)
    {
        val += inc;
    }
    else
    {
        curve_x_ += (curve_scalar_ / time_samps);
        val = beg + inc * (1.0f - EXPF(curve_x_));
        if(val != val)
            val = 0.0f; // NaN check
    }

    // Update Segment
    phase_ += 1;
    prev_segment_ = current_segment_;
    if(current_segment_ != ADENV_SEG_IDLE)
    {
        if((out >= 1.f && current_segment_ == ADENV_SEG_ATTACK)
           || (out <= 0.f && current_segment_ == ADENV_SEG_DECAY))
        {
            // Advance segment
            current_segment_++;
            // TODO: Add Cycling feature here.
            if(current_segment_ > ADENV_SEG_DECAY)
            {
                current_segment_ = ADENV_SEG_IDLE;
            }
        }
    }
    if(current_segment_ == ADENV_SEG_IDLE)
    {
        val = out = 0.0f;
    }
    output_ = val;

    return out * (max_ - min_) + min_;
}

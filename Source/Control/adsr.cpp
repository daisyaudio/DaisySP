#include "adsr.h"
#include <math.h>

using namespace daisysp;


void Adsr::Init(float sample_rate, int blockSize)
{
    sample_rate_  = sample_rate / blockSize;
    attackShape_  = -1.f;
    attackTarget_ = 0.0f;
    attackTime_   = -1.f;
    decayTime_    = -1.f;
    releaseTime_  = -1.f;
    sus_level_    = 0.7f;
    x_            = 0.0f;
    gate_         = false;
    mode_         = ADSR_SEG_IDLE;

    SetTime(ADSR_SEG_ATTACK, 0.1f);
    SetTime(ADSR_SEG_DECAY, 0.1f);
    SetTime(ADSR_SEG_RELEASE, 0.1f);
}

void Adsr::Retrigger(bool hard)
{
    mode_ = ADSR_SEG_ATTACK;
    if(hard)
        x_ = 0.f;
}

void Adsr::SetTime(int seg, float time)
{
    switch(seg)
    {
        case ADSR_SEG_ATTACK: SetAttackTime(time, 0.0f); break;
        case ADSR_SEG_DECAY:
        {
            SetTimeConstant(time, decayTime_, decayD0_);
        }
        break;
        case ADSR_SEG_RELEASE:
        {
            SetTimeConstant(time, releaseTime_, releaseD0_);
        }
        break;
        default: return;
    }
}

void Adsr::SetAttackTime(float timeInS, float shape)
{
    if((timeInS != attackTime_) || (shape != attackShape_))
    {
        attackTime_  = timeInS;
        attackShape_ = shape;
        if(timeInS > 0.f)
        {
            float x         = shape;
            float target    = 9.f * powf(x, 10.f) + 0.3f * x + 1.01f;
            attackTarget_   = target;
            float logTarget = logf(1.f - (1.f / target)); // -1 for decay
            attackD0_       = -expm1f(logTarget / (timeInS * sample_rate_));
        }
        else
            attackD0_ = 1.f; // instant change
    }
}
void Adsr::SetDecayTime(float timeInS)
{
    SetTimeConstant(timeInS, decayTime_, decayD0_);
}
void Adsr::SetReleaseTime(float timeInS)
{
    SetTimeConstant(timeInS, releaseTime_, releaseD0_);
}


void Adsr::SetTimeConstant(float timeInS, float& time, float& coeff)
{
    if(timeInS != time)
    {
        time = timeInS;
        if(time > 0.f)
        {
            const float target = logf(1. / M_E);
            coeff              = -expm1f(target / (time * sample_rate_));
        }
        else
            coeff = 1.f; // instant change
    }
}


float Adsr::Process(bool gate)
{
    float out = 0.0f;

    if(gate && !gate_) // rising edge
        mode_ = ADSR_SEG_ATTACK;
    else if(!gate && gate_) // falling edge
        mode_ = ADSR_SEG_RELEASE;
    gate_ = gate;

    float D0(attackD0_);
    if(mode_ == ADSR_SEG_DECAY)
        D0 = decayD0_;
    else if(mode_ == ADSR_SEG_RELEASE)
        D0 = releaseD0_;

    float target = mode_ == ADSR_SEG_DECAY ? sus_level_ : -0.01f;
    switch(mode_)
    {
        case ADSR_SEG_IDLE: out = 0.0f; break;
        case ADSR_SEG_ATTACK:
            x_ += D0 * (attackTarget_ - x_);
            out = x_;
            if(out > 1.f)
            {
                x_ = out = 1.f;
                mode_    = ADSR_SEG_DECAY;
            }
            break;
        case ADSR_SEG_DECAY:
        case ADSR_SEG_RELEASE:
            x_ += D0 * (target - x_);
            out = x_;
            if(out < 0.0f)
            {
                x_ = out = 0.f;
                mode_    = ADSR_SEG_IDLE;
            }
        default: break;
    }
    return out;
}

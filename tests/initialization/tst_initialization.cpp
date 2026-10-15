#include "daisysp.h"
#include <cstdio>
#include <initializer_list>

using namespace daisysp;

int main()
{
    // Static storage supplies a defined initial history, so the regression does
    // not depend on the contents of uninitialized memory on the first run.
    static Oscillator osc;
    static AdEnv      env;
    int               failures = 0;

    for(float sample_rate : {44100.f, 48000.f, 96000.f})
    {
        osc.Init(sample_rate);
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
        osc.SetFreq(220.f);
        for(int i = 0; i < 64; ++i)
            osc.Process();

        osc.Init(sample_rate);
        osc.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);
        osc.SetFreq(220.f);
        // At phase zero the corrected square input is zero. With the
        // integrator initialized to zero, the first triangle sample is zero.
        const float first = osc.Process();
        if(first != 0.f)
        {
            std::printf("FAIL: triangle after Init(%.0f): %g, expected 0\n",
                        sample_rate,
                        first);
            ++failures;
        }

        env.Init(sample_rate);
        env.SetTime(ADENV_SEG_ATTACK, .01f);
        env.Trigger();
        // Reinitializing before processing must discard a pending trigger:
        // Init documents the initial segment as idle.
        env.Init(sample_rate);
        for(int i = 0; i < 16; ++i)
        {
            const float output = env.Process();
            if(output != 0.f || env.IsRunning())
            {
                std::printf(
                    "FAIL: AD envelope after Init(%.0f): %g, segment %u\n",
                    sample_rate,
                    output,
                    unsigned(env.GetCurrentSegment()));
                ++failures;
                break;
            }
        }
    }

    if(failures == 0)
        std::puts("PASS: oscillator and AD envelope initialization");
    return failures == 0 ? 0 : 1;
}

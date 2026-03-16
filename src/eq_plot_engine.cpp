#include "eq_plot_engine.h"

static float logFreq(int i, int points)
{
    float ratio = (float)i / (points - 1);

    return EQ_MIN_FREQ * powf(EQ_MAX_FREQ / EQ_MIN_FREQ, ratio);
}

static float computeResponse(ChannelState& ch, float freq)
{
    Biquad b;

    float mag = 1.0f;

    /* HPF */

    if (ch.hpfOn && ch.hpfFreq > 0.0f)
    {
        biquadHPFButterworth(b, EQ_SAMPLE_RATE, ch.hpfFreq);

        mag *= biquadMagnitude(b, freq, EQ_SAMPLE_RATE);
    }

    /* EQ Bands */

    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        biquadPeaking(
            b,
            EQ_SAMPLE_RATE,
            ch.eq[i].freq,
            ch.eq[i].q,
            ch.eq[i].gain
        );

        mag *= biquadMagnitude(b, freq, EQ_SAMPLE_RATE);
    }

    return 20.0f * log10f(mag);
}

void eqPlotRender(int channel, float* buffer, int points)
{
    ChannelState& ch = channels[channel];

    for (int i = 0; i < points; i++)
    {
        float freq = eqFreqTable[i];

        buffer[i] = computeResponse(ch, freq);
    }
}
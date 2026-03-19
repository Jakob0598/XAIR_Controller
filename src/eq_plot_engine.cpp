#include "eq_plot_engine.h"

static float scaleFreq(float v)
{
    float logMin = log10f(EQ_MIN_FREQ);
    float logMax = log10f(EQ_MAX_FREQ);

    float logF = logMin + v * (logMax - logMin);
    return powf(10.0f, logF);
}

static float scaleGain(float v)
{
    return -15.0f + v * 30.0f;
}

static float scaleQ(float v)
{
    return 10.0f * powf(0.03f, v);
}

static float scaleHPF(float v)
{
    float logMin = log10f(20.0f);
    float logMax = log10f(200.0f);

    float logF = logMin + v * (logMax - logMin);
    return powf(10.0f, logF);
}

// =============================
// RESPONSE BERECHNUNG
// =============================
static float computeResponse(ChannelState& ch, float freq)
{
    float mag = 1.0f;
    Biquad b;

    // HPF
    if (ch.hpfOn)
    {
        float f = scaleHPF(ch.hpfFreq);
        biquadLCutButterworth(b, EQ_SAMPLE_RATE, f);
        mag *= biquadMagnitude(b, freq, EQ_SAMPLE_RATE);
    }

    // EQ
    if (ch.eqOn)
    {
        for (int i = 1; i <= 4; i++)
        {
            EqBand& band = ch.eq[i];

            float f = scaleFreq(band.freq);
            float g = scaleGain(band.gain);
            float q = scaleQ(band.q);

            switch (band.type)
            {
                case EQ_LCUT:
                    biquadHPFButterworth(b, EQ_SAMPLE_RATE, f, q);
                    break;

                case EQ_HCUT:
                    biquadLPFButterworth(b, EQ_SAMPLE_RATE, f, q);
                    break;

                case EQ_LSHV:
                    biquadLowShelf(b, EQ_SAMPLE_RATE, f, g, q);
                    break;

                case EQ_HSHV:
                    biquadHighShelf(b, EQ_SAMPLE_RATE, f, g, q);
                    break;

                case EQ_PEQ:
                    biquadPeaking(b, EQ_SAMPLE_RATE, f, q, g);
                    break;

                case EQ_VEQ:
                    biquadVEQ(b, EQ_SAMPLE_RATE, f, q, g);
                    break;

                default:
                    biquadPeaking(b, EQ_SAMPLE_RATE, f, q, g);
                    break;
            }
            mag *= biquadMagnitude(b, freq, EQ_SAMPLE_RATE);
        }
    }

    return 20.0f * log10f(mag);
}

// =============================
// RENDER
// =============================
void eqPlotRender(int ch, float* buffer, int points)
{
    if (ch < 1 || ch >= MAX_CHANNELS) return;

    ChannelState& c = channels[ch];

    for (int i = 0; i < points; i++)
    {
        float freq = eqFreqTable[i];
        buffer[i] = computeResponse(c, freq);

        if (i % 20 == 0) yield();
    }
}

void eqPlotRenderSingleBand(int ch, int bandIndex, float* buffer, int points)
{
    ChannelState& c = channels[ch];

    for (int i = 0; i < points; i++)
    {
        float freq = eqFreqTable[i];

        float mag = 1.0f;

        EqBand& band = c.eq[bandIndex];

        float f = scaleFreq(band.freq);
        float g = scaleGain(band.gain);
        float q = scaleQ(band.q);

        Biquad bq;

        switch (band.type)
        {
            case EQ_LCUT:
                biquadHPFButterworth(bq, EQ_SAMPLE_RATE, f, q);
                break;

            case EQ_HCUT:
                biquadLPFButterworth(bq, EQ_SAMPLE_RATE, f, q);
                break;

            case EQ_LSHV:
                biquadLowShelf(bq, EQ_SAMPLE_RATE, f, g, q);
                break;

            case EQ_HSHV:
                biquadHighShelf(bq, EQ_SAMPLE_RATE, f, g, q);
                break;

            case EQ_PEQ:
                biquadPeaking(bq, EQ_SAMPLE_RATE, f, q, g);
                break;

            case EQ_VEQ:
                biquadVEQ(bq, EQ_SAMPLE_RATE, f, q, g);
                break;

            default:
                biquadPeaking(bq, EQ_SAMPLE_RATE, f, q, g);
                break;
        }

        mag *= biquadMagnitude(bq, freq, EQ_SAMPLE_RATE);

        buffer[i] = 20.0f * log10f(mag);
    }
}

void eqPlotRenderHPF(int ch, float* buffer, int points)
{
    if (ch < 1 || ch >= MAX_CHANNELS) return;

    ChannelState& c = channels[ch];

    for (int i = 0; i < points; i++)
    {
        float freq = eqFreqTable[i];

        float mag = 1.0f;

        if (c.hpfOn)
        {
            float f = scaleHPF(c.hpfFreq);

            Biquad b;
            biquadLCutButterworth(b, EQ_SAMPLE_RATE, f);

            mag *= biquadMagnitude(b, freq, EQ_SAMPLE_RATE);
        }

        buffer[i] = 20.0f * log10f(mag);

        if (i % 20 == 0) yield();
    }
}
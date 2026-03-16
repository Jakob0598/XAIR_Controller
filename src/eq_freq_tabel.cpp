#include "eq_freq_table.h"


float eqFreqTable[EQ_POINTS];

void eqFreqTableInit()
{
    const float logMin = log10f(EQ_MIN_FREQ);
    const float logMax = log10f(EQ_MAX_FREQ);

    for (int i = 0; i < EQ_POINTS; i++)
    {
        float r = (float)i / (EQ_POINTS - 1);

        float logF = logMin + r * (logMax - logMin);

        eqFreqTable[i] = powf(10.0f, logF);
    }
}
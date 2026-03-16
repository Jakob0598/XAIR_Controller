#pragma once
#include "config.h"

struct Biquad
{
    float b0,b1,b2,a1,a2;
};

void biquadLowpass(Biquad& b,float fs,float f,float q);
void biquadHighpass(Biquad& b,float fs,float f,float q);
void biquadHPFButterworth(Biquad& b, float fs, float f);

void biquadPeaking(Biquad& b,float fs,float f,float q,float gain);

float biquadMagnitude(const Biquad& b,float freq,float fs);
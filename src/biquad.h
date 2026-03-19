#pragma once
#include "config.h"

struct Biquad
{
    float b0,b1,b2,a1,a2;
};

// bestehend
void biquadLowpass(Biquad& b,float fs,float f,float q);
void biquadHighpass(Biquad& b,float fs,float f,float q);

void biquadHPFButterworth(Biquad& b, float fs, float f, float q);
void biquadLPFButterworth(Biquad& b, float fs, float f, float q);

void biquadLowShelf(Biquad& b, float fs, float f, float gain, float q);
void biquadHighShelf(Biquad& b, float fs, float f, float gain, float q);

void biquadVEQ(Biquad& b, float fs, float f, float q, float gain);
void biquadPeaking(Biquad& b,float fs,float f,float q,float gain);


float biquadMagnitude(const Biquad& b,float freq,float fs);

void biquadLCutButterworth(Biquad& b, float fs, float f);
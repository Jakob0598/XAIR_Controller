#include "biquad.h"

static const float BUTTERWORTH_Q = 0.70710678f;

// =============================
// PEAKING 
// =============================
void biquadPeaking(Biquad& b,float fs,float f,float q,float gain)
{
    float A = powf(10.0f,gain/40.0f);

    float w = 2.0f*PI*f/fs;
    float alpha = sinf(w)/(2.0f*q);

    float cosw = cosf(w);

    float b0 = 1 + alpha*A;
    float b1 = -2*cosw;
    float b2 = 1 - alpha*A;

    float a0 = 1 + alpha/A;
    float a1 = -2*cosw;
    float a2 = 1 - alpha/A;

    b.b0=b0/a0;
    b.b1=b1/a0;
    b.b2=b2/a0;

    b.a1=a1/a0;
    b.a2=a2/a0;
}

// =============================
// HIGHPASS 
// =============================
void biquadHighpass(Biquad& b,float fs,float f,float q)
{
    float w = 2.0f*PI*f/fs;

    float alpha = sinf(w)/(2*q);
    float cosw = cosf(w);

    float b0 = (1+cosw)/2;
    float b1 = -(1+cosw);
    float b2 = (1+cosw)/2;

    float a0 = 1+alpha;
    float a1 = -2*cosw;
    float a2 = 1-alpha;

    b.b0=b0/a0;
    b.b1=b1/a0;
    b.b2=b2/a0;

    b.a1=a1/a0;
    b.a2=a2/a0;
}

// =============================
// LCut BUTTERWORTH 
// =============================
void biquadLCutButterworth(Biquad& b, float fs, float f)
{
    float Q = BUTTERWORTH_Q;

    float w = 2.0f * PI * f / fs;

    float cosw = cosf(w);
    float sinw = sinf(w);

    float alpha = sinw / (2.0f * Q);

    float b0 =  (1 + cosw) / 2.0f;
    float b1 = -(1 + cosw);
    float b2 =  (1 + cosw) / 2.0f;

    float a0 = 1 + alpha;
    float a1 = -2 * cosw;
    float a2 = 1 - alpha;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;

    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}

// =============================
// HPF BUTTERWORTH 
// =============================

void biquadHPFButterworth(Biquad& b, float fs, float f, float q)
{
    float w = 2.0f * PI * f / fs;
    float cosw = cosf(w);
    float sinw = sinf(w);

    float alpha = sinw / (2.0f * q);

    float b0 =  (1 + cosw) / 2.0f;
    float b1 = -(1 + cosw);
    float b2 =  (1 + cosw) / 2.0f;

    float a0 = 1 + alpha;
    float a1 = -2 * cosw;
    float a2 = 1 - alpha;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;
    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}


// =============================
// LPF BUTTERWORTH 
// =============================
void biquadLPFButterworth(Biquad& b, float fs, float f, float q)
{
    float w = 2.0f * PI * f / fs;
    float cosw = cosf(w);
    float sinw = sinf(w);

    float alpha = sinw / (2.0f * q);

    float b0 = (1 - cosw) / 2.0f;
    float b1 = 1 - cosw;
    float b2 = (1 - cosw) / 2.0f;

    float a0 = 1 + alpha;
    float a1 = -2 * cosw;
    float a2 = 1 - alpha;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;
    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}

// =============================
// LOW SHELF 
// =============================
void biquadLowShelf(Biquad& b, float fs, float f, float gain, float q)
{
    float A = powf(10.0f, gain / 40.0f);

    float w = 2.0f * PI * f / fs;
    float cosw = cosf(w);
    float sinw = sinf(w);

    float alpha = sinw / (2.0f * q);
    float beta = 2.0f * sqrtf(A) * alpha;

    float b0 =    A*((A+1) - (A-1)*cosw + beta);
    float b1 =  2*A*((A-1) - (A+1)*cosw);
    float b2 =    A*((A+1) - (A-1)*cosw - beta);
    float a0 =        (A+1) + (A-1)*cosw + beta;
    float a1 =   -2*((A-1) + (A+1)*cosw);
    float a2 =        (A+1) + (A-1)*cosw - beta;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;
    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}

// =============================
// HIGH SHELF 
// =============================
void biquadHighShelf(Biquad& b, float fs, float f, float gain, float q)
{
    float A = powf(10.0f, gain / 40.0f);

    float w = 2.0f * PI * f / fs;
    float cosw = cosf(w);
    float sinw = sinf(w);

    float alpha = sinw / (2.0f * q);
    float beta = 2.0f * sqrtf(A) * alpha;

    float b0 =    A*((A+1) + (A-1)*cosw + beta);
    float b1 = -2*A*((A-1) + (A+1)*cosw);
    float b2 =    A*((A+1) + (A-1)*cosw - beta);
    float a0 =        (A+1) - (A-1)*cosw + beta;
    float a1 =    2*((A-1) - (A+1)*cosw);
    float a2 =        (A+1) - (A-1)*cosw - beta;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;
    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}

// =============================
// MAGNITUDE 
// =============================
float biquadMagnitude(const Biquad& b,float freq,float fs)
{
    float w = 2*M_PI*freq/fs;

    float cosw = cosf(w);
    float sinw = sinf(w);

    float num_real =
        b.b0 + b.b1*cosw + b.b2*cos(2*w);

    float num_imag =
        -b.b1*sinw - b.b2*sin(2*w);

    float den_real =
        1 + b.a1*cosw + b.a2*cos(2*w);

    float den_imag =
        -b.a1*sinw - b.a2*sin(2*w);

    float num = num_real*num_real + num_imag*num_imag;
    float den = den_real*den_real + den_imag*den_imag;

    return sqrtf(num/den);
}

// =============================
// VEQ
// =============================
void biquadVEQ(Biquad& b, float fs, float f, float q, float gain)
{
    // VEQ = breiteres PEQ
    float qAdjusted = q * 0.6f; // breiter!

    float A = powf(10.0f, gain / 40.0f);

    float w = 2.0f * PI * f / fs;
    float alpha = sinf(w) / (2.0f * qAdjusted);

    float cosw = cosf(w);

    float b0 = 1 + alpha * A;
    float b1 = -2 * cosw;
    float b2 = 1 - alpha * A;

    float a0 = 1 + alpha / A;
    float a1 = -2 * cosw;
    float a2 = 1 - alpha / A;

    b.b0 = b0 / a0;
    b.b1 = b1 / a0;
    b.b2 = b2 / a0;
    b.a1 = a1 / a0;
    b.a2 = a2 / a0;
}
#pragma once
#include "config.h"

struct EQBand {

    float freq;
    float gain;
    float q;

};

struct Channel {

    float fader;
    float gain;

    EQBand eq[4];

    float aux[MAX_AUX];

};

struct MixerState {

    Channel ch[MAX_CHANNELS];

};

extern MixerState mixer;
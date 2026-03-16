#pragma once

#include "config.h"

#define MAX_CHANNELS 16
#define MAX_BUSES 6
#define MAX_EQ_BANDS 4
#define MAX_METERS 64


/* ============================= */
/*           EQ BAND             */
/* ============================= */

struct EqBand
{
    float freq;
    float gain;
    float q;
};


/* ============================= */
/*         COMPRESSOR            */
/* ============================= */

struct Compressor
{
    float threshold;
    float ratio;
    float attack;
    float release;
};


/* ============================= */
/*            GATE               */
/* ============================= */

struct Gate
{
    float threshold;
};


/* ============================= */
/*          CHANNEL              */
/* ============================= */

struct ChannelState
{
    float fader;
    bool mute;
    float pan;

    float gain;

    float hpfFreq;
    bool hpfOn;

    EqBand eq[MAX_EQ_BANDS];
    Compressor comp;
    Gate gate;

    float sends[MAX_BUSES];

    char name[32];
};


/* ============================= */
/*             BUS               */
/* ============================= */

struct BusState
{
    float fader;
};


/* ============================= */
/*          GLOBAL STATE         */
/* ============================= */

extern ChannelState channels[MAX_CHANNELS];
extern BusState buses[MAX_BUSES];

extern float mainFader;

extern float meters[MAX_METERS];


/* ============================= */
/*        SET FUNCTIONS          */
/* ============================= */

void mixerSetFader(int ch, float value);
void mixerSetMute(int ch, bool value);
void mixerSetPan(int ch, float value);
void mixerSetGain(int ch, float value);

void mixerSetEqGain(int ch, int band, float value);
void mixerSetEqFreq(int ch, int band, float value);
void mixerSetEqQ(int ch, int band, float value);

void mixerSetHPFFreq(int ch, float value);
void mixerSetHPFOn(int ch, bool value);

void mixerSetCompThreshold(int ch, float value);
void mixerSetCompRatio(int ch, float value);
void mixerSetCompAttack(int ch, float value);
void mixerSetCompRelease(int ch, float value);

void mixerSetGateThreshold(int ch, float value);

void mixerSetSend(int ch, int bus, float value);

void mixerSetBusFader(int bus, float value);
void mixerSetMainFader(float value);

void mixerSetMeter(int index, float value);

void mixerSetName(int ch, const char* name);
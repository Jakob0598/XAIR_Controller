#pragma once

#include "config.h"

#define MAX_CHANNELS 17   // 1-based
#define MAX_BUSES 7
#define MAX_EQ_BANDS 5
#define MAX_METERS 64

extern int mixerMaxChannels;
extern int mixerMaxBuses;
void mixerSetLimits(int channels, int buses);

enum EqType
{
    EQ_LCUT,
    EQ_LSHV,
    EQ_PEQ,
    EQ_VEQ,
    EQ_HSHV,
    EQ_HCUT
};

struct EqBand
{
    float freq;
    float gain;
    float q;

    EqType type;   
};

struct Compressor
{
    float threshold;
    float ratio;
    float attack;
    float release;
};

struct Gate
{
    float threshold;
};

struct ChannelState
{
    float fader;
    bool mute;
    float pan;

    float gain;

    float hpfFreq;
    bool hpfOn;
    bool eqOn;

    EqBand eq[MAX_EQ_BANDS];
    Compressor comp;
    Gate gate;

    float sends[MAX_BUSES];

    char name[32];
};

struct BusState
{
    float fader;
};

extern ChannelState channels[MAX_CHANNELS];
extern BusState buses[MAX_BUSES];

extern float mainFader;
extern float meters[MAX_METERS];

// SETTERS (jetzt 1-based!)
void mixerSetFader(int ch, float value);
void mixerSetMute(int ch, bool value);
void mixerSetPan(int ch, float value);
void mixerSetGain(int ch, float value);

void mixerSetEqOn(int ch, bool value);
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

void mixerSetMixerModel(const char* model);
void mixerSetMixerName(const char* name);
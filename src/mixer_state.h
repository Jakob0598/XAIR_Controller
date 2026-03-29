#pragma once

#include "config.h"

#define MAX_CHANNELS 17   // 1-based
#define MAX_BUSES 7
#define MAX_EQ_BANDS 5
#define MAX_METERS 64

extern int mixerMaxChannels;
extern int mixerMaxBuses;
void mixerSetLimits(int channels, int buses);

extern char mixerName[32];
extern char mixerModel[32];

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

// FX-Insert-Modus (XAir: 0=off, 1..4=FX slot)
enum InsMode
{
    INS_OFF  = 0,
    INS_FX1  = 1,
    INS_FX2  = 2,
    INS_FX3  = 3,
    INS_FX4  = 4
};

struct ChannelState
{
    float fader;
    bool  mute;
    float pan;

    // Preamp
    float gain;        // 0.0-1.0 → maps to +10..+60 dB (XAir)
    bool  phantom;     // 48V
    bool  polarity;    // phase invert
    float hpfFreq;
    bool  hpfOn;

    // EQ
    bool   eqOn;
    EqBand eq[MAX_EQ_BANDS];

    // Dynamics
    Compressor comp;
    Gate gate;

    // Insert / FX
    InsMode insMode;
    bool    insOn;

    // Stereo Link (wird paarweise gesetzt vom XAir: ch01+02, ch03+04, …)
    bool stereoLinked;

    // Sends
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

// SETTERS (1-based channel index)
void mixerSetFader(int ch, float value);
void mixerSetMute(int ch, bool value);
void mixerSetPan(int ch, float value);
void mixerSetGain(int ch, float value);
void mixerSetPhantom(int ch, bool value);
void mixerSetPolarity(int ch, bool value);
void mixerSetStereoLink(int ch, bool value);   // setzt ch UND ch±1
void mixerSetInsMode(int ch, InsMode mode);
void mixerSetInsOn(int ch, bool value);

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

// Hilfsfunktion: liefert den "Partner" eines Stereolink-Kanals (ch^1 im 1-based Schema)
// z.B. ch=1 → 2, ch=2 → 1, ch=3 → 4, ch=4 → 3
inline int mixerStereoPartner(int ch)
{
    if (ch < 1) return -1;
    // Kanäle sind paarweise: 1&2, 3&4, …
    if ((ch % 2) == 1) return ch + 1;   // ungerade → nächster
    return ch - 1;                        // gerade → vorheriger
}

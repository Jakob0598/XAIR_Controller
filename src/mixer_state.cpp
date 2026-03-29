#include "mixer_state.h"

ChannelState channels[MAX_CHANNELS];
BusState     buses[MAX_BUSES];

float mainFader = 0.0f;
float meters[MAX_METERS];

int mixerMaxChannels = 16;
int mixerMaxBuses    = 6;

void mixerSetLimits(int channelLimits, int busesLimits)
{
    mixerMaxChannels = channelLimits;
    mixerMaxBuses    = busesLimits;

    DBG2("ChannelLimits:", channelLimits);
    DBG2("BusLimits:",     busesLimits);
}

char mixerName[32]  = {0};
char mixerModel[32] = {0};

static bool validCh(int ch)   { return ch >= 1 && ch < MAX_CHANNELS; }
static bool validBus(int b)   { return b  >= 1 && b  < MAX_BUSES;    }
static bool validBand(int b)  { return b  >= 1 && b  < MAX_EQ_BANDS; }

// =============================
// MIXER META
// =============================

void mixerSetMixerName(const char* name)
{
    strncpy(mixerName,  name,  sizeof(mixerName)  - 1);
    mixerName[sizeof(mixerName) - 1] = '\0';
}

void mixerSetMixerModel(const char* model)
{
    strncpy(mixerModel, model, sizeof(mixerModel) - 1);
    mixerModel[sizeof(mixerModel) - 1] = '\0';
}

// =============================
// CHANNEL BASICS
// =============================

void mixerSetFader(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].fader = value;
}

void mixerSetMute(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].mute = value;
}

void mixerSetPan(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].pan = value;
}

void mixerSetGain(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].gain = value;
}

void mixerSetPhantom(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].phantom = value;
}

void mixerSetPolarity(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].polarity = value;
}

void mixerSetStereoLink(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].stereoLinked = value;
    // Partner ebenfalls markieren
    int partner = mixerStereoPartner(ch);
    if (partner >= 1 && partner < MAX_CHANNELS)
        channels[partner].stereoLinked = value;
}

void mixerSetInsMode(int ch, InsMode mode)
{
    if (!validCh(ch)) return;
    channels[ch].insMode = mode;
}

void mixerSetInsOn(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].insOn = value;
}

void mixerSetName(int ch, const char* name)
{
    if (!validCh(ch)) return;
    strncpy(channels[ch].name, name, sizeof(channels[ch].name) - 1);
    channels[ch].name[sizeof(channels[ch].name) - 1] = '\0';
}

// =============================
// EQ
// =============================

void mixerSetEqOn(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].eqOn = value;
}

void mixerSetEqGain(int ch, int band, float value)
{
    if (!validCh(ch) || !validBand(band)) return;
    channels[ch].eq[band].gain = value;
}

void mixerSetEqFreq(int ch, int band, float value)
{
    if (!validCh(ch) || !validBand(band)) return;
    channels[ch].eq[band].freq = value;
}

void mixerSetEqQ(int ch, int band, float value)
{
    if (!validCh(ch) || !validBand(band)) return;
    channels[ch].eq[band].q = value;
}

void mixerSetHPFFreq(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].hpfFreq = value;
}

void mixerSetHPFOn(int ch, bool value)
{
    if (!validCh(ch)) return;
    channels[ch].hpfOn = value;
}

// =============================
// COMPRESSOR
// =============================

void mixerSetCompThreshold(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].comp.threshold = value;
}

void mixerSetCompRatio(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].comp.ratio = value;
}

void mixerSetCompAttack(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].comp.attack = value;
}

void mixerSetCompRelease(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].comp.release = value;
}

// =============================
// GATE
// =============================

void mixerSetGateThreshold(int ch, float value)
{
    if (!validCh(ch)) return;
    channels[ch].gate.threshold = value;
}

// =============================
// SENDS
// =============================

void mixerSetSend(int ch, int bus, float value)
{
    if (!validCh(ch) || !validBus(bus)) return;
    channels[ch].sends[bus] = value;
}

// =============================
// BUS
// =============================

void mixerSetBusFader(int bus, float value)
{
    if (!validBus(bus)) return;
    buses[bus].fader = value;
}

// =============================
// MAIN FADER
// =============================

void mixerSetMainFader(float value)
{
    mainFader = value;
}

// =============================
// METERS
// =============================

void mixerSetMeter(int index, float value)
{
    if (index < 0 || index >= MAX_METERS) return;
    meters[index] = value;
}
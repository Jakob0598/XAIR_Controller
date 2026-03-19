#include "mixer_state.h"

ChannelState channels[MAX_CHANNELS];
BusState buses[MAX_BUSES];

float mainFader = 0.0f;
float meters[MAX_METERS];

int mixerMaxChannels = 16;
int mixerMaxBuses = 6;

void mixerSetLimits(int channelLimits, int busesLimits)
{
    mixerMaxChannels = channelLimits;
    mixerMaxBuses = busesLimits;

    DBG2("ChannelLimits:", channelLimits);
    DBG2("BusLimits:", busesLimits);
}

static char mixerName[32];
static char mixerModel[32];

static bool validCh(int ch) { return ch >= 1 && ch < MAX_CHANNELS; }
static bool validBus(int b) { return b >= 1 && b < MAX_BUSES; }
static bool validBand(int b) { return b >= 1 && b < MAX_EQ_BANDS; }

void mixerSetMixerName(const char* name)
{
    strncpy(mixerName, name, sizeof(mixerName) - 1);
}

void mixerSetMixerModel(const char* model)
{
    strncpy(mixerModel, model, sizeof(mixerModel) - 1);
}

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

// ---------- EQ ----------
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

// ---------- SEND ----------
void mixerSetSend(int ch, int bus, float value)
{
    if (!validCh(ch) || !validBus(bus)) return;
    channels[ch].sends[bus] = value;
}

void mixerSetName(int ch, const char* name)
{
    if (!validCh(ch)) return;
    strncpy(channels[ch].name, name, sizeof(channels[ch].name) - 1);
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
// BUS
// =============================

void mixerSetBusFader(int bus, float value)
{
    if (!validBus(bus)) return;
    buses[bus].fader = value;
}


// =============================
// MAIN
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
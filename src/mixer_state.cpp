#include "mixer_state.h"


/* ============================= */
/*         GLOBAL STATE          */
/* ============================= */

ChannelState channels[MAX_CHANNELS];
BusState buses[MAX_BUSES];

float mainFader = 0.0f;

float meters[MAX_METERS];


/* ============================= */
/*        CHANNEL SETTERS        */
/* ============================= */

void mixerSetFader(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].fader = value;
}

void mixerSetMute(int ch, bool value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].mute = value;
}

void mixerSetPan(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].pan = value;
}

void mixerSetGain(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].gain = value;
}


/* ============================= */
/*             EQ                */
/* ============================= */

void mixerSetEqGain(int ch, int band, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    if (band < 0 || band >= MAX_EQ_BANDS) return;

    channels[ch].eq[band].gain = value;
}

void mixerSetEqFreq(int ch, int band, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    if (band < 0 || band >= MAX_EQ_BANDS) return;

    channels[ch].eq[band].freq = value;
}

void mixerSetEqQ(int ch, int band, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    if (band < 0 || band >= MAX_EQ_BANDS) return;

    channels[ch].eq[band].q = value;
}


void mixerSetHPFFreq(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].hpfFreq = value;
}

void mixerSetHPFOn(int ch, bool value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].hpfOn = value;
}

/* ============================= */
/*         COMPRESSOR            */
/* ============================= */

void mixerSetCompThreshold(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].comp.threshold = value;
}

void mixerSetCompRatio(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].comp.ratio = value;
}

void mixerSetCompAttack(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].comp.attack = value;
}

void mixerSetCompRelease(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].comp.release = value;
}


/* ============================= */
/*             GATE              */
/* ============================= */

void mixerSetGateThreshold(int ch, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    channels[ch].gate.threshold = value;
}


/* ============================= */
/*            SENDS              */
/* ============================= */

void mixerSetSend(int ch, int bus, float value)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;
    if (bus < 0 || bus >= MAX_BUSES) return;

    channels[ch].sends[bus] = value;
}


/* ============================= */
/*             BUS               */
/* ============================= */

void mixerSetBusFader(int bus, float value)
{
    if (bus < 0 || bus >= MAX_BUSES) return;

    buses[bus].fader = value;
}


/* ============================= */
/*             MAIN              */
/* ============================= */

void mixerSetMainFader(float value)
{
    mainFader = value;
}


/* ============================= */
/*            METERS             */
/* ============================= */

void mixerSetMeter(int index, float value)
{
    if (index < 0 || index >= MAX_METERS) return;

    meters[index] = value;
}


/* ============================= */
/*           CHANNEL NAME        */
/* ============================= */

void mixerSetName(int ch, const char* name)
{
    if (ch < 0 || ch >= MAX_CHANNELS) return;

    strncpy(channels[ch].name, name, sizeof(channels[ch].name) - 1);
}
#include "mixer_defaults.h"

void mixerInitDefaults()
{
    mixerSetLimits(16, 6); // default XR18
    for (int ch = 1; ch < MAX_CHANNELS; ch++)
    {
        channels[ch].fader = 0.0f;
        channels[ch].mute = true;
        channels[ch].pan = 0.5f;
        channels[ch].gain = 0.0f;

        // Preamp
        channels[ch].phantom = false;
        channels[ch].polarity = false;

        channels[ch].hpfFreq = 0.0f;
        channels[ch].hpfOn = false;
        channels[ch].eqOn = false;

        // Config
        channels[ch].stereoLinked = false;
        channels[ch].insMode = INS_OFF;
        channels[ch].insOn = false;

        strcpy(channels[ch].name, "");

        // EQ
        for (int b = 1; b < MAX_EQ_BANDS; b++)
        {
            channels[ch].eq[b].freq = 0.2f*b;
            channels[ch].eq[b].gain = 0.5f;
            channels[ch].eq[b].q    = 0.5f;
            channels[ch].eq[b].type = EQ_PEQ;
        }

        // Sends
        for (int bus = 1; bus < MAX_BUSES; bus++)
        {
            channels[ch].sends[bus] = 0.0f;
        }
    }

    // Bus
    for (int b = 1; b < MAX_BUSES; b++)
    {
        buses[b].fader = 0.0f;
    }

    mainFader = 0.0f;

    for (int i = 0; i < MAX_METERS; i++)
    {
        meters[i] = 0.0f;
    }

}
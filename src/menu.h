#pragma once
#include "config.h"

// VU-Meter Spaltenbreite (wird auch von display_manager genutzt)
#define VU_WIDTH   22

// Globale Redraw-Flags (können von OSC-Parser etc. gesetzt werden)
extern bool needRedraw;
extern bool needMiniRedraw;

// EQ Band Disable State (wird von display_manager gelesen)
extern float savedBandGain[17][5];
extern bool  bandDisabled[17][5];

void menuBegin();
void menuLoop();

// Fader auf aktuelle Mixer-Werte fahren (wird auch vom network_manager aufgerufen)
void syncFadersToMixer();

// Kanal-zu-Fader-Mapping (Stereolink-bewusst)
int mapFaderToChannel(int faderIndex);

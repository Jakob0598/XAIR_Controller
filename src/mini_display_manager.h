#pragma once
#include <config.h>

void miniDisplayBegin();
void miniDisplayLoop();
void miniDisplayStartupAnimation();

// Text ohne VU (für EQ-Parameter-Anzeige etc.)
void miniDisplayText(uint8_t display, String text);

// Text + vertikaler VU-Balken am rechten Rand (vuLevel: 0.0–1.0)
void miniDisplayTextWithVU(uint8_t displayIndex, String line1, String line2, float vuLevel);

// Display komplett ausschalten (leer, schwarz)
void miniDisplayClear(uint8_t displayIndex);

// Parameter-Anzeige ohne VU – schickes Layout mit Label, Wert und optionaler Einheit
void miniDisplayParam(uint8_t displayIndex, const char* label, const char* value, const char* unit = nullptr);
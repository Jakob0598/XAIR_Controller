#pragma once
#include <config.h>

void ledBegin();

void ledSet(uint8_t index, bool state);
void ledToggle(uint8_t index);

void ledAllOff();
void ledAllOn();
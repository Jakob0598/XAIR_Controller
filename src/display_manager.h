#pragma once
#include "config.h"

void displayBegin();
void displayLoop();

void displayClear();

void displayPrint(int x, int y, String text);
void displayDrawEq(int channel);

void displayBacklight(bool state);
#pragma once
#include "config.h"

extern TFT_eSPI tft;

void displayBegin();
void displayLoop();
void displayStartupAnimation();
void displayClear();

void displayPrint(int x, int y, String text);
void displayDrawEq(int channel);

void displayBacklight(bool state);

void drawDashedLine(int x0, int y0, int x1, int y1, uint16_t color, int dashLength = 4, int gapLength = 4);
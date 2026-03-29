#pragma once
#include "config.h"

extern TFT_eSPI tft;

void displayBegin();
void displayLoop();
void displayStartupAnimation();
void displayBootComplete();
void displayClear();

void displayPrint(int x, int y, String text);
void displayDrawEq(int channel);

void displayBacklight(bool state);

void drawDashedLine(int x0, int y0, int x1, int y1, uint16_t color, int dashLength = 4, int gapLength = 4);

void displayDrawStereoVUMeter(int meterLeft, int meterRight);
void displayResetVUBackground();


void displayDrawStatusBar();
void displayDrawWiFi(int x, int y);
void displayDrawXAir(int x, int y);
void displayDrawBattery(int x, int y);
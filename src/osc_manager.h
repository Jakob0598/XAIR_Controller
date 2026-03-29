#pragma once
#include "config.h"

void oscBegin();
void oscLoop();
void oscReconnect();

bool isSystemReady();

void oscBroadcast(const char* address);
void oscSend(const char* address);
void oscSendFloat(const char* address, float value);
void oscSendInt(const char* address, int value);
void oscSendString(const char* address, const char* value);
void oscSendMeters(const char* meterId, int packs);

extern bool mixerDetected;

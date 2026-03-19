#pragma once

#include "config.h"

extern bool mixerDetected;

void xairHandleChannel(OSCMessage& msg, const char* address);
void xairHandleBus(OSCMessage& msg, const char* address);
void xairHandleMain(OSCMessage& msg, const char* address);
void xairHandleMeters(OSCMessage& msg, const char* address);
void xairHandleGlobal(OSCMessage& msg, const char* address);
#pragma once

#include "config.h"

void xairHandleChannel(OSCMessage& msg, const char* address);
void xairHandleBus(OSCMessage& msg, const char* address);
void xairHandleMain(OSCMessage& msg, const char* address);
void xairHandleMeters(OSCMessage& msg, const char* address);
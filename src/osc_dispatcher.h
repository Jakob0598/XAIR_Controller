#pragma once
#include "config.h"

typedef void (*OscHandler)(OSCMessage& msg, const char* address);

void oscRegisterRoutes();
void oscDispatch(OSCMessage& msg, char address[OSC_BUFFER_SIZE]);
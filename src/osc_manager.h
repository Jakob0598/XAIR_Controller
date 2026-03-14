#pragma once

void oscBegin();
void oscLoop();

void sendOSCFloat(const char* addr,float value);
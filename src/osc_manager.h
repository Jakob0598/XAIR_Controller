#pragma once

void oscBegin();
void oscLoop();
void oscReconnect();

void oscBroadcast(const char* address);
void oscSend(const char* address);
void oscSendFloat(const char* address, float value);
void oscSendInt(const char* address, int value);

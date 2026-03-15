#pragma once
#include "config.h"

struct Settings {

    uint16_t magic;

    char ssid[32];
    char password[64];

    uint8_t ip[4];

    uint16_t sendPort;
    uint16_t receivePort;

};

extern Settings settings;

void loadSettings();
void saveSettings();
void setDefaults();
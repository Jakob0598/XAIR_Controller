#pragma once
#include "config.h"

// WiFi State Machine
enum WifiState
{
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_AP_FALLBACK
};


// öffentliche API
void wifiBegin();
void wifiLoop();

bool wifiConnected();
WifiState wifiGetState();
IPAddress wifiIP();
String wifiSSID();



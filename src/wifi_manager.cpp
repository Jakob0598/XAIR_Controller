#include "wifi_manager.h"
#include "settings.h"
#include "config.h"

#include <WiFi.h>
#include <ESPmDNS.h>

unsigned long lastReconnectAttempt = 0;
unsigned long wifiFailStart = 0;

bool apModeActive = false;

const unsigned long reconnectInterval = WIFI_RECONNECT_INTERVAL;
const unsigned long apFallbackDelay = WIFI_AP_TIMEOUT;


void startAP()
{
    DBG("Starting fallback AP");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(HOSTNAME);

    DBG2("AP IP: ", WiFi.softAPIP());

    apModeActive = true;
}


void startMDNS()
{
    if (MDNS.begin(HOSTNAME))
    {
        DBG("mDNS responder started");
        DBG3("Open http://", HOSTNAME, ".local");
    }
    else
    {
        DBG("mDNS start failed");
    }
}


void wifiBegin()
{
    DBG("Starting WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);

    DBG2("Connecting to SSID: ", settings.ssid);

    WiFi.begin(settings.ssid, settings.password);
}


bool wifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}


void wifiLoop()
{
    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
        if (now - lastReconnectAttempt > reconnectInterval)
        {
            DBG("WiFi reconnect attempt");

            WiFi.begin(settings.ssid, settings.password);

            lastReconnectAttempt = now;
        }

        if (wifiFailStart == 0)
        {
            wifiFailStart = now;
        }

        if (!apModeActive && (now - wifiFailStart > apFallbackDelay))
        {
            DBG("WiFi offline -> enabling AP fallback");

            startAP();
        }
    }
    else
    {
        static bool printed = false;

        if (!printed)
        {
            DBG("WiFi connected");

            DBG2("IP: ", WiFi.localIP());

            startMDNS();

            printed = true;
        }

        wifiFailStart = 0;
    }
}
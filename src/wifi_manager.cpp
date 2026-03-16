#include "wifi_manager.h"


WifiState wifiState = WIFI_DISCONNECTED;

unsigned long lastReconnectAttempt = 0;
unsigned long wifiFailStart = 0;

bool mdnsStarted = false;

const unsigned long reconnectInterval = WIFI_RECONNECT_INTERVAL;
const unsigned long apFallbackDelay = WIFI_AP_TIMEOUT;


void startAP()
{
    DBG("Starting fallback AP");

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(HOSTNAME);

    DBG2("AP IP: ", WiFi.softAPIP());

    wifiState = WIFI_AP_FALLBACK;
}


void stopAP()
{
    DBG("Stopping fallback AP");

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}


void startMDNS()
{
    if (mdnsStarted) return;

    if (MDNS.begin(HOSTNAME))
    {
        DBG("mDNS responder started");
        DBG3("Open http://", HOSTNAME, ".local");
        mdnsStarted = true;
        MDNS.addService("http", "tcp", 80);
    }
    else
    {
        DBG("mDNS start failed");
    }
}


void connectWiFi()
{
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED || status == WL_IDLE_STATUS)
        return;

    DBG2("Connecting to SSID: ", settings.ssid);

    WiFi.begin(settings.ssid, settings.password);

    wifiState = WIFI_CONNECTING;
    lastReconnectAttempt = millis();
}


void WiFiEvent(WiFiEvent_t event)
{
    switch(event)
    {

        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            DBG("WiFi connected to AP");
            break;


        case ARDUINO_EVENT_WIFI_STA_GOT_IP:

            DBG("WiFi got IP");
            DBG2("IP: ", WiFi.localIP());

            wifiState = WIFI_CONNECTED;

            wifiFailStart = 0;

            startMDNS();

            break;


        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:

            if (wifiState != WIFI_CONNECTING)
            {
                DBG("WiFi lost connection");
                wifiState = WIFI_DISCONNECTED;
            }

            break;


        default:
            break;
    }
}


void wifiBegin()
{
    DBG("Starting WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);

    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.setSleep(false);

    WiFi.onEvent(WiFiEvent);

    wifiState = WIFI_CONNECTING;

    connectWiFi();
}


bool wifiConnected()
{
    return wifiState == WIFI_CONNECTED;
}


void wifiLoop()
{
    unsigned long now = millis();

    switch (wifiState)
    {

        case WIFI_CONNECTED:
            return;


        case WIFI_CONNECTING:
        case WIFI_DISCONNECTED:

            if (wifiFailStart == 0)
            {
                wifiFailStart = now;
            }
            if (wifiState == WIFI_CONNECTING)
            {
                return;
            }
            if (now - lastReconnectAttempt > reconnectInterval)
            {
                wl_status_t status = WiFi.status();
                
                if (status == WL_DISCONNECTED || status == WL_CONNECT_FAILED)
                {
                    DBG("WiFi reconnect attempt");

                    WiFi.disconnect(false);
                    connectWiFi();
                }

                lastReconnectAttempt = now;
            }

            if (now - wifiFailStart > apFallbackDelay)
            {
                startAP();
            }

            break;


        case WIFI_AP_FALLBACK:

            if (WiFi.status() == WL_CONNECTED)
            {
                DBG("WiFi restored -> disabling AP");

                stopAP();

                wifiState = WIFI_CONNECTED;
            }

            break;
    }
}

WifiState wifiGetState()
{
    return wifiState;
}

String wifiSSID()
{
    return WiFi.SSID();
}

IPAddress wifiIP()
{
    return WiFi.localIP();
}
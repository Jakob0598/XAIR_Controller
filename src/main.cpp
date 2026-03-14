#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "wifi_manager.h"
#include "osc_manager.h"
#include "display_manager.h"
#include "webui.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    DBG("");
    DBG("================================");
    DBG("XAIR Controller Boot");
    DBG("================================");

    loadSettings();
    DBG("Settings loaded");

    wifiBegin();
    DBG("WiFi init");

    oscBegin();
    DBG("OSC init");

    displayBegin();
    DBG("Display init");

    webuiBegin();
    DBG("WebUI started");
}

void loop() {

    wifiLoop();

    oscLoop();

    displayLoop();

}
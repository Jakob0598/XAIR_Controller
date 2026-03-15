#include "config.h"

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

    networkBegin();

    displayBegin();
    DBG("Display init");

    webuiBegin();
    DBG("WebUI started");
}

void loop() {

    wifiLoop();
    networkLoop();
    oscLoop();

    displayLoop();

}
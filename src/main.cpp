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

    i2cBegin();
    DBG("I2C bus init");

    displayBegin();
    DBG("Display init");

    miniDisplayBegin();
    DBG("Mini displays init");

    ledBegin();
    DBG("LEDs init");

    batterieBegin();
    DBG("Batterie manager init");

    buttonsBegin();
    DBG("Buttons init");

    faderBegin();
    DBG("Fader init");  

    wifiBegin();
    DBG("WiFi init");

    oscBegin();
    DBG("OSC init");

    networkBegin();
    DBG("Network manager init");

    webuiBegin();
    DBG("WebUI started");

}

void loop() {

    wifiLoop();
    networkLoop();
    oscLoop();
    faderLoop();
    displayLoop();
    miniDisplayLoop();
    encoderLoop();
    buttonsLoop();

}
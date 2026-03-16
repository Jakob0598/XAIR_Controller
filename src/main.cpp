#include "config.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    DBG("");
    DBG("================================");
    DBG("XAIR Controller Boot");
    DBG("================================");

    DBG("Loading settings...");
    loadSettings();
    DBG("Settings loaded");
    DBG("");

    DBG("Initializing I2C bus...");
    i2cBegin();
    DBG("I2C bus initialized");
    DBG("");

    DBG("Initializing display...");
    displayBegin();
    DBG("Display initialized");
    DBG("");

    DBG("Initializing mini displays...");
    miniDisplayBegin();
    DBG("Mini displays initialized");
    DBG("");

    DBG("Initializing battery manager...");
    batterieBegin();
    DBG("Batterie manager initialized");
    DBG("");

    DBG("Initializing LEDs...");
    ledBegin();
    DBG("LEDs initialized");
    DBG("");

    DBG("Initializing buttons...");
    buttonsBegin();
    DBG("Buttons initialized");
    DBG("");

    DBG("Initializing encoders...");
    encoderBegin(); 
    DBG("Encoders initialized");
    DBG("");

    DBG("Initializing fader...");
    faderBegin();
    DBG("Fader initialized");
    DBG("");

    DBG("Initializing WiFi...");
    wifiBegin();
    DBG("WiFi initialized");
    DBG("");

    DBG("Initializing OSC...");
    oscBegin();
    DBG("OSC initialized");
    DBG("");

    DBG("Initializing network manager...");
    networkBegin();
    DBG("Network manager initialized");
    DBG("");

    DBG("Initializing WebUI...");
    webuiBegin();
    DBG("WebUI initialized");
    DBG("");

    DBG("================================");
    DBG("Initialization complete");
    DBG("================================");

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
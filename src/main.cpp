#include "config.h"

static uint32_t meterTimer = 0;   // reserviert für zukünftige Nutzung

void setup()
{
    if(DEBUG_SERIAL){
    Serial.begin(115200);
    delay(1000);
    }

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

    DBG("Initializing mixer defaults...");
    mixerInitDefaults();
    DBG("Mixer defaults initialized");
    DBG("");

    DBG("Initializing LEDs...");
    ledBegin();
    DBG("LEDs initialized");
    DBG("");

    DBG("Initializing buttons...");
    buttonsBegin();
    DBG("Buttons initialized");
    DBG("");

    DBG("Initializing encoder...");
    encoderBegin();
    DBG("Encoder initialized");
    DBG("");

    // ========== KOORDINIERTE BOOT-ANIMATION ==========
    // Phase 1+2: Logo, Waveform, LED-Chase, Mini-Display-Nachrichten
    // Endet mit "Calibrating..." Anzeige
    displayStartupAnimation();

    // Phase 3: Fader-Kalibrierung (blockierend, aber Display zeigt Fortschritt)
    DBG("Initializing faders...");
    faderBegin();
    faderCalibrateAll();
    DBG("Faders initialized & calibrated");
    DBG("");

    // Phase 4: "READY" Flash + Aufräumen
    displayBootComplete();

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

    meterTimer = millis();

    menuBegin();
}

void loop()
{
    wifiLoop();
    networkLoop();
    oscLoop();
    webuiUpdateMixerInfo();

    faderLoop();
    displayLoop();
    miniDisplayLoop();
    buttonsLoop();
    menuLoop();

}
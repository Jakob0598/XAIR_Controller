#include "network_manager.h"
#include "menu.h"
#include "xair_sync_manager.h"

extern bool mixerDetected;

static bool oscActive      = false;
static bool faderPushDone  = false;  // Fader nach dem ersten erfolgreichen Sync gesetzt

void networkBegin()
{
    oscActive     = false;
    faderPushDone = false;
}

void networkLoop()
{
    if (wifiConnected())
    {
        if (!oscActive)
        {
            // WiFi neu verbunden: OSC neu starten, Zustand leeren
            // Sync-Start übernimmt osc_manager sobald XAir antwortet
            oscReconnect();
            oscActive     = true;
            faderPushDone = false;

            DBG("Network: WiFi connected, waiting for XAir...");
        }
    }
    else
    {
        if (oscActive)
        {
            DBG("Network: WiFi lost");

            for (int i = 0; i < MAX_METERS; i++)
                mixerSetMeter(i, 0.0f);

            mixerName[0]  = '\0';
            mixerModel[0] = '\0';
        }
        oscActive     = false;
        faderPushDone = false;
    }

    // Fader einmalig auf Mixer-Werte fahren sobald der erste Sync fertig ist
    // Bedingung: Mixer erkannt UND kein Sync mehr aktiv UND noch nicht gemacht
    if (!faderPushDone && mixerDetected && isSystemReady() && !xairRequestRunning())
    {
        faderPushDone = true;
        DBG("Network: sync complete, moving faders to mixer positions");
        syncFadersToMixer();
    }
}
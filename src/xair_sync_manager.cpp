#include "xair_sync_manager.h"

// ==============================
// CONFIG
// ==============================

extern int mixerMaxChannels;
extern int mixerMaxBuses;

#define XAIR_CHANNEL_COUNT mixerMaxChannels
#define XAIR_BUS_COUNT mixerMaxBuses
#define XAIR_EQ_BANDS 4

#define REQUEST_INTERVAL 30   // 🔥 stabiler

// ==============================
// INTERNAL STATE
// ==============================

static bool requestActive = false;
static bool syncDone = false;

static uint32_t lastSendTime = 0;

static int ch = 1;
static int bus = 1;
static int sendBus = 1;
static int band = 1;
static int step = 0;
static int linkPair = 1;

// 🔥 FINAL STATE
static int finalStep = 0;

// 🔥 SEND GUARD
static bool sentThisCycle = false;

// ==============================
// SINGLE REQUEST STATE
// ==============================

static bool singleRequestActive = false;
static int singleType = 0;
static int singleCh = 0;
static int singleStep = 0;
static int singleBus = 1;
static int singleBand = 1;

enum
{
    REQ_NONE,
    REQ_CHANNEL,
    REQ_SENDS,
    REQ_EQ,
    REQ_CHANNEL_FULL,
    REQ_BUS,
    REQ_MAIN,
    REQ_METERS
};

// ==============================
// SAFE SEND (🔥 wichtig)
// ==============================

static bool safeSend(const char* addr)
{
    if (sentThisCycle) return false;

    oscSend(addr);
    sentThisCycle = true;
    return true;
}

// ==============================
// START
// ==============================

void xairRequestStart()
{
    if (requestActive) return;

    DBG("Starting XAir full sync...");

    requestActive = true;
    syncDone = false;

    ch = 1;
    bus = 1;
    sendBus = 1;
    band = 1;
    step = -2;    // Start mit /xinfo → /status → dann Channels
    linkPair = 1;
    finalStep = 0;

    lastSendTime = 0;
}

bool xairRequestRunning()
{
    return requestActive || singleRequestActive;
}

// ==============================
// PROCESS
// ==============================

void xairRequestProcess()
{
    if (millis() - lastSendTime < REQUEST_INTERVAL)
        return;

    lastSendTime = millis();
    sentThisCycle = false;

    char addr[64];

    // =========================
    // SINGLE REQUESTS (immer verarbeiten, auch nach syncDone)
    // =========================
    if (singleRequestActive)
    {
        switch(singleType)
        {
            case REQ_CHANNEL:
            {
                switch(singleStep)
                {
                    case 0: sprintf(addr, "/ch/%02d/mix/fader",      singleCh); break;
                    case 1: sprintf(addr, "/ch/%02d/mix/on",         singleCh); break;
                    case 2: sprintf(addr, "/ch/%02d/mix/pan",        singleCh); break;
                    case 3: sprintf(addr, "/headamp/%02d/gain",      singleCh); break;
                    case 4: sprintf(addr, "/ch/%02d/config/name",    singleCh); break;
                    case 5: sprintf(addr, "/ch/%02d/preamp/hpf",     singleCh); break;
                    case 6: sprintf(addr, "/ch/%02d/preamp/hpon",    singleCh); break;
                    case 7: sprintf(addr, "/headamp/%02d/phantom",   singleCh); break;
                    case 8: sprintf(addr, "/ch/%02d/preamp/invert",  singleCh); break;
                    case 9: sprintf(addr, "/ch/%02d/insert/sel",     singleCh); break;
                    case 10:sprintf(addr, "/ch/%02d/insert/on",      singleCh); break;
                    case 11:
                    {
                        if (singleCh % 2 == 1)
                            sprintf(addr, "/config/chlink/%d-%d", singleCh, singleCh + 1);
                        else
                        {
                            singleStep++;
                            return;
                        }
                        break;
                    }
                }

                if (!safeSend(addr)) return;

                singleStep++;
                if (singleStep > 11)
                    singleRequestActive = false;

                return;
            }

            case REQ_SENDS:
            {
                sprintf(addr, "/ch/%02d/mix/%02d/level", singleCh, singleBus);

                if (!safeSend(addr)) return;

                singleBus++;
                if (singleBus > XAIR_BUS_COUNT)
                    singleRequestActive = false;

                return;
            }

            case REQ_EQ:
            {
                switch(singleStep)
                {
                    case 0: sprintf(addr, "/ch/%02d/eq/on", singleCh); break;
                    case 1: sprintf(addr, "/ch/%02d/eq/%d/type", singleCh, singleBand); break;
                    case 2: sprintf(addr, "/ch/%02d/eq/%d/g", singleCh, singleBand); break;
                    case 3: sprintf(addr, "/ch/%02d/eq/%d/f", singleCh, singleBand); break;
                    case 4: sprintf(addr, "/ch/%02d/eq/%d/q", singleCh, singleBand); break;
                }

                if (!safeSend(addr)) return;

                singleStep++;
                if (singleStep > 4)
                {
                    singleStep = 1;
                    singleBand++;
                    if (singleBand > XAIR_EQ_BANDS)
                        singleRequestActive = false;
                }
                return;
            }

            case REQ_BUS:
                if (!safeSend("/bus/01/mix/fader")) return;
                singleRequestActive = false;
                return;

            case REQ_MAIN:
                if (!safeSend("/main/st/mix/fader")) return;
                singleRequestActive = false;
                return;

            case REQ_METERS:
                    oscSendMeters("/meters/5", 44);
                    singleRequestActive = false;
                return;
        }
    }

    // =========================
    // FULL SYNC – nur wenn nicht bereits abgeschlossen
    // =========================
    if (syncDone) return;
    if (!requestActive) return;

    if (step == -2)
    {
        // Schritt -2: /xinfo anfordern
        if (!safeSend("/xinfo")) return;
        step = -1;
        return;
    }

    if (step == -1)
    {
        // Schritt -1: /status anfordern (triggert weitere /xinfo Antwort)
        if (!safeSend("/status")) return;
        step = 0;
        // Kurze Pause damit XAir antworten kann bevor Channels abgefragt werden
        lastSendTime = millis() + 150;
        return;
    }

    // =========================
    // FULL SYNC CHANNELS
    // =========================
    if (!requestActive) return;

    if (ch <= XAIR_CHANNEL_COUNT)
    {
        switch(step)
        {
            case 0: sprintf(addr, "/ch/%02d/mix/fader",       ch); break;
            case 1: sprintf(addr, "/ch/%02d/mix/on",          ch); break;
            case 2: sprintf(addr, "/ch/%02d/mix/pan",         ch); break;
            case 3: sprintf(addr, "/headamp/%02d/gain",       ch); break;
            case 4: sprintf(addr, "/ch/%02d/config/name",     ch); break;
            case 5: sprintf(addr, "/ch/%02d/preamp/hpf",      ch); break;
            case 6: sprintf(addr, "/ch/%02d/preamp/hpon",     ch); break;
            case 7: sprintf(addr, "/headamp/%02d/phantom",    ch); break;
            case 8: sprintf(addr, "/ch/%02d/preamp/invert",   ch); break;
            case 9: sprintf(addr, "/ch/%02d/insert/sel",      ch); break;
            case 10:sprintf(addr, "/ch/%02d/insert/on",       ch); break;

            case 11:
            {
                // Stereo Link: globaler Pfad /config/chlink/X-Y
                // Nur für ungerade Kanäle anfragen (1-2, 3-4, 5-6, ...)
                if (ch % 2 == 1)
                {
                    sprintf(addr, "/config/chlink/%d-%d", ch, ch + 1);
                }
                else
                {
                    // Gerade Kanäle überspringen – Link kommt über den ungeraden Partner
                    step++;
                    return;
                }
                break;
            }

            case 12:
                sprintf(addr, "/ch/%02d/mix/%02d/level", ch, sendBus);
                if (!safeSend(addr)) return;

                sendBus++;
                if (sendBus > XAIR_BUS_COUNT)
                {
                    sendBus = 1;
                    step++;
                }
                return;

            case 13: sprintf(addr, "/ch/%02d/eq/on", ch); break;
            case 14: sprintf(addr, "/ch/%02d/eq/%d/type", ch, band); break;
            case 15: sprintf(addr, "/ch/%02d/eq/%d/g", ch, band); break;
            case 16: sprintf(addr, "/ch/%02d/eq/%d/f", ch, band); break;

            case 17:
                sprintf(addr, "/ch/%02d/eq/%d/q", ch, band);
                if (!safeSend(addr)) return;

                band++;
                if (band > XAIR_EQ_BANDS)
                {
                    band = 1;
                    step = 0;
                    ch++;
                }
                else
                {
                    step = 14;
                }
                return;
        }

        if (!safeSend(addr)) return;

        step++;
        return;
    }

    // =========================
    // BUS
    // =========================
    if (bus <= XAIR_BUS_COUNT)
    {
        sprintf(addr, "/bus/%01d/mix/fader", bus);
        if (!safeSend(addr)) return;

        bus++;
        return;
    }

    // =========================
    // FINAL 
    // =========================
    if (finalStep == 0)
    {
        if (!safeSend("/lr/mix/fader")) return;
        finalStep = 1;
        return;
    }
    else if (finalStep == 1)
    {
        oscSendMeters("/meters/5", 44);
        finalStep = 2;
        return;
    }

    // =========================
    // DONE
    // =========================
    DBG("XAir sync complete");

    requestActive = false;
    syncDone = true;
}
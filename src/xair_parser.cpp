#include "xair_parser.h"

// =============================
// VALUE
// =============================
static bool getValue(OSCMessage& msg, float& out)
{
    if (msg.size() == 0) return false;

    if (msg.isFloat(0)) { out = msg.getFloat(0); return true; }
    if (msg.isInt(0))   { out = (float)msg.getInt(0); return true; }

    out = msg.getFloat(0);
    return true;
}

// =============================
// TOKEN PARSER
// =============================
static int splitAddress(const char* addr, char parts[][16], int maxParts)
{
    int count = 0;
    const char* start = addr;

    while (*start && count < maxParts)
    {
        if (*start == '/') start++;

        const char* end = strchr(start, '/');
        if (!end) end = start + strlen(start);

        int len = end - start;
        if (len > 0)
        {
            strncpy(parts[count], start, len);
            parts[count][len] = '\0';
            count++;
        }

        start = end;
    }

    return count;
}

// =============================
// CHANNEL
// =============================
void xairHandleChannel(OSCMessage& msg, const char* address)
{
    char parts[8][16];
    int count = splitAddress(address, parts, 8);

    if (count < 2) return;

    int ch = atoi(parts[1]);
    if (ch < 1) return;

    DBG2("RX:", address);

    float v;
    bool hasValue = getValue(msg, v);

    // =============================
    // MIX
    // =============================
    if (strcmp(parts[2], "mix") == 0)
    {
        if (count == 4 && strcmp(parts[3], "fader") == 0)
        {
            if (!hasValue) return;
            mixerSetFader(ch, v);
            DBG3("FADER", ch, v);
            return;
        }

        if (count == 4 && strcmp(parts[3], "on") == 0)
        {
            if (!hasValue) return;
            mixerSetMute(ch, (int)v == 0);
            DBG3("MUTE", ch, v);
            return;
        }

        if (count == 4 && strcmp(parts[3], "pan") == 0)
        {
            if (!hasValue) return;
            mixerSetPan(ch, v);
            DBG3("PAN", ch, v);
            return;
        }

        // SENDS
        if (count == 5 && strcmp(parts[4], "level") == 0)
        {
            if (!hasValue) return;

            int bus = atoi(parts[3]);
            if (bus >= 1)
            {
                mixerSetSend(ch, bus, v);
                DBG3("SEND", ch, v);
            }
            return;
        }
    }

    // =============================
    // PREAMP
    // =============================
    if (strcmp(parts[2], "preamp") == 0)
    {
        if (!hasValue) return;

        if (strcmp(parts[3], "trim") == 0 || strcmp(parts[3], "gain") == 0)
        {
            mixerSetGain(ch, v);
            DBG3("GAIN", ch, v);
        }
        else if (strcmp(parts[3], "hpf") == 0)
        {
            mixerSetHPFFreq(ch, v);
            DBG3("HPF FREQ", ch, v);
        }
        else if (strcmp(parts[3], "hpon") == 0)
        {
            mixerSetHPFOn(ch, (int)v == 1);
            DBG3("HPF ON", ch, v);
        }
        else if (strcmp(parts[3], "phantom") == 0)
        {
            mixerSetPhantom(ch, (int)v == 1);
            DBG3("PHANTOM", ch, (int)v);
        }
        else if (strcmp(parts[3], "invert") == 0 || strcmp(parts[3], "polarity") == 0)
        {
            mixerSetPolarity(ch, (int)v == 1);
            DBG3("POLARITY", ch, (int)v);
        }
        return;
    }

    // =============================
    // CONFIG  (name + stereolink)
    // =============================
    if (strcmp(parts[2], "config") == 0)
    {
        if (strcmp(parts[3], "name") == 0)
        {
            char name[32];
            msg.getString(0, name, sizeof(name));
            mixerSetName(ch, name);
            DBG2("NAME:", name);
        }
        return;
    }

    // =============================
    // INSERT  (/ch/XX/insert/on, /ch/XX/insert/sel)
    // =============================
    if (strcmp(parts[2], "insert") == 0)
    {
        if (!hasValue) return;

        if (strcmp(parts[3], "on") == 0)
        {
            mixerSetInsOn(ch, (int)v == 1);
            DBG3("INSERT ON", ch, (int)v);
        }
        else if (strcmp(parts[3], "sel") == 0)
        {
            mixerSetInsMode(ch, (InsMode)constrain((int)v, 0, 4));
            DBG3("INSERT SEL", ch, (int)v);
        }
        return;
    }

    // =============================
    // EQ
    // =============================
    if (strcmp(parts[2], "eq") == 0)
    {
        // GLOBAL EQ ON/OFF
        if (count == 4 && strcmp(parts[3], "on") == 0)
        {
            if (!hasValue) return;
            channels[ch].eqOn = (int)v == 1;
            DBG3("EQ ON", ch, v);
            return;
        }

        if (count < 5) return;

        int band = atoi(parts[3]);
        if (band < 1) return;

        if (!hasValue) return;

        if (strcmp(parts[4], "g") == 0)
        {
            mixerSetEqGain(ch, band, v);
            DBG3("EQ G", band, v);
        }
        else if (strcmp(parts[4], "f") == 0)
        {
            mixerSetEqFreq(ch, band, v);
            DBG3("EQ F", band, v);
        }
        else if (strcmp(parts[4], "q") == 0)
        {
            mixerSetEqQ(ch, band, v);
            DBG3("EQ Q", band, v);
        }
        else if (strcmp(parts[4], "type") == 0)
        {
            channels[ch].eq[band].type = (EqType)(int)v;
            DBG3("EQ TYPE", band, (int)v);
        }
        return;
    }
}

// =============================
// HEADAMP (/headamp/XX/gain, /headamp/XX/phantom)
// XX is 0-based index (00 = channel 1)
// =============================
void xairHandleHeadamp(OSCMessage& msg, const char* address)
{
    char parts[6][16];
    int count = splitAddress(address, parts, 6);

    if (count < 3) return;

    int ch = atoi(parts[1]);   // 1-based (01 = channel 1)
    if (ch < 1) return;

    float v;
    if (!getValue(msg, v)) return;

    if (strcmp(parts[2], "gain") == 0)
    {
        mixerSetGain(ch, v);
        DBG3("HEADAMP GAIN", ch, v);
    }
    else if (strcmp(parts[2], "phantom") == 0)
    {
        mixerSetPhantom(ch, (int)v == 1 || v > 0.5f);
        DBG3("HEADAMP PHANTOM", ch, (int)v);
    }
}

// =============================
// BUS
// =============================
void xairHandleBus(OSCMessage& msg, const char* address)
{
    char parts[6][16];
    int count = splitAddress(address, parts, 6);

    if (count < 4) return;

    int bus = atoi(parts[1]);
    if (bus < 1) return;

    float v;
    if (!getValue(msg, v)) return;

    if (strcmp(parts[2], "mix") == 0 && strcmp(parts[3], "fader") == 0)
    {
        mixerSetBusFader(bus, v);
        DBG3("BUS FADER", bus, v);
    }
}

// =============================
// MAIN
// =============================
void xairHandleMain(OSCMessage& msg, const char* address)
{
    char parts[6][16];
    int count = splitAddress(address, parts, 6);

    if (count < 3) return;

    float v;
    if (!getValue(msg, v)) return;

    if (strcmp(parts[1], "mix") == 0 &&
        strcmp(parts[2], "fader") == 0)
    {
        mixerSetMainFader(v);
        DBG2("MAIN FADER:", v);
    }
}

// =============================
// METERS
// =============================
void xairHandleMeters(OSCMessage& msg, const char* address)
{
    if (!msg.isBlob(0)) return;

    const uint8_t* blob = msg.getBlob(0);
    int size = msg.getBlobLength(0);

    if (size < 4) return;

    // Anzahl Werte aus dem Blob-Header
    uint32_t count;
    memcpy(&count, blob, 4);

    const uint8_t* data = blob + 4;

    // -------------------------------------------------------
    // /meters/5  – ALL OUTPUTS: 6 aux, lr, 16 p16, 18 usb, hphones
    //   Index 6 = LR L, Index 7 = LR R  (post-fader stereo out)
    // -------------------------------------------------------
    if (strstr(address, "/meters/5") != nullptr)
    {
        if (count < 8) return;

        int16_t rawL, rawR;
        memcpy(&rawL, data + 6 * 2, 2);
        memcpy(&rawR, data + 7 * 2, 2);

        float dbL = rawL / 256.0f;
        float dbR = rawR / 256.0f;

        // dB → linear (0–1), Clamp bei 0 dBFS
        float linL = constrain(powf(10.0f, dbL / 20.0f), 0.0f, 1.0f);
        float linR = constrain(powf(10.0f, dbR / 20.0f), 0.0f, 1.0f);

        mixerSetMeter(0, linL);  // Main L
        mixerSetMeter(1, linR);  // Main R

        DBG3("MASTER METERS dB L/R:", dbL, dbR);
        return;
    }

    // -------------------------------------------------------
    // /meters/1  – ALL CHANNELS (pre-fader):
    //   16 mono (Index 0-15) + 5x2 fx/aux + 6 bus + 4 fx send + 2 st + 2 monitor
    //   = 40 Werte insgesamt
    //   Wir nehmen Index 0–15 = CH01–CH16, je linker Kanal
    // -------------------------------------------------------
    if (strstr(address, "/meters/1") != nullptr)
    {
        if ((int)count < 16) return;

        for (int ch = 0; ch < 16 && ch < (int)count; ch++)
        {
            int16_t raw;
            memcpy(&raw, data + ch * 2, 2);

            float db  = raw / 256.0f;
            float lin = constrain(powf(10.0f, db / 20.0f), 0.0f, 1.0f);

            // Meter-Slot 2+ = Channel 1-16
            mixerSetMeter(2 + ch, lin);
        }
        return;
    }
}

// =============================
// CONFIG (global paths like /config/chlink/1-2)
// =============================
void xairHandleConfig(OSCMessage& msg, const char* address)
{
    DBG2("RX CONFIG:", address);

    // /config/chlink/1-2, /config/chlink/3-4, etc.
    if (strncmp(address, "/config/chlink/", 15) == 0)
    {
        const char* pair = address + 15;  // "1-2", "3-4", etc.
        int chLow = 0, chHigh = 0;
        if (sscanf(pair, "%d-%d", &chLow, &chHigh) == 2)
        {
            float v;
            if (getValue(msg, v))
            {
                bool linked = ((int)v == 1);
                if (chLow >= 1 && chLow < MAX_CHANNELS)
                    channels[chLow].stereoLinked = linked;
                if (chHigh >= 1 && chHigh < MAX_CHANNELS)
                    channels[chHigh].stereoLinked = linked;
                DBG3("CHLINK", chLow, (int)v);
            }
        }
    }
}

// =============================
// GLOBAL (XINFO / STATUS)
// =============================
void xairHandleGlobal(OSCMessage& msg, const char* address)
{
    DBG2("RX GLOBAL:", address);

    // -------------------------
    // /xinfo
    // -------------------------
    if (strncmp(address, "/xinfo", 6) == 0)
    {
        if (msg.size() < 4) return;

        char ip[32];
        char model[32];
        char name[32];
        char fw[16];

        msg.getString(0, ip,    sizeof(ip));
        msg.getString(1, model, sizeof(model));
        msg.getString(2, name,  sizeof(name));
        msg.getString(3, fw,    sizeof(fw));

        DBG2("IP:",    ip);
        DBG2("MODEL:", model);
        DBG2("NAME:",  name);
        DBG2("FW:",    fw);

        mixerSetMixerModel(model);
        mixerSetMixerName(name);

        // Limits immer setzen – auch wenn mixerDetected schon true ist,
        // damit ein Reconnect die Kanal-Anzahl korrekt aktualisiert
        if (strcmp(model, "XR12") == 0)
        {
            mixerSetLimits(12, 2);
        }
        else if (strcmp(model, "XR16") == 0)
        {
            mixerSetLimits(16, 4);
        }
        else if (strcmp(model, "XR18") == 0)
        {
            mixerSetLimits(18, 6);
        }
        else
        {
            // Unbekanntes Modell: konservativ auf 16/6
            mixerSetLimits(16, 6);
        }

        mixerDetected = true;
        DBG2("Mixer detected:", model);

        return;
    }

    // -------------------------
    // /status
    // -------------------------
    if (strcmp(address, "/status") == 0)
    {
        DBG("STATUS RECEIVED");
        oscSend("/xinfo");
        return;
    }
}
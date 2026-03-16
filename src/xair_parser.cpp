#include "xair_parser.h"

static int parseChannel(const char* addr)
{
    if (strncmp(addr, "/ch/", 4) != 0) return -1;

    char num[3];
    num[0] = addr[4];
    num[1] = addr[5];
    num[2] = 0;

    return atoi(num) - 1;
}

static int parseEqBand(const char* addr)
{
    const char* p = strstr(addr, "/eq/");
    if (!p) return -1;

    return p[4] - '1';
}

static int parseSendBus(const char* addr)
{
    const char* p = strstr(addr, "/mix/");
    if (!p) return -1;

    return atoi(p + 5) - 1;
}

void xairHandleChannel(OSCMessage& msg, const char* address)
{
    int ch = parseChannel(address);
    if (ch < 0) return;

    /* ---------- FADER ---------- */

    if (strstr(address, "/mix/fader"))
    {
        float v = msg.getFloat(0);
        mixerSetFader(ch, v);
        return;
    }

    /* ---------- MUTE ---------- */

    if (strstr(address, "/mix/on"))
    {
        int v = msg.getInt(0);
        mixerSetMute(ch, v == 0);
        return;
    }

    /* ---------- PAN ---------- */

    if (strstr(address, "/mix/pan"))
    {
        float v = msg.getFloat(0);
        mixerSetPan(ch, v);
        return;
    }

    /* ---------- GAIN ---------- */

    if (strstr(address, "/preamp/gain"))
    {
        float v = msg.getFloat(0);
        mixerSetGain(ch, v);
        return;
    }

    /* ---------- SEND LEVEL ---------- */

    if (strstr(address, "/mix/") && strstr(address, "/level"))
    {
        int bus = parseSendBus(address);
        float v = msg.getFloat(0);

        mixerSetSend(ch, bus, v);
        return;
    }

    /* ---------- EQ ---------- */

    if (strstr(address, "/eq/"))
    {
        int band = parseEqBand(address);

        if (band < 0) return;

        if (strstr(address, "/g"))
        {
            mixerSetEqGain(ch, band, msg.getFloat(0));
        }
        else if (strstr(address, "/f"))
        {
            mixerSetEqFreq(ch, band, msg.getFloat(0));
        }
        else if (strstr(address, "/q"))
        {
            mixerSetEqQ(ch, band, msg.getFloat(0));
        }

        return;
    }

    /* ---------- HPF ---------- */

    if (strstr(address, "/preamp/hpf"))
    {
        float v = msg.getFloat(0);
        mixerSetHPFFreq(ch, v);
        return;
    }

    if (strstr(address, "/preamp/hpon"))
    {
        int v = msg.getInt(0);
        mixerSetHPFOn(ch, v == 1);
        return;
    }

    /* ---------- COMPRESSOR ---------- */

    if (strstr(address, "/dyn/"))
    {
        if (strstr(address, "threshold"))
            mixerSetCompThreshold(ch, msg.getFloat(0));

        else if (strstr(address, "ratio"))
            mixerSetCompRatio(ch, msg.getFloat(0));

        else if (strstr(address, "attack"))
            mixerSetCompAttack(ch, msg.getFloat(0));

        else if (strstr(address, "release"))
            mixerSetCompRelease(ch, msg.getFloat(0));

        return;
    }

    /* ---------- GATE ---------- */

    if (strstr(address, "/gate/"))
    {
        if (strstr(address, "thr"))
            mixerSetGateThreshold(ch, msg.getFloat(0));

        return;
    }

    /* ---------- NAME ---------- */

    if (strstr(address, "/config/name"))
    {
        char name[32];
        msg.getString(0, name, sizeof(name));

        mixerSetName(ch, name);
        return;
    }
}

void xairHandleBus(OSCMessage& msg, const char* address)
{
    int bus = atoi(&address[5]) - 1;

    if (bus < 0) return;

    if (strstr(address, "/mix/fader"))
    {
        float v = msg.getFloat(0);
        mixerSetBusFader(bus, v);
    }
}

void xairHandleMain(OSCMessage& msg, const char* address)
{
    if (strstr(address, "/mix/fader"))
    {
        float v = msg.getFloat(0);
        mixerSetMainFader(v);
    }
}

void xairHandleMeters(OSCMessage& msg, const char* address)
{
    int count = msg.size();

    for (int i = 0; i < count; i++)
    {
        float meter = msg.getFloat(i);
        mixerSetMeter(i, meter);
    }
}
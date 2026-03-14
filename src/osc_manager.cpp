#include "osc_manager.h"
#include "settings.h"
#include "mixer_state.h"
#include "webui.h"
#include "config.h"

#include <WiFiUdp.h>
#include <OSCMessage.h>

 WiFiUDP Udp;
 IPAddress outIP;


void oscBroadcast(const char* address)
{
    OSCMessage msg(address);

    Udp.beginPacket(IPAddress(255,255,255,255), settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG2("oscBroadcast: ",address);
}

void oscSend(const char* address)
{
    OSCMessage msg(address);

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG2("oscSend: ",address);
}

void oscSendFloat(const char* address, float value)
{
    OSCMessage msg(address);
    msg.add(value);

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG3("oscSendFloat: ",address, value);
}

void oscSendInt(const char* address, int value)
{
    OSCMessage msg(address);
    msg.add(value);

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG3("oscSendInt: ",address, value);
}

void oscBegin()
{
    outIP = IPAddress(
        settings.ip[0],
        settings.ip[1],
        settings.ip[2],
        settings.ip[3]
    );

    Udp.begin(settings.receivePort);

    DBG("OSC started");

    DBG2("Send IP: ", outIP);
    DBG2("Send Port: ", settings.sendPort);
    DBG2("Receive Port: ", settings.receivePort);
}

void oscLoop()
{
    OSCMessage msg;
    int size = Udp.parsePacket();
    if (size > 0)
    {
        DBG2("OSC Received by IP: ", Udp.remoteIP());
        while (size--)
        {
            msg.fill(Udp.read());
        }

        if (!msg.hasError())
        {
            char address[64];
            msg.getAddress(address);
            DBG2("OSC Address: ", address);
            for (int i = 0; i < msg.size(); i++)
{
    if (msg.isInt(i))
    {
        int value = msg.getInt(i);
        DBG3("Int: ", i, value);
    }

    else if (msg.isFloat(i))
    {
        float value = msg.getFloat(i);
        DBG3("Float: ", i, value);
    }

    else if (msg.isDouble(i))
    {
        double value = msg.getDouble(i);
        DBG3("Double: ", i, value);
    }

    else if (msg.isString(i))
    {
        char value[64];
        msg.getString(i, value, 64);
        DBG3("String: ", i, value);
    }

    else if (msg.isBoolean(i))
    {
        bool value = msg.getBoolean(i);
        DBG3("Bool: ", i, value);
    }

    else
    {
        DBG2("Unknown type index: ", i);
    }
}
        }
        else
        {
            DBG("OSC error");
        }
    }
}


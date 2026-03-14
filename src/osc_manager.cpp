#include "osc_manager.h"
#include "settings.h"
#include "mixer_state.h"
#include "webui.h"
#include "config.h"

#include <WiFiUdp.h>
#include <OSCMessage.h>

WiFiUDP Udp;
IPAddress outIp;

void oscBegin()
{
    outIp = IPAddress(
        settings.ip[0],
        settings.ip[1],
        settings.ip[2],
        settings.ip[3]
    );

    Udp.begin(settings.receivePort);

    DBG("OSC started");

    DBG2("Send IP: ", outIp);
    DBG2("Send Port: ", settings.sendPort);
    DBG2("Receive Port: ", settings.receivePort);
}

void sendOSCFloat(const char* addr,float value) {

    OSCMessage msg(addr);
    msg.add(value);

    Udp.beginPacket(outIp,settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
}

void oscLoop()
{
    OSCMessage msg;
    int size = Udp.parsePacket();

    if(size<=0) return;

    uint8_t buf[size];

    Udp.read(buf,size);

    msg.fill(buf,size);

    if(msg.hasError())
    {
        DBG("OSC error");
        return;
    }

    String addr = msg.getAddress();

    DBG2("OSC RX: ", addr);

    webuiUpdateOSC(addr);

    if(addr.startsWith("/ch/")) {

    int ch = addr.substring(4,6).toInt()-1;

    if(addr.endsWith("/mix/fader"))
{
    float value = msg.getFloat(0);

    DBG2("Fader value: ", value);

    mixer.ch[ch].fader = value;
}

    if(addr.indexOf("/eq/") != -1) {

        int band = addr.substring(7,8).toInt()-1;

        if(addr.endsWith("/f"))
            mixer.ch[ch].eq[band].freq = msg.getFloat(0);

        if(addr.endsWith("/g"))
            mixer.ch[ch].eq[band].gain = msg.getFloat(0);

        if(addr.endsWith("/q"))
            mixer.ch[ch].eq[band].q = msg.getFloat(0);
    }

    if(addr.indexOf("/mix/") != -1) {

        for(int a=0;a<MAX_AUX;a++) {

            String path="/mix/"+String(a+1);

            if(addr.endsWith(path))
                mixer.ch[ch].aux[a] = msg.getFloat(0);

        }

    }

}

}
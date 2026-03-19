#include "osc_manager.h"

WiFiUDP Udp;
IPAddress outIP;
static unsigned long oscReadyTime = 0;
static unsigned long lastXremote = 0;

static bool xairSynced = false;
static bool systemReady = false;
static bool oscAlive = false;

bool mixerDetected = false;

bool isSystemReady()
{
    if (WiFi.status() != WL_CONNECTED){
        return false;
    }
    if (!oscAlive){
        return false;
    }
    return true;
}

void oscBroadcast(const char* address)
{
    if (!wifiConnected()){return;}
    OSCMessage msg(address);

    Udp.beginPacket(IPAddress(255,255,255,255), settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG2("oscBroadcast: ",address);
}

void oscSend(const char* address)
{
    if (!wifiConnected()){return;}
    OSCMessage msg(address);

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG2("oscSend: ",address);
}

void oscSendFloat(const char* address, float value)
{
    if (!wifiConnected()){return;}
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
    if (!wifiConnected()){return;}
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
    oscRegisterRoutes();
    Udp.begin(settings.receivePort);
    Udp.setTimeout(0);
    oscReadyTime = millis() + 200;
    DBG("OSC started");
    DBG2("Send IP: ", outIP.toString());
    DBG2("Send Port: ", settings.sendPort);
    DBG2("Receive Port: ", settings.receivePort);
}

void oscReconnect()
{
    DBG("OSC reconnect");

    Udp.stop();
    delay(200);

    Udp.begin(settings.receivePort);

    oscReadyTime = millis();

    DBG2("Receive Port: ", settings.receivePort);
  
    lastXremote = 0;
    systemReady = false;
    xairSynced = false;
    oscAlive = false;
    mixerDetected = false;
}

void oscLoop()
{
    if (!wifiConnected()){
        return;
    }
    if (millis() < oscReadyTime + 200){
        return;
    }
    
    
    if (!xairRequestRunning() && millis() - lastXremote > RefreshXremoteInterval)
    {
        if(oscAlive){
            oscSend("/xremote");
        }
        else{
            oscSend("/status");
        }
        lastXremote = millis();
    }

    int size = Udp.parsePacket();

    if (size > 0)
    {

        OSCMessage msg;

        for(int i = 0; i < size; i++)
            msg.fill(Udp.read());

        if (!msg.hasError())
        {
            char address[OSC_BUFFER_SIZE] = {0};
            msg.getAddress(address, 0);
            oscDispatch(msg, address);
            oscAlive = true;
        }
        else
        {
            DBG("OSC error");
        }
    }

    xairRequestProcess();      // dein Chunk-System

    // =========================
    // SYSTEM READY CHECK
    // =========================
    

    if (!systemReady && isSystemReady())
    {
        if (millis() > oscReadyTime + 500)   // 500ms stabil
        {
            DBG("System stable → starting sync");

            systemReady = true;
            xairSynced = false;
        }
    }

    // =========================
    // INITIAL SYNC (GENAU EINMAL)
    // =========================
    if (systemReady && !xairSynced)
    {
        xairRequestStart();
        xairSynced = true;
    }
}

void oscSendMeters(const char* meterId, int packs)
{
    if (!wifiConnected()) return;

    OSCMessage msg("/meters");

    msg.add(meterId);  // STRING!
    msg.add(packs);        // INT!

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();

    DBG2("oscSendMeters:", meterId);
}





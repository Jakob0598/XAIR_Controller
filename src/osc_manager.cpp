#include "osc_manager.h"

WiFiUDP Udp;
IPAddress outIP;
static unsigned long oscReadyTime = 0;
static unsigned long lastXremote = 0;

static bool xairSynced = false;
static bool systemReady = false;
static bool oscAlive = false;

bool mixerDetected = false;

// Timeout: wenn >12s keine OSC-Antwort → Verbindung verloren
#define OSC_ALIVE_TIMEOUT 12000
static unsigned long lastOscRxTime = 0;

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

// Wird aufgerufen wenn Verbindung verloren geht
static void oscHandleDisconnect()
{
    if (!oscAlive) return;  // bereits disconnected

    DBG("OSC: XAir connection lost (timeout)");

    oscAlive = false;
    systemReady = false;
    xairSynced = false;
    mixerDetected = false;

    // Mixer-Name/Model leeren damit Statusbar reagiert
    mixerName[0] = '\0';
    mixerModel[0] = '\0';

    // Alle Meter auf 0 setzen
    for (int i = 0; i < MAX_METERS; i++)
        mixerSetMeter(i, 0.0f);
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

void oscSendString(const char* address, const char* value)
{
    if (!wifiConnected()) return;
    OSCMessage msg(address);
    msg.add(value);

    Udp.beginPacket(outIP, settings.sendPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
    DBG3("oscSendString: ", address, value);
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
    lastOscRxTime = millis();

    // Name/Model leeren damit Statusbar sauber neu startet
    mixerName[0]  = '\0';
    mixerModel[0] = '\0';

    // Meter auf 0
    for (int i = 0; i < MAX_METERS; i++)
        mixerSetMeter(i, 0.0f);
}

void oscLoop()
{
    if (!wifiConnected()){
        // WiFi verloren → sofort alles zurücksetzen
        if (oscAlive) oscHandleDisconnect();
        return;
    }
    if (millis() < oscReadyTime + 200){
        return;
    }

    // Timeout-Prüfung: XAir antwortet nicht mehr
    if (oscAlive && (millis() - lastOscRxTime > OSC_ALIVE_TIMEOUT))
    {
        oscHandleDisconnect();
    }
    
    
    if (!xairRequestRunning() && millis() - lastXremote > RefreshXremoteInterval)
    {
        if(oscAlive)
        {
            oscSend("/xremote");

            // /meters/1 (alle Channel pre-fader) – Subscription läuft ~10s, alle 8s erneuern
            // Nur senden wenn XAir verbunden, spart unnötige Pakete
            oscSendMeters("/meters/1", 40);

            // /meters/5 (Master L/R outputs) – wie bisher
            oscSendMeters("/meters/5", 44);
        }
        else
        {
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
            lastOscRxTime = millis();  // Timestamp aktualisieren
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
    // Warte auf: WiFi + oscAlive + mixerDetected (Name/Model empfangen)

    if (!systemReady && isSystemReady() && mixerDetected)
    {
        if (millis() > oscReadyTime + 500)   // 500ms stabil
        {
            DBG("System stable + mixer detected → starting sync");

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
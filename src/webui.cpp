#include "webui.h"



uint16_t wifi_ssid_text;
uint16_t wifi_pass_text;

uint16_t ip1Field, ip2Field, ip3Field, ip4Field;
uint16_t sendPortField, receivePortField;

uint16_t mixerNameLabel;
uint16_t mixerModelLabel;
uint16_t mixerStatusLabel;

/* ---------------------------------------------------------- */

uint8_t validIP(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

uint16_t validPort(int port)
{
    if (port < 1) return 1;
    if (port > 65535) return 65535;
    return port;
}

/* ---------------------------------------------------------- */

void textCallback(Control *sender, int type) {}

/* WIFI SAVE */

void saveWifiCallback(Control *sender, int type)
{
    if(type!=B_UP) return;

    DBG("Saving WiFi settings");

    String ssid = ESPUI.getControl(wifi_ssid_text)->value;

    DBG2("New SSID: ", ssid);
    
    String pass = ESPUI.getControl(wifi_pass_text)->value;

    ssid.toCharArray(settings.ssid,32);
    pass.toCharArray(settings.password,64);

    saveSettings();

    delay(500);
    ESP.restart();
}

/* NETWORK SAVE */

void saveNetworkCallback(Control *sender, int type)
{
    if(type != B_UP) return;

    settings.ip[0] = validIP(ESPUI.getControl(ip1Field)->value.toInt());
    settings.ip[1] = validIP(ESPUI.getControl(ip2Field)->value.toInt());
    settings.ip[2] = validIP(ESPUI.getControl(ip3Field)->value.toInt());
    settings.ip[3] = validIP(ESPUI.getControl(ip4Field)->value.toInt());

    settings.sendPort =
        validPort(ESPUI.getControl(sendPortField)->value.toInt());

    settings.receivePort =
        validPort(ESPUI.getControl(receivePortField)->value.toInt());

    saveSettings();

    delay(500);
    ESP.restart();
}

/* ---------------------------------------------------------- */

void webuiBegin()
{
    DBG("Starting WebUI...");

    ESPUI.setVerbosity(Verbosity::Quiet);

    /* CONTROL TAB */

    auto maintab = ESPUI.addControl(Tab,"","Info");

// 🔹 Section
ESPUI.addControl(Separator,"Mixer Info","",None, maintab);

// 🔹 Status
mixerStatusLabel = ESPUI.addControl(
    Label,
    "Status",
    "Not connected",
    None,
    maintab
);

// 🔹 Name
mixerNameLabel = ESPUI.addControl(
    Label,
    "Name",
    "-",
    None,
    maintab
);

// 🔹 Model
mixerModelLabel = ESPUI.addControl(
    Label,
    "Model",
    "-",
    None,
    maintab
);

    /* UDP TAB */

    auto iptab = ESPUI.addControl(Tab,"","UDP");

    auto ipPanel =
        ESPUI.addControl(Separator,"Host IP","",None,iptab);

    ip1Field =
        ESPUI.addControl(Text,"",String(settings.ip[0]),None,ipPanel,textCallback);

    ip2Field =
        ESPUI.addControl(Text,"",String(settings.ip[1]),None,ipPanel,textCallback);

    ip3Field =
        ESPUI.addControl(Text,"",String(settings.ip[2]),None,ipPanel,textCallback);

    ip4Field =
        ESPUI.addControl(Text,"",String(settings.ip[3]),None,ipPanel,textCallback);

    auto sendPortPanel =
        ESPUI.addControl(Separator,"Send Port","",None,iptab);

    sendPortField =
        ESPUI.addControl(Text,"",String(settings.sendPort),None,sendPortPanel,textCallback);

    auto receivePortPanel =
        ESPUI.addControl(Separator,"Receive Port","",None,iptab);

    receivePortField =
        ESPUI.addControl(Text,"",String(settings.receivePort),None,receivePortPanel,textCallback);

    ESPUI.addControl(
        Button,
        "Save Network",
        "Save & Reboot",
        Peterriver,
        receivePortPanel,
        saveNetworkCallback
    );

    /* WIFI TAB */

    auto wifitab = ESPUI.addControl(Tab,"","WiFi");

    auto wifiPanel =
        ESPUI.addControl(Separator,"WiFi Settings","",None,wifitab);

    wifi_ssid_text =
        ESPUI.addControl(Text,"SSID","SSID",None,wifiPanel,textCallback);

    wifi_pass_text =
        ESPUI.addControl(Text,"Password","Password",None,wifiPanel,textCallback);

    ESPUI.addControl(
        Button,
        "Save WiFi",
        "Save & Reboot",
        Peterriver,
        wifiPanel,
        saveWifiCallback
    );

    /* CSS */

    ESPUI.setCustomCSS(

        "body {background-color:#1e1e1e;color:white;}"
        ".panel {background-color:#2d2d2d;}"

        "#conStatus.color-green {background-color:#2196F3 !important;}"
        "#conStatus {float:right;}"

        ".sectionbreak h5 {color:white;}"

        "input::placeholder {color:#aaa;}"

        "#tab7 #id8 input {width:45px; display:inline-block; text-align:center;}"
        "#tab7 #id13 input {width:60px; display:inline-block; text-align:center;}"
        "#tab7 #id15 input {width:60px; display:inline-block; text-align:center;}"
        "#tab7 button {display:block; margin-top:10px; margin-left:-5px;}"

        "#tab18 button {display:block; margin-top:10px; margin-left:-5px;}"

        "#mainHeader::after{content:'by Jakob'; font-size:25px !important; color:#888; display:inline-block; transform:scale(0.5) translateY(10px); margin-left:-15px;}"

    );

    ESPUI.begin("XAIR Controller");
}

/* ---------------------------------------------------------- */

void webuiUpdateMixerInfo()
{
    // 🔹 Name
    if(strlen(mixerName) > 0)
        ESPUI.updateControlValue(mixerNameLabel, String(mixerName));
    else
        ESPUI.updateControlValue(mixerNameLabel, "-");

    // 🔹 Model
    if(strlen(mixerModel) > 0)
        ESPUI.updateControlValue(mixerModelLabel, String(mixerModel));
    else
        ESPUI.updateControlValue(mixerModelLabel, "-");

    // 🔹 Status
    if(strlen(mixerName) > 0)
        ESPUI.updateControlValue(mixerStatusLabel, "Connected");
    else
        ESPUI.updateControlValue(mixerStatusLabel, "Not connected");
}
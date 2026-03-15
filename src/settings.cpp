#include "config.h"


Settings settings;

void setDefaults() {

    settings.magic = SETTINGS_MAGIC;

    strcpy(settings.ssid,"");
    strcpy(settings.password,"");

    settings.ip[0]=192;
    settings.ip[1]=168;
    settings.ip[2]=1;
    settings.ip[3]=100;

    settings.sendPort = 10024;
    settings.receivePort = 10023;
}

void saveSettings() {

    EEPROM.begin(EEPROM_SIZE);
    EEPROM.put(0,settings);
    EEPROM.commit();
    EEPROM.end();
}

void loadSettings()
{
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.get(0,settings);
    EEPROM.end();

    if(settings.magic != SETTINGS_MAGIC)
    {
        DBG("EEPROM empty -> loading defaults");

        setDefaults();
        saveSettings();
    }
    else
    {
        DBG("Settings loaded from EEPROM");
    }

    DBG2("Send port: ", settings.sendPort);
    DBG2("Receive port: ", settings.receivePort);
}
#pragma once

#define HOSTNAME "XAIR_Controller"

#define DEBUG_SERIAL true

#if DEBUG_SERIAL
  #define DBG(x) Serial.println(x)
  #define DBG2(x,y) { Serial.print(x); Serial.print(" "); Serial.println(y); }
  #define DBG3(x,y,z) { Serial.print(x); Serial.print(" "); Serial.print(y); Serial.print(" "); Serial.println(z); }
#else
  #define DBG(x)
  #define DBG2(x,y)
  #define DBG3(x,y,z)
#endif

#define EEPROM_SIZE 256
#define SETTINGS_MAGIC 4242

#define WIFI_RECONNECT_INTERVAL 2000
#define WIFI_AP_TIMEOUT 30000

#define OSC_BUFFER_SIZE 256

#define MAX_CHANNELS 16
#define MAX_AUX 6


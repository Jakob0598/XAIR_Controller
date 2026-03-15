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


//ST7789:
//MOSI -> 23
//CLK -> 18
//CS -> 5
//DC -> 16
//RST -> 17
//BL -> 4
#define TOUCH_CS -1

//I2C:
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQUENCY 400000

//TCA9548A
#define TCA9548A_ADDR 0x70 
#define SSD1306_CH_1 0
#define SSD1306_CH_2 1
#define SSD1306_CH_3 2
#define UPS_CH       3

//SSD1306
#define SSD1306_ADDR 0x3C
#define MINI_DISPLAY_WIDTH 128
#define MINI_DISPLAY_HEIGHT 64

//Waveshare UPS (INA219)
#define INA219_ADDR 0x40

//Battery
#define BATTERY_MIN_VOLTAGE 9.0
#define BATTERY_MAX_VOLTAGE 12.6

//Encoder:
#define ENCODER_A_PIN 32
#define ENCODER_B_PIN 33
#define ENCODER_BUTTON_PIN 25

//Potentiometer Fader:
#define FADER_1_PIN 34
#define FADER_2_PIN 35
#define FADER_3_PIN 36

//Fader Motor:
#define FADER_MOTOR_1_A_PIN 19
#define FADER_MOTOR_1_B_PIN 4
#define FADER_MOTOR_2_A_PIN 16
#define FADER_MOTOR_2_B_PIN 17 
#define FADER_MOTOR_3_A_PIN 18
#define FADER_MOTOR_3_B_PIN 23

//Taster:
#define MUTE_BUTTON_1_PIN 39
#define MUTE_BUTTON_2_PIN 27
#define MUTE_BUTTON_3_PIN 26
#define RETURN_BUTTON_4_PIN 14

//Taster LED:
#define MUTE_LED_1_PIN 13
#define MUTE_LED_2_PIN 15
#define MUTE_LED_3_PIN 2
#define RETURN_LED_4_PIN 12

#include <Arduino.h>
#include <math.h>
#include <TFT_eSPI.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <EEPROM.h>
#include <ESPUI.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ESP32Encoder.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "settings.h"
#include "wifi_manager.h"
#include "osc_manager.h"
#include "display_manager.h"
#include "webui.h"
#include "network_manager.h"
#include "mixer_state.h"
#include "menu.h"
#include "encoder.h"
#include "fader.h"
#include "led_manager.h"
#include "buttons.h"
#include "batterie_manager.h"
#include "i2c_manager.h"
#include "mini_display_manager.h"



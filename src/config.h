#pragma once

#define HOSTNAME "XAIR-Controller"

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

#define OSC_BUFFER_SIZE 512

#define RefreshXremoteInterval 5000

//EQ-Definitions for display
#define EQ_SAMPLE_RATE     48000

#define EQ_MIN_FREQ        20.0f
#define EQ_MAX_FREQ        20000.0f

#define EQ_DB_RANGE        15.0f

#define EQ_PLOT_WIDTH   (TFT_HEIGHT - 20)
#define EQ_PLOT_HEIGHT  (TFT_WIDTH - 80)

#define EQ_PLOT_X  ((TFT_HEIGHT  - EQ_PLOT_WIDTH)  / 2)
#define EQ_PLOT_Y  ((TFT_WIDTH - EQ_PLOT_HEIGHT) / 2)

#define EQ_POINTS EQ_PLOT_WIDTH

#define EQ_COLOR_BAND1 TFT_CYAN
#define EQ_COLOR_BAND2 TFT_GREEN
#define EQ_COLOR_BAND3 TFT_MAGENTA
#define EQ_COLOR_BAND4 TFT_YELLOW
#define EQ_COLOR_SUM   TFT_ORANGE
#define EQ_COLOR_OFF   TFT_LIGHTGREY

#define EQ_GRID_COLOR TFT_DARKGREY
#define EQ_CURVE_COLOR TFT_GREEN
#define EQ_ZERO_DB_COLOR TFT_LIGHTGREY

#define EQ_GRID_FREQ_LINES 10
#define EQ_GRID_DB_LINES   7



//-----------------
/*
#define ST7789V2_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 280

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   16
#define TFT_RST  17

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY 20000000

#define TFT_RGB_ORDER TFT_RGB

#define TFT_INVERSION_ON
*/
#define BACKLIGHT_PIN 4

//#define TOUCH_CS_PIN -1



//I2C:
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define I2C_FREQUENCY 400000

//MCP23017
#define MCP23017_ADDR 0x20

// MCP23017 Pins
#define MCP_BTN_1 0
#define MCP_BTN_2 1
#define MCP_BTN_3 2
#define MCP_BTN_4 3

#define MCP_LED_1 8
#define MCP_LED_2 9
#define MCP_LED_3 10
#define MCP_LED_4 11

//TCA9548A
#define TCA9548A_ADDR 0x70 
#define SSD1306_CH_1 0
#define SSD1306_CH_2 1
#define SSD1306_CH_3 2
#define UPS_CH       3

//SSD1306
#define SSD1306_ADDR 0x3C
#define MINI_DISPLAY_WIDTH 128
#define MINI_DISPLAY_HEIGHT 32

//Waveshare UPS (INA219)
#define INA219_ADDR 0x41

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
#define FADER_COUNT 3

#define FADER_PWM_FREQ 20000   // 20kHz → kein hörbares Fiepen
#define FADER_PWM_RES  8       // 0-255


/*
#define FADER_MOTOR_1_A_PIN 25
#define FADER_MOTOR_1_B_PIN 26

#define FADER_MOTOR_2_A_PIN 27
#define FADER_MOTOR_2_B_PIN 32

#define FADER_MOTOR_3_A_PIN 33
#define FADER_MOTOR_3_B_PIN 13
*/
#define FADER_MOTOR_1_A_PIN 19
#define FADER_MOTOR_1_B_PIN 2
#define FADER_MOTOR_2_A_PIN -1
#define FADER_MOTOR_2_B_PIN -1 
#define FADER_MOTOR_3_A_PIN -1
#define FADER_MOTOR_3_B_PIN -1

#define FADER1_CH_A 0
#define FADER1_CH_B 1
#define FADER2_CH_A 2
#define FADER2_CH_B 3
#define FADER3_CH_A 4
#define FADER3_CH_B 5

//Taster:
#define MUTE_BUTTON_1_PIN MCP_BTN_1
#define MUTE_BUTTON_2_PIN MCP_BTN_2
#define MUTE_BUTTON_3_PIN MCP_BTN_3
#define RETURN_BUTTON_4_PIN MCP_BTN_4

//Taster LED:
#define MUTE_LED_1_PIN MCP_LED_1
#define MUTE_LED_2_PIN MCP_LED_2
#define MUTE_LED_3_PIN MCP_LED_3
#define RETURN_LED_4_PIN MCP_LED_4

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
#include <Adafruit_MCP23X17.h>
#include <string.h>
#include <stdlib.h>

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
#include "xair_parser.h"
#include "osc_dispatcher.h" 
#include "biquad.h"
#include "eq_plot_engine.h"
#include "eq_grid.h"
#include "eq_freq_table.h"
#include "xair_sync_manager.h"
#include "mixer_defaults.h"







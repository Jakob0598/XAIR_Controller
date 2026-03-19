#pragma once
#include <config.h>

extern bool tcaAvailable;
extern Adafruit_MCP23X17 mcp;

void i2cBegin();
void i2cScanBus();

void i2cSelectChannel(uint8_t channel);

bool i2cDeviceAvailable(uint8_t addr);
bool isMCPConnected(uint8_t addr);

uint8_t i2cRead8(uint8_t addr, uint8_t reg);
uint16_t i2cRead16(uint8_t addr, uint8_t reg);

void i2cWrite8(uint8_t addr, uint8_t reg, uint8_t value);
void i2cWrite16(uint8_t addr, uint8_t reg, uint16_t value);
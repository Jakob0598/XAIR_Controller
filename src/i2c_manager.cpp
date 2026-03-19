#include "i2c_manager.h"

bool tcaAvailable = false;
Adafruit_MCP23X17 mcp;

bool isMCPConnected(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

void i2cBegin()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);

    DBG2("I2C SDA:", I2C_SDA_PIN);
    DBG2("I2C SCL:", I2C_SCL_PIN);

    i2cScanBus();

    Wire.beginTransmission(TCA9548A_ADDR);

    if(Wire.endTransmission() == 0)
    {
        tcaAvailable = true;
        DBG("TCA9548A detected");
    }
    else
    {
        tcaAvailable = false;
        DBG("TCA9548A not detected");
    }

    DBG("Initializing MCP23017...");
    mcp.begin_I2C(MCP23017_ADDR);
    if (isMCPConnected(MCP23017_ADDR)) {
        DBG("MCP23017 detected");
    } else {
        DBG("MCP23017 not detected");
    }
}

void i2cSelectChannel(uint8_t channel)
{
    if(!tcaAvailable){
        return;
    }

    if(channel > 7)
    {
        DBG2("I2C invalid channel:", channel);
        return;
    }

    Wire.beginTransmission(TCA9548A_ADDR);
    Wire.write(1 << channel);

    uint8_t err = Wire.endTransmission();

    if(err != 0)
    {
        DBG2("TCA channel switch failed:", channel);
        return;
    }
}

bool i2cDeviceAvailable(uint8_t addr)
{
    Wire.beginTransmission(addr);

    bool ok = (Wire.endTransmission() == 0);

    if(ok)
        DBG2("I2C device found:", addr);

    return ok;
}

uint8_t i2cRead8(uint8_t addr, uint8_t reg)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    if(Wire.endTransmission(false) != 0)
    {
        DBG3("I2C read8 failed addr/reg:", addr, reg);
        return 0;
    }

    Wire.requestFrom(addr, (uint8_t)1);

    uint32_t timeout = millis();

    while(Wire.available() < 1)
    {
        if(millis() - timeout > 50)
        {
            DBG2("I2C read8 timeout:", addr);
            return 0;
        }
    }

    DBG2("I2C read8 no data:", addr);

    return 0;
}

uint16_t i2cRead16(uint8_t addr, uint8_t reg)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    if(Wire.endTransmission(false) != 0)
    {
        DBG3("I2C read16 failed addr/reg:", addr, reg);
        return 0;
    }

    Wire.requestFrom(addr, (uint8_t)2);

    uint32_t timeout = millis();

    while(Wire.available() < 2)
    {
        if(millis() - timeout > 50)
        {
            DBG2("I2C read16 timeout:", addr);
            return 0;
        }
    }

    uint16_t value = Wire.read() << 8;
    value |= Wire.read();

    return value;
}

void i2cWrite8(uint8_t addr, uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);

    if(Wire.endTransmission() != 0)
    {
        DBG3("I2C write failed addr/reg:", addr, reg);
    }
}

void i2cWrite16(uint8_t addr, uint8_t reg, uint16_t value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);

    Wire.write((value >> 8) & 0xFF);   // High byte
    Wire.write(value & 0xFF);          // Low byte

    if(Wire.endTransmission() != 0)
    {
        DBG3("I2C write16 failed addr/reg:", addr, reg);
    }
}

void i2cScanBus()
{
    DBG("----- I2C Scan -----");

    uint8_t count = 0;

    for(uint8_t addr = 1; addr < 127; addr++)
    {
        Wire.beginTransmission(addr);

        if(Wire.endTransmission() == 0)
        {
            DBG2("I2C device found: 0x", String(addr, HEX));
            count++;
        }
    }

    if(count == 0)
        DBG("No I2C devices found");

    DBG("--------------------");
}


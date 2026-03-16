#include "i2c_manager.h"

void i2cBegin()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);

    DBG2("I2C SDA:", I2C_SDA_PIN);
    DBG2("I2C SCL:", I2C_SCL_PIN);
}

void i2cSelectChannel(uint8_t channel)
{
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

    if(Wire.available())
        return Wire.read();

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

    if(Wire.available() < 2)
    {
        DBG2("I2C read16 no data:", addr);
        return 0;
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
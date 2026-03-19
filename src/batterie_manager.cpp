#include "batterie_manager.h"

static bool ina219Available = false;

static float batterieVoltage = 0;
static float batterieCurrent = 0;
static float batteriePower   = 0;


static void batterieUpdate()
{
    if(!ina219Available)
        return;

    // Bus Voltage Register
    uint16_t rawBus = i2cRead16(INA219_ADDR, 0x02);

    // Current Register
    int16_t rawCurrent = (int16_t)i2cRead16(INA219_ADDR, 0x04);

    // laut Datenblatt
    batterieVoltage = (rawBus >> 3) * 0.004f;

    // Calibration 32V 2A -> 100uA per bit
    batterieCurrent = rawCurrent * 0.0001f;

    batteriePower = batterieVoltage * batterieCurrent;
}


void batterieBegin()
{
    DBG("Batterie manager init");

    if(!i2cDeviceAvailable(INA219_ADDR))
    {
        DBG("INA219 not detected!");
        ina219Available = false;
        return;
    }

    DBG("INA219 detected");

    ina219Available = true;

    // Calibration Register
    i2cWrite16(INA219_ADDR, 0x05, 4096);

    // Config Register
    uint16_t config =
        0x2000 | // 32V range
        0x1800 | // Gain 8
        0x0180 | // Bus ADC 12bit
        0x0078 | // Shunt ADC 12bit 128 samples
        0x0007;  // continuous mode

    i2cWrite16(INA219_ADDR, 0x00, config);

    DBG("INA219 initialized");

    batterieGetVoltage();
    batterieGetCurrent();
    batterieGetPower();
    batterieGetPercentage();
}


float batterieGetVoltage()
{
    batterieUpdate();
    DBG2("Battery voltage:", batterieVoltage);
    return batterieVoltage;
}

float batterieGetCurrent()
{
    batterieUpdate();
    DBG2("Battery current:", batterieCurrent);
    return batterieCurrent;
}

float batterieGetPower()
{
    batterieUpdate();
    DBG2("Battery power:", batteriePower);
    return batteriePower;
}

int batterieGetPercentage()
{
    if(!ina219Available)
        return -1;

    batterieUpdate();

    float percent =
        (batterieVoltage - BATTERY_MIN_VOLTAGE) /
        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0f;

    percent = constrain(percent, 0, 100);

    DBG2("Battery percent:", percent);

    return (int)percent;
}
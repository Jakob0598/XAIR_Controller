#include "batterie_manager.h"
#include "i2c_manager.h"

static float batterieVoltage = 0;
static float batterieCurrent = 0;

static void batterieUpdate()
{
    i2cSelectChannel(UPS_CH);

    uint16_t rawBus = i2cRead16(INA219_ADDR, 0x02);
    uint16_t rawCurrent = i2cRead16(INA219_ADDR, 0x04);

    batterieVoltage = (rawBus >> 3) * 0.004;
    batterieCurrent = rawCurrent * 0.001;
}

void batterieBegin()
{
    i2cSelectChannel(UPS_CH);

    if(!i2cDeviceAvailable(INA219_ADDR))
    {
        DBG("INA219 not detected!");
        return;
    }

    DBG("INA219 detected");

    batterieUpdate();

    DBG2("Battery voltage:", batterieVoltage);
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

    float power = batterieVoltage * batterieCurrent;

    DBG2("Battery power:", power);

    return power;
}

int batterieGetPercentage()
{
    batterieUpdate();

    float percent =
        (batterieVoltage - BATTERY_MIN_VOLTAGE) /
        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    percent = constrain(percent, 0, 100);

    DBG2("Battery percent:", percent);

    return (int)percent;
}
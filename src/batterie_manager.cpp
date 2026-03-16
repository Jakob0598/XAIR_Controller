#include "batterie_manager.h"
#include "i2c_manager.h"

static bool ina219Available = false;

static float batterieVoltage = 0;
static float batterieCurrent = 0;

static void batterieUpdate()
{
    if(!ina219Available){
        return;
    }
    i2cSelectChannel(UPS_CH);

    uint16_t rawBus = i2cRead16(INA219_ADDR, 0x02);
    uint16_t rawCurrent = i2cRead16(INA219_ADDR, 0x04);

    batterieVoltage = (rawBus >> 3) * 0.004;
    batterieCurrent = rawCurrent * 0.001;
}

void batterieBegin()
{
    DBG("Batterie manager init");

    i2cSelectChannel(UPS_CH);

    if(!i2cDeviceAvailable(INA219_ADDR))
    {
        DBG("INA219 not detected!");
        ina219Available = false;
        return;
    }

    DBG("INA219 detected");

    ina219Available = true;
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
    if(!ina219Available){
        return -1;
    }

    batterieUpdate();

    float percent =
        (batterieVoltage - BATTERY_MIN_VOLTAGE) /
        (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE) * 100.0;

    percent = constrain(percent, 0, 100);

    DBG2("Battery percent:", percent);

    return (int)percent;
}
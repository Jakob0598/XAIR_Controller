#include "network_manager.h"

static bool oscActive = false;

void networkBegin()
{
    oscActive = false;
}

void networkLoop()
{
    if (wifiConnected())
    {
        if (!oscActive)
        {
            oscReconnect();
            oscActive = true;
        }
    }
    else
    {
        oscActive = false;
    }
}
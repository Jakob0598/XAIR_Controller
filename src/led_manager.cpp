#include "led_manager.h"

uint8_t ledPins[4] =
{
    MUTE_LED_1_PIN,
    MUTE_LED_2_PIN,
    MUTE_LED_3_PIN,
    RETURN_LED_4_PIN
};

bool ledState[4];



void ledBegin()
{
    for (int i = 0; i < 4; i++)
    {
        mcp.pinMode(ledPins[i], OUTPUT);
        mcp.digitalWrite(ledPins[i], LOW);
        ledState[i] = false;
    }
}



void ledSet(uint8_t index, bool state)
{
    if (index >= 4) return;

    ledState[index] = state;
    mcp.digitalWrite(ledPins[index], state);
}



void ledToggle(uint8_t index)
{
    if (index >= 4) return;

    ledState[index] = !ledState[index];
    mcp.digitalWrite(ledPins[index], ledState[index]);
}



void ledAllOff()
{
    for (int i = 0; i < 4; i++)
        ledSet(i, false);
}



void ledAllOn()
{
    for (int i = 0; i < 4; i++)
        ledSet(i, true);
}
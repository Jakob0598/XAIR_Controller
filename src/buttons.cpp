#include "buttons.h"

static const uint8_t buttonPins[BUTTON_COUNT] =
{
    MUTE_BUTTON_1_PIN,
    MUTE_BUTTON_2_PIN,
    MUTE_BUTTON_3_PIN,
    ENCODER_BUTTON_PIN,
    RETURN_BUTTON_4_PIN
};

bool buttonState[BUTTON_COUNT];
bool lastButtonState[BUTTON_COUNT];

bool pressedEvent[BUTTON_COUNT];
bool releasedEvent[BUTTON_COUNT];

unsigned long lastDebounce[BUTTON_COUNT];

constexpr uint16_t debounceTime = 25;



void buttonsBegin()
{
    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        pinMode(buttonPins[i], INPUT_PULLUP);

        buttonState[i] = false;
        lastButtonState[i] = false;

        pressedEvent[i] = false;
        releasedEvent[i] = false;

        lastDebounce[i] = 0;
        DBG2("Button pin:", buttonPins[i]);
    }
}



void buttonsUpdate()
{
    unsigned long now = millis();

    for(int i = 0; i < BUTTON_COUNT; i++)
    {
        bool reading = !digitalRead(buttonPins[i]);

        if(reading != lastButtonState[i])
        {
            lastDebounce[i] = now;
        }

        if((now - lastDebounce[i]) > debounceTime)
        {
            if(reading != buttonState[i])
            {
                buttonState[i] = reading;

                if(buttonState[i])
                {
                    pressedEvent[i] = true;
                    DBG2("Button pressed:", i);
                }
                else
                {
                    releasedEvent[i] = true;
                    DBG2("Button released:", i);
                }
            }
        }

        lastButtonState[i] = reading;
    }
}



bool buttonPressed(ButtonID id)
{
    if(pressedEvent[id])
    {
        pressedEvent[id] = false;
        return true;
    }
    return false;
}



bool buttonReleased(ButtonID id)
{
    if(releasedEvent[id])
    {
        releasedEvent[id] = false;
        return true;
    }
    return false;
}



bool buttonHeld(ButtonID id)
{
    return buttonState[id];
}



void buttonsLoop()
{
    buttonsUpdate();
}
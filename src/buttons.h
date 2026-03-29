#pragma once
#include "config.h"

#define BUTTON_COUNT 5

enum ButtonID
{
    BUTTON_1 = 0,
    BUTTON_2,
    BUTTON_3,
    BUTTON_RETURN,
    BUTTON_ENCODER
};

void buttonsBegin();
void buttonsLoop();

bool buttonPressed(ButtonID id);
bool buttonReleased(ButtonID id);
bool buttonHeld(ButtonID id);
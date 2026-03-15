#pragma once
#include <config.h>

void batterieBegin();

float batterieGetVoltage();
float batterieGetCurrent();
float batterieGetPower();

int batterieGetPercentage();
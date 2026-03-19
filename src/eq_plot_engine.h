#pragma once

#include "config.h"

void eqPlotRender(int channel, float* buffer, int points);
void eqPlotRenderSingleBand(int ch, int bandIndex, float* buffer, int points);
void eqPlotRenderHPF(int ch, float* buffer, int points);
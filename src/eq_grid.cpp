#include "eq_grid.h"

extern TFT_eSPI tft;

const float eqGridFreqs[] =
{
    20,
    50,
    100,
    200,
    500,
    1000,
    2000,
    5000,
    10000,
    20000
};

const int eqGridFreqCount = sizeof(eqGridFreqs)/sizeof(float);

static int freqToX(float freq)
{
    for (int i = 0; i < EQ_POINTS; i++)
    {
        if (eqFreqTable[i] >= freq)
            return EQ_PLOT_X + i;
    }

    return EQ_PLOT_X + EQ_POINTS - 1;
}

void eqGridDraw()
{
    int x0 = EQ_PLOT_X;
    int y0 = EQ_PLOT_Y;

    int w = EQ_PLOT_WIDTH;
    int h = EQ_PLOT_HEIGHT;

    /* horizontal dB lines */

    for (int i = 0; i < EQ_GRID_DB_LINES; i++)
    {
        float r = (float)i / (EQ_GRID_DB_LINES - 1);

        int y = y0 + r * h;

        tft.drawFastHLine(x0, y, w, EQ_GRID_COLOR);
    }

    /* 0 dB line */

    int yZero = y0 + h / 2;

    tft.drawFastHLine(x0, yZero, w, EQ_ZERO_DB_COLOR);


    /* vertical frequency lines */

    for (int i = 0; i < eqGridFreqCount; i++)
    {
        int x = freqToX(eqGridFreqs[i]);

        tft.drawFastVLine(x, y0, h, EQ_GRID_COLOR);
    }
}
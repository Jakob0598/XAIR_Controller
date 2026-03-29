#include "eq_grid.h"

extern TFT_eSPI tft;

const float eqGridFreqs[] =
{
    20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000
};

const int eqGridFreqCount = sizeof(eqGridFreqs)/sizeof(float);

struct FreqLabel { float freq; const char* label; };
static const FreqLabel freqLabels[] = {
    { 100,   "100"  },
    { 1000,  "1k"   },
    { 10000, "10k"  },
};
static const int freqLabelCount = 3;

struct DbLabel { float ratio; const char* label; };
static const DbLabel dbLabels[] = {
    { 0.0f / 6.0f, "+15" },
    { 1.0f / 6.0f, "+10" },
    { 2.0f / 6.0f, "+5"  },
    { 3.0f / 6.0f, "0"   },
    { 4.0f / 6.0f, "-5"  },
    { 5.0f / 6.0f, "-10" },
    { 6.0f / 6.0f, "-15" },
};
static const int dbLabelCount = 7;

int freqToX(float freq)
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
    int w  = EQ_PLOT_WIDTH;
    int h  = EQ_PLOT_HEIGHT;

    uint16_t gridCol  = tft.color565(25, 25, 25);
    uint16_t labelCol = tft.color565(50, 50, 50);

    tft.setTextSize(1);

    for (int i = 0; i < EQ_GRID_DB_LINES; i++)
    {
        float r = (float)i / (EQ_GRID_DB_LINES - 1);
        int y = y0 + (int)(r * h);
        tft.drawFastHLine(x0, y, w, gridCol);
    }

    int yZero = y0 + h / 2;
    tft.drawFastHLine(x0, yZero, w, tft.color565(45, 45, 45));

    for (int i = 0; i < eqGridFreqCount; i++)
    {
        int x = freqToX(eqGridFreqs[i]);
        tft.drawFastVLine(x, y0, h, gridCol);
    }

    tft.setTextColor(labelCol, TFT_BLACK);
    for (int i = 0; i < dbLabelCount; i++)
    {
        int y = y0 + (int)(dbLabels[i].ratio * h);
        int tw = strlen(dbLabels[i].label) * 6;
        tft.setCursor(x0 - tw - 2, y - 3);
        tft.print(dbLabels[i].label);
    }

    for (int i = 0; i < freqLabelCount; i++)
    {
        int x = freqToX(freqLabels[i].freq);
        int tw = strlen(freqLabels[i].label) * 6;
        tft.setCursor(x - tw / 2, y0 + h + 2);
        tft.setTextColor(labelCol, TFT_BLACK);
        tft.print(freqLabels[i].label);
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

#include "display_manager.h"

static TFT_eSPI tft = TFT_eSPI();

static bool displayReady = false;

static float eqCurve[EQ_POINTS];



void displayBacklight(bool state)
{
    if(state)
        digitalWrite(4, HIGH);
    else
        digitalWrite(4, LOW);
}



void displayClear()
{
    if(!displayReady) return;

    tft.fillScreen(TFT_BLACK);
}



void displayPrint(int x, int y, String text)
{
    if(!displayReady) return;

    tft.setCursor(x, y);
    tft.print(text);
}



void displayBegin()
{
    pinMode(4, OUTPUT);
    displayBacklight(true);

    tft.init();
    tft.setRotation(1);

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    displayReady = true;

    DBG("Initializing EQ frequency table...");
    eqFreqTableInit();
    DBG("EQ frequency table initialized");

    DBG("Display ready");

    tft.setCursor(10,10);
    tft.println("XAIR Controller");
}



void displayLoop()
{
    if(!displayReady) return;

    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 200)
        return;

    lastUpdate = millis();

    // Beispielstatusanzeige
    int bat = batterieGetPercentage();

    tft.fillRect(0,200,320,40,TFT_BLACK);

    tft.setCursor(10,210);
    tft.print("Battery: ");
    tft.print(bat);
    tft.print("%");
}

void displayDrawEq(int ch)
{
    eqGridDraw();
    eqPlotRender(ch, eqCurve, EQ_POINTS);

    int x0 = EQ_PLOT_X;
    int y0 = EQ_PLOT_Y + EQ_PLOT_HEIGHT / 2;

    int lastX = x0;
    int lastY = y0;

    for (int i = 1; i < EQ_POINTS; i++)
    {
        float db = eqCurve[i];

        if (db > EQ_DB_RANGE) db = EQ_DB_RANGE;
        if (db < -EQ_DB_RANGE) db = -EQ_DB_RANGE;

        int x = x0 + i;

        int y = y0 - (db / EQ_DB_RANGE) * (EQ_PLOT_HEIGHT / 2);

        tft.drawLine(lastX, lastY, x, y, TFT_GREEN);

        lastX = x;
        lastY = y;
    }
}
#include "display_manager.h"

TFT_eSPI tft = TFT_eSPI();

static bool displayReady = false;

static float eqCurve[EQ_POINTS];



void displayBacklight(bool state)
{
    if(state)
        digitalWrite(BACKLIGHT_PIN, HIGH);
    else
        digitalWrite(BACKLIGHT_PIN, LOW);
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
    pinMode(BACKLIGHT_PIN, OUTPUT);
    displayBacklight(true);

    tft.init();
    tft.setRotation(1);
    tft.setViewport(0, 0, tft.width(), tft.height());

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    displayReady = true;

    DBG("Initializing EQ frequency table...");
    eqFreqTableInit();
    DBG("EQ frequency table initialized");

    DBG("Display ready");

    tft.setCursor(50,50);
    tft.println("XAIR Controller");
}



void displayLoop()
{
    if(!displayReady) return;

    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 200)
        return;

    lastUpdate = millis();

}

void displayDrawEq(int ch)
{
    static uint32_t last = 0;
    if (millis() - last < 100) return;
    last = millis();

    ChannelState& c = channels[ch];

    int x0 = EQ_PLOT_X;
    int y0 = EQ_PLOT_Y;
    int w = EQ_PLOT_WIDTH;
    int h = EQ_PLOT_HEIGHT;
    int midY = y0 + h / 2;

    tft.fillRect(x0, y0, w, h, TFT_BLACK);

    eqGridDraw();

    float buffer[EQ_POINTS];

    // =========================
    // EINZELNE BÄNDER
    // =========================
    uint16_t colors[5];
    if(c.eqOn){
        colors[1] = EQ_COLOR_BAND1;
        colors[2] = EQ_COLOR_BAND2;
        colors[3] = EQ_COLOR_BAND3;
        colors[4] = EQ_COLOR_BAND4;}
    else{
        colors[1] = EQ_COLOR_OFF;
        colors[2] = EQ_COLOR_OFF;
        colors[3] = EQ_COLOR_OFF;
        colors[4] = EQ_COLOR_OFF;
    }

    for (int b = 1; b <= 4; b++)
    {
        if(c.eq[b].gain != 0.5f){
            eqPlotRenderSingleBand(ch, b, buffer, EQ_POINTS);

            int lastX = x0;
            int lastY = midY;

            for (int i = 0; i < EQ_POINTS; i++)
            {
                float db = buffer[i];

                if (db > EQ_DB_RANGE) db = EQ_DB_RANGE;
                if (db < -EQ_DB_RANGE) db = -EQ_DB_RANGE;

                float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);

                int x = x0 + i;
                int y = y0 + (1.0f - norm) * h;
                
                drawDashedLine(lastX, lastY, x, y, colors[b], 3, 3);

                lastX = x;
                lastY = y;
            }
        }
    }

    // =========================
    // GESAMTKURVE
    // =========================
        eqPlotRender(ch, buffer, EQ_POINTS);

        int lastX = x0;
        int lastY = midY;

        uint16_t sumColor = c.eqOn ? EQ_COLOR_SUM : EQ_COLOR_OFF;

        for (int i = 0; i < EQ_POINTS; i++)
        {
            float db = buffer[i];

            if (db > EQ_DB_RANGE) db = EQ_DB_RANGE;
            if (db < -EQ_DB_RANGE) db = -EQ_DB_RANGE;

            float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);

            int x = x0 + i;
            int y = y0 + (1.0f - norm) * h;

            tft.drawLine(lastX, lastY, x, y, sumColor);

            lastX = x;
            lastY = y;
        }

    if(!c.eqOn && c.hpfOn)
    {
       float buffer[EQ_POINTS];

        eqPlotRenderHPF(ch, buffer, EQ_POINTS);

        int lastX = x0;
        int lastY = midY;

        for (int i = 0; i < EQ_POINTS; i++)
        {
            float db = buffer[i];

            if (db > EQ_DB_RANGE) db = EQ_DB_RANGE;
            if (db < -EQ_DB_RANGE) db = -EQ_DB_RANGE;

            float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);

            int x = x0 + i;
            int y = y0 + (1.0f - norm) * h;

            tft.drawLine(lastX, lastY, x, y, EQ_COLOR_SUM);

            lastX = x;
            lastY = y;
        } 
    }

    // 0 dB Linie
    tft.drawFastHLine(x0, midY, w, TFT_LIGHTGREY);
}

void displayStartupAnimation()
{
    if(!displayReady) return;
    DBG("Startup animation start");

    tft.fillScreen(TFT_BLACK);

    int w = tft.width();
    int h = tft.height();

    // animierte Scanlines
    for(int y=0; y<h; y+=4)
    {
        tft.drawFastHLine(0, y, w, TFT_DARKGREY);
        delay(2);
    }

    delay(200);

    // Titel
    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setTextSize(3);

    tft.setCursor(40, 60);
    tft.print("XAIR");

    tft.setCursor(40, 100);
    tft.print("Controller");

    delay(400);

    // EQ Linien Animation
    int centerY = h/2 + 40;

    for(int i=0;i<w;i+=6)
    {
        int y = centerY + sin(i * 0.05) * 15;

        tft.drawLine(i, centerY, i, y, TFT_BLUE);

        delay(5);
    }

    delay(200);

    // Progress Bar
    int barW = w - 40;
    int barX = 20;
    int barY = h - 40;

    tft.drawRect(barX, barY, barW, 12, TFT_WHITE);

    for(int i=0;i<barW;i+=4)
    {
        tft.fillRect(barX+1, barY+1, i, 10, TFT_BLUE);
        delay(8);
    }

    delay(300);

    tft.fillScreen(TFT_BLACK);
}

void drawDashedLine(int x0, int y0, int x1, int y1, uint16_t color, int dashLength, int gapLength)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int steps = max(abs(dx), abs(dy));

    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;

    float x = x0;
    float y = y0;

    int counter = 0;
    bool draw = true;

    for (int i = 0; i <= steps; i++)
    {
        if (draw)
        {
            tft.drawPixel((int)x, (int)y, color);
        }

        counter++;

        if (draw && counter >= dashLength)
        {
            draw = false;
            counter = 0;
        }
        else if (!draw && counter >= gapLength)
        {
            draw = true;
            counter = 0;
        }

        x += xInc;
        y += yInc;
    }
}
#include "display_manager.h"
#include "xair_sync_manager.h"

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
    tft.setRotation(3);
    tft.setViewport(0, 0, tft.width(), tft.height());

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);

    displayReady = true;

    DBG("Initializing EQ frequency table...");
    eqFreqTableInit();
    DBG("EQ frequency table initialized");

    DBG("Display ready");

    //tft.setCursor(50,50);
    //tft.println("XAIR Controller");
}



void displayLoop()
{
    if(!displayReady) return;
    // StatusBar und VU-Meter werden von menuLoop() gezeichnet
    // Hier nichts tun um doppeltes Zeichnen/Flickering zu vermeiden
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

    // Nur den Plot-Bereich löschen (nicht Header/Tabs/VU)
    tft.fillRect(x0, y0, w, h, TFT_BLACK);

    eqGridDraw();

    float buffer[EQ_POINTS];

    // Aktive Band-Farben
    uint16_t bandColors[5] = {
        tft.color565(160, 160, 160),  // HPF (grau/weiß)
        EQ_COLOR_BAND1,
        EQ_COLOR_BAND2,
        EQ_COLOR_BAND3,
        EQ_COLOR_BAND4
    };

    // Dimmed Versionen für deaktivierte Bänder
    uint16_t dimColors[5] = {
        tft.color565(40, 40, 40),
        tft.color565(0, 50, 50),
        tft.color565(0, 40, 0),
        tft.color565(40, 0, 40),
        tft.color565(40, 40, 0),
    };

    // =========================
    // HPF KURVE (immer sichtbar, grau wenn aus)
    // =========================
    {
        float hpfBuf[EQ_POINTS];
        eqPlotRenderHPF(ch, hpfBuf, EQ_POINTS);

        uint16_t hpfCol = c.hpfOn ? bandColors[0] : dimColors[0];
        int lastX = x0;
        int lastY = midY;

        for (int i = 0; i < EQ_POINTS; i++)
        {
            float db = constrain(hpfBuf[i], -EQ_DB_RANGE, EQ_DB_RANGE);
            float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);
            int x = x0 + i;
            int y = y0 + (int)((1.0f - norm) * h);

            if (c.hpfOn)
                tft.drawLine(lastX, lastY, x, y, hpfCol);
            else
                drawDashedLine(lastX, lastY, x, y, hpfCol, 2, 3);

            lastX = x;
            lastY = y;
        }
    }

    // =========================
    // EINZELNE EQ BÄNDER (immer sichtbar)
    // =========================
    for (int b = 1; b <= 4; b++)
    {
        bool disabled = bandDisabled[ch][b];
        bool eqOff    = !c.eqOn;

        // Farbe: aktiv = Bandfarbe, disabled/eqOff = gedimmt
        uint16_t col = (disabled || eqOff) ? dimColors[b] : bandColors[b];

        eqPlotRenderSingleBand(ch, b, buffer, EQ_POINTS);

        int lastX = x0;
        int lastY = midY;

        for (int i = 0; i < EQ_POINTS; i++)
        {
            float db = constrain(buffer[i], -EQ_DB_RANGE, EQ_DB_RANGE);
            float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);
            int x = x0 + i;
            int y = y0 + (int)((1.0f - norm) * h);

            if (disabled || eqOff)
                drawDashedLine(lastX, lastY, x, y, col, 2, 3);
            else
                tft.drawLine(lastX, lastY, x, y, col);

            lastX = x;
            lastY = y;
        }
    }

    // =========================
    // GESAMTKURVE (dick, orange/grau)
    // =========================
    eqPlotRender(ch, buffer, EQ_POINTS);

    uint16_t sumColor = c.eqOn ? EQ_COLOR_SUM : EQ_COLOR_OFF;
    // Dezenter Fill unter/über der Summenkurve (X32-Style)
    uint16_t fillColor = c.eqOn ? tft.color565(40, 20, 0) : tft.color565(15, 15, 15);

    int lastX = x0;
    int lastY = midY;

    for (int i = 0; i < EQ_POINTS; i++)
    {
        float db = constrain(buffer[i], -EQ_DB_RANGE, EQ_DB_RANGE);
        float norm = (db + EQ_DB_RANGE) / (2.0f * EQ_DB_RANGE);
        int x = x0 + i;
        int y = y0 + (int)((1.0f - norm) * h);

        // Dezenter Fill zwischen Kurve und 0dB-Linie
        if (c.eqOn || c.hpfOn)
        {
            if (y < midY)
                tft.drawFastVLine(x, y + 1, midY - y - 1, fillColor);
            else if (y > midY)
                tft.drawFastVLine(x, midY + 1, y - midY - 1, fillColor);
        }

        // Summenkurve 3px dick
        tft.drawLine(lastX, lastY,     x, y,     sumColor);
        tft.drawLine(lastX, lastY - 1, x, y - 1, sumColor);
        tft.drawLine(lastX, lastY + 1, x, y + 1, sumColor);

        lastX = x;
        lastY = y;
    }

    // =========================
    // VERTIKALE FREQUENZ-INDIKATOREN
    // =========================
    // Senkrechte gestrichelte Linien bei der Center-Frequenz jedes Bandes
    for (int b = 1; b <= 4; b++)
    {
        float freq = c.eq[b].freq;
        // freq ist 0.0-1.0, skalieren auf Hz
        float logMin = log10f(20.0f);
        float logMax = log10f(20000.0f);
        float hz = powf(10.0f, logMin + freq * (logMax - logMin));
        int fx = freqToX(hz);

        bool disabled = bandDisabled[ch][b] || !c.eqOn;
        uint16_t lineCol = disabled ? dimColors[b] : bandColors[b];

        // Gestrichelte vertikale Linie
        for (int py = y0; py < y0 + h; py += 4)
        {
            tft.drawPixel(fx, py,     lineCol);
            tft.drawPixel(fx, py + 1, lineCol);
        }
    }

    // HPF Frequenz-Linie
    {
        float logMin = log10f(20.0f);
        float logMax = log10f(400.0f);
        float hz = powf(10.0f, logMin + c.hpfFreq * (logMax - logMin));
        int fx = freqToX(hz);

        uint16_t lineCol = c.hpfOn ? bandColors[0] : dimColors[0];
        for (int py = y0; py < y0 + h; py += 4)
        {
            tft.drawPixel(fx, py,     lineCol);
            tft.drawPixel(fx, py + 1, lineCol);
        }
    }

    // 0 dB Linie (über alles)
    tft.drawFastHLine(x0, midY, w, tft.color565(60, 60, 60));
}

void displayStartupAnimation()
{
    if(!displayReady) return;
    DBG("Startup animation start");

    tft.fillScreen(TFT_BLACK);

    int w = tft.width();
    int h = tft.height();
    uint16_t accentBlue = tft.color565(0, 120, 255);

    int barW = w - 80;
    int barX = 40;
    int barY = h - 28;
    int barInner = barW - 2;

    // ========== PHASE 1: Scanlines + Logo ==========

    for(int y = 0; y < h; y += 3)
    {
        tft.drawFastHLine(0, y, w, tft.color565(12, 12, 12));
        if (y % 12 == 0) ledToggle(y / 48 % 4);
        delay(1);
    }
    ledAllOff();
    delay(80);

    // Progress bar Rahmen
    tft.drawRect(barX, barY, barW, 10, tft.color565(35, 35, 35));

    // "XAIR" buchstabenweise + smooth progress 0→15%
    tft.setTextSize(4);
    const char* title = "XAIR";
    int titleX = (w - 4 * 24) / 2;

    for (int i = 0; i < 4; i++)
    {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(titleX + i * 24, 45);
        tft.print(title[i]);

        ledAllOff();
        ledSet(i, true);

        int fill = (int)((float)(i + 1) / 4.0f * 0.15f * barInner);
        tft.fillRect(barX + 1, barY + 1, fill, 8, accentBlue);
        delay(100);
    }
    ledAllOff();

    // "Controller"
    tft.setTextSize(2);
    tft.setTextColor(accentBlue, TFT_BLACK);
    tft.setCursor((w - 10 * 12) / 2, 90);
    tft.print("Controller");

    // Smooth progress 15→25%
    for (int p = 15; p <= 25; p += 2)
    {
        int fill = (int)(p / 100.0f * barInner);
        tft.fillRect(barX + 1, barY + 1, fill, 8, accentBlue);
        delay(20);
    }

    miniDisplayText(0, "XAIR\nController");
    miniDisplayText(1, "System\nBoot...");
    miniDisplayText(2, "v1.0\nInit...");
    delay(250);

    // ========== PHASE 2: Waveform ==========

    int waveY = h / 2 + 25;
    for (int frame = 0; frame < 4; frame++)
    {
        float phase = frame * 1.0f;
        for (int x = 10; x < w - 10; x += 2)
        {
            float t = (float)x / w;
            float envelope = sinf(t * M_PI);
            float amp = sinf(t * 8.0f + phase) * 20.0f * envelope;
            int y1 = waveY + (int)amp;

            uint16_t col = tft.color565(
                (uint8_t)(30 * (1.0f - t)),
                (uint8_t)(100 + 120 * t),
                (uint8_t)(255 * envelope)
            );
            int yTop = min(waveY, y1);
            int yH   = abs(y1 - waveY) + 1;
            tft.drawFastVLine(x, yTop, yH, col);
        }
        delay(50);

        if (frame < 3)
            tft.fillRect(10, waveY - 25, w - 20, 50, TFT_BLACK);

        int pct = 25 + (frame + 1) * 4;
        int fill = (int)(pct / 100.0f * barInner);
        tft.fillRect(barX + 1, barY + 1, fill, 8, accentBlue);
    }

    // LED sweep
    for (int i = 0; i < 4; i++) { ledSet(i, true); delay(50); }
    for (int i = 0; i < 4; i++) { ledSet(i, false); delay(50); }

    // ========== PHASE 3: Calibrating ==========

    // Smooth progress 41→45%
    for (int p = 41; p <= 45; p++)
    {
        int fill = (int)(p / 100.0f * barInner);
        tft.fillRect(barX + 1, barY + 1, fill, 8, accentBlue);
        delay(15);
    }

    tft.setTextSize(1);
    tft.setTextColor(tft.color565(80, 80, 80), TFT_BLACK);
    tft.setCursor((w - 11 * 6) / 2, h - 44);
    tft.print("Calibrating");

    miniDisplayText(0, "Fader\nCalibrate");
    miniDisplayText(1, "Please\nWait...");
    miniDisplayText(2, "");

    ledAllOn();
}

void displayBootComplete()
{
    if(!displayReady) return;

    int w = tft.width();
    int h = tft.height();
    uint16_t accentBlue = tft.color565(0, 120, 255);
    uint16_t brightBlue = tft.color565(40, 160, 255);

    int barW = w - 80;
    int barX = 40;
    int barY = h - 28;
    int barInner = barW - 2;

    // Smooth progress 45→100%
    for (int p = 45; p <= 100; p++)
    {
        int fill = (int)(p / 100.0f * barInner);
        uint16_t col = (p > 95) ? brightBlue : accentBlue;
        tft.fillRect(barX + 1, barY + 1, fill, 8, col);
        delay(8);
    }

    delay(200);

    miniDisplayText(0, "Fader OK");
    miniDisplayText(1, "System\nReady!");
    miniDisplayText(2, "");

    for (int i = 0; i < 3; i++)
    {
        ledAllOn();  delay(60);
        ledAllOff(); delay(60);
    }

    delay(150);

    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < 3; i++)
        miniDisplayClear(i);

    ledAllOff();

    // VU-Hintergrund muss nach Clear neu gezeichnet werden
    displayResetVUBackground();
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

// ======================================================
// VU METER – Durchgehende Spalte am rechten Rand
// ======================================================
// Hintergrund wird nur einmal gezeichnet (bei Init/Seitenwechsel).
// Nur die Balken werden bei Wertänderung aktualisiert.

static bool vuBgDrawn = false;

void displayResetVUBackground()
{
    vuBgDrawn = false;
}

void displayDrawStereoVUMeter(int meterLeft, int meterRight)
{
    if(!displayReady) return;

    // ---- Geometrie ----
    const int VU_COL_W = VU_WIDTH;                    // Gesamtbreite der VU-Spalte
    const int VU_COL_X = tft.width() - VU_COL_W;     // Linker Rand der Spalte
    const int BAR_W    = 7;
    const int GAP      = 2;
    const int BAR_H    = tft.height() - 32;           // Statusbar(20) + 6px oben + 6px unten
    const int Y0       = 26;
    const int XL       = VU_COL_X + 3;
    const int XR       = XL + BAR_W + GAP;

    // ---- Hintergrund nur einmal zeichnen ----
    if (!vuBgDrawn)
    {
        tft.fillRect(VU_COL_X, 20, VU_COL_W, tft.height() - 20, tft.color565(8, 8, 8));

        // Trennlinie links
        tft.drawFastVLine(VU_COL_X, 20, tft.height() - 20, tft.color565(22, 22, 22));

        // Führungsschienen
        tft.fillRect(XL, Y0, BAR_W, BAR_H, tft.color565(14, 14, 14));
        tft.fillRect(XR, Y0, BAR_W, BAR_H, tft.color565(14, 14, 14));

        // dB-Markierungen
        struct { float lin; } marks[] = {
            { 0.010f }, { 0.100f }, { 0.316f }, { 0.501f }, { 0.708f }, { 1.000f },
        };
        for (auto& m : marks)
        {
            int py = Y0 + BAR_H - (int)(m.lin * BAR_H);
            if (py >= Y0 && py < Y0 + BAR_H)
                tft.drawFastHLine(XL + BAR_W, py, GAP, tft.color565(50, 50, 50));
        }

        // L / R Labels
        tft.setTextSize(1);
        tft.setTextColor(tft.color565(60, 60, 60), tft.color565(8, 8, 8));
        tft.setCursor(XL + 1, Y0 + BAR_H + 2);
        tft.print("L");
        tft.setCursor(XR + 1, Y0 + BAR_H + 2);
        tft.print("R");
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        vuBgDrawn = true;
    }

    // ---- Werte ----
    float valL = constrain(meters[meterLeft],  0.0f, 1.0f);
    float valR = constrain(meters[meterRight], 0.0f, 1.0f);

    // Peak-Hold
    static float    peakL = 0, peakR = 0;
    static uint32_t peakTL = 0, peakTR = 0;

    if(valL >= peakL) { peakL = valL; peakTL = millis(); }
    else if(valL < 0.001f) { peakL = 0; }
    else if(millis() - peakTL > 1200) peakL = max(0.0f, peakL - 0.008f);

    if(valR >= peakR) { peakR = valR; peakTR = millis(); }
    else if(valR < 0.001f) { peakR = 0; }
    else if(millis() - peakTR > 1200) peakR = max(0.0f, peakR - 0.008f);

    // Change Detection
    static float lastValL = -1, lastValR = -1;
    static float lastPkL = -1, lastPkR = -1;
    static uint32_t lastVuT = 0;

    bool changed = false;
    if (fabsf(valL - lastValL) > 0.008f || fabsf(valR - lastValR) > 0.008f) changed = true;
    if (fabsf(peakL - lastPkL) > 0.008f || fabsf(peakR - lastPkR) > 0.008f) changed = true;
    if (millis() - lastVuT > 80) changed = true;

    if (!changed) return;

    lastValL = valL; lastValR = valR;
    lastPkL = peakL; lastPkR = peakR;
    lastVuT = millis();

    int filledL = (int)(valL * BAR_H);
    int filledR = (int)(valR * BAR_H);

    uint16_t bgCol = tft.color565(14, 14, 14);

    // ---- Balken-Zeichner (nur Bars, kein Hintergrund-Clear) ----
    auto drawBar = [&](int bx, int filled, float peak)
    {
        for (int i = 0; i < BAR_H; i++)
        {
            int py = Y0 + BAR_H - 1 - i;

            if (i < filled && i % 4 != 3)
            {
                float lvl = (float)i / BAR_H;
                uint16_t col;
                if      (lvl < 0.65f) col = tft.color565(30, 200, 75);
                else if (lvl < 0.85f) col = tft.color565(230, 180, 0);
                else                  col = tft.color565(220, 40, 20);

                tft.drawFastHLine(bx, py, BAR_W, col);
            }
            else
            {
                tft.drawFastHLine(bx, py, BAR_W, bgCol);
            }
        }

        // Helle Spitze
        if (filled > 1)
        {
            float lvlTop = (float)(filled - 1) / BAR_H;
            uint16_t topCol;
            if      (lvlTop < 0.65f) topCol = tft.color565(90, 255, 130);
            else if (lvlTop < 0.85f) topCol = tft.color565(255, 235, 70);
            else                     topCol = tft.color565(255, 110, 70);
            tft.drawFastHLine(bx, Y0 + BAR_H - filled, BAR_W, topCol);
        }

        // Peak-Linie
        int peakPx = (int)(peak * BAR_H);
        if (peakPx > 2)
        {
            int py = Y0 + BAR_H - peakPx;
            tft.drawFastHLine(bx, py, BAR_W, TFT_WHITE);
        }
    };

    drawBar(XL, filledL, peakL);
    drawBar(XR, filledR, peakR);
}

// ======================================================
// BATTERIE-ICON  (26×12px, Segmente, Prozentanzeige)
// ======================================================

void displayDrawBattery(int x, int y)
{
    if(!displayReady) return;

    int percent = batterieGetPercentage();

    const int W  = 22;
    const int H  = 11;
    const int TW = 3;
    const int TH = 5;

    // Rahmen-Farbe nach Ladestand
    uint16_t rimCol = tft.color565(130, 130, 130);
    uint16_t fillCol;

    if     (percent < 0)   fillCol = tft.color565(80, 80, 80);
    else if(percent < 20)  fillCol = tft.color565(220, 35, 35);
    else if(percent < 40)  fillCol = tft.color565(220, 155, 0);
    else                   fillCol = tft.color565(40, 200, 70);

    // Hintergrund
    tft.fillRect(x, y, W + TW, H, TFT_BLACK);

    // Äußerer Rahmen (leicht abgerundet: Ecken abdunkeln)
    tft.drawRect(x, y, W, H, rimCol);
    tft.drawPixel(x,     y,     TFT_BLACK);
    tft.drawPixel(x+W-1, y,     TFT_BLACK);
    tft.drawPixel(x,     y+H-1, TFT_BLACK);
    tft.drawPixel(x+W-1, y+H-1, TFT_BLACK);

    // Innenbereich
    tft.fillRect(x+1, y+1, W-2, H-2, tft.color565(15, 15, 15));

    // Terminal (Pluspol)
    int ty = y + (H - TH) / 2;
    tft.fillRect(x + W, ty, TW, TH, rimCol);

    if(percent >= 0)
    {
        // Füllbalken (3 Segmente, je ~6px)
        const int SEG_COUNT = 4;
        const int SEG_W = (W - 4) / SEG_COUNT - 1;
        const int SEG_H = H - 4;
        int activeSegs  = (percent * SEG_COUNT + 50) / 100;
        activeSegs      = constrain(activeSegs, 0, SEG_COUNT);

        for(int s = 0; s < SEG_COUNT; s++)
        {
            int sx = x + 2 + s * (SEG_W + 1);
            int sy = y + 2;
            uint16_t sc = (s < activeSegs) ? fillCol : tft.color565(30, 30, 30);
            tft.fillRect(sx, sy, SEG_W, SEG_H, sc);
        }

        // Prozentzahl (nur bei ≥2 aktiven Segmenten)
        if(activeSegs >= 2)
        {
            char buf[5];
            sprintf(buf, "%d%%", percent);
            tft.setTextSize(1);
            tft.setTextColor(TFT_WHITE, fillCol);
            int tw = strlen(buf) * 6;
            // Über die aktiven Segmente zentriert
            int tx = x + 2 + (activeSegs * (SEG_W + 1) - tw) / 2;
            tx = constrain(tx, x + 2, x + W - tw - 2);
            tft.setCursor(tx, y + 2);
            tft.print(buf);
        }
    }
    else
    {
        // Kein Sensor: X
        tft.drawLine(x+3, y+2, x+W-4, y+H-3, TFT_RED);
        tft.drawLine(x+W-4, y+2, x+3, y+H-3, TFT_RED);
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ======================================================
// WLAN-ICON  (Smartphone-Stil: 4 Balken aufsteigend)
// ======================================================

void displayDrawWiFi(int x, int y)
{
    if(!displayReady) return;

    static uint32_t animT    = 0;
    static int      animStep = 0;

    bool connected = wifiConnected();
    int  rssi      = WiFi.RSSI();

    int level = 0;
    if(connected)
    {
        if     (rssi > -55) level = 4;
        else if(rssi > -65) level = 3;
        else if(rssi > -75) level = 2;
        else                level = 1;
    }

    if(!connected && millis() - animT > 300)
    {
        animT    = millis();
        animStep = (animStep + 1) % 4;
    }

    // 4 Balken: Breite 3px, Abstand 2px, Höhen 4/6/9/12px
    const int heights[4] = { 4, 6, 9, 12 };
    const int bw  = 3;
    const int gap = 1;
    const int totalW = 4 * bw + 3 * gap;
    int sx = x - totalW / 2;

    for(int b = 0; b < 4; b++)
    {
        int bx = sx + b * (bw + gap);
        int bh = heights[b];
        int by = y - bh + 1;

        uint16_t col;
        if(connected)
        {
            if(b < level)
            {
                // Aktive Balken: Farbverlauf grün→weiß je nach Signalstärke
                float t = (float)b / 3.0f;
                col = tft.color565(
                    (uint8_t)(40  + t * 215),
                    (uint8_t)(200 + t * 55),
                    (uint8_t)(70  + t * 185)
                );
            }
            else
            {
                col = tft.color565(45, 45, 45);
            }
        }
        else
        {
            // Such-Animation: Balken laufen von unten hoch
            col = (b == animStep % 4) ? tft.color565(120, 120, 120)
                                      : tft.color565(35, 35, 35);
        }

        tft.fillRect(bx, by, bw, bh, col);

        // Kleiner Highlight oben (1px heller)
        if(connected && b < level)
            tft.drawFastHLine(bx, by, bw, TFT_WHITE);
    }

    // Basispunkt
    tft.fillRect(sx + (totalW - bw) / 2, y + 1, bw, 2,
                 connected ? TFT_WHITE : tft.color565(60, 60, 60));
}

// ======================================================
// XAIR-ICON  (Mixer-Fader-Symbol, 3 Kanäle)
// ======================================================

void displayDrawXAir(int x, int y)
{
    if(!displayReady) return;

    bool connected = isSystemReady();
    bool syncing   = xairRequestRunning();

    // Farbe
    uint16_t railCol, knobCol, baseCol;
    if(syncing)
    {
        // Cyan pulsierend – wird durch 150ms-Redraw animiert
        static uint32_t pulseT = 0;
        static bool     pulseHi = true;
        if(millis() - pulseT > 300) { pulseT = millis(); pulseHi = !pulseHi; }
        uint8_t br = pulseHi ? 220 : 120;
        knobCol  = tft.color565(0, br, br);
        railCol  = tft.color565(0, 50, 60);
        baseCol  = tft.color565(0, br/2, br/2);
    }
    else if(connected)
    {
        knobCol = tft.color565(40, 210, 80);
        railCol = tft.color565(20, 60, 30);
        baseCol = tft.color565(40, 210, 80);
    }
    else
    {
        knobCol = tft.color565(55, 55, 55);
        railCol = tft.color565(30, 30, 30);
        baseCol = tft.color565(55, 55, 55);
    }

    const int ICO_H = 13;
    const int ICO_W = 15;

    // Hintergrund löschen
    tft.fillRect(x, y, ICO_W, ICO_H, TFT_BLACK);

    // Basislinie
    tft.drawFastHLine(x, y + ICO_H - 1, ICO_W, baseCol);

    // 3 Fader-Kanäle
    const int sliderY[3] = { y + 2, y + 6, y + 1 };  // versch. Positionen
    for(int c = 0; c < 3; c++)
    {
        int cx = x + 1 + c * 5;

        // Schiene
        tft.drawFastVLine(cx + 1, y, ICO_H - 2, railCol);

        // Fader-Knopf 3×3px mit 1px hellerem Highlight
        tft.fillRect(cx, sliderY[c], 3, 3, knobCol);
        tft.drawFastHLine(cx, sliderY[c], 3, TFT_WHITE);  // Glanz oben
    }
}

// ======================================================
// STATUSBAR  (320×20px, oben)
// ======================================================
// Layout:
//  [XAir-Icon 15px] [Mitte: Name/Sync/Suche] [WiFi 20px] [Bat 28px]
//  Trennlinie unten
// ======================================================

void displayDrawStatusBar()
{
    if(!displayReady) return;

    // Track state for change detection
    static bool     lastWifi     = false;
    static bool     lastXair     = false;
    static bool     lastSync     = false;
    static int      lastBat      = -999;
    static char     lastName[32] = "";
    static int      lastMode     = -1;   // 0=nowifi, 1=searching, 2=syncing, 3=connected
    static uint32_t lastAnimT    = 0;

    bool wifi  = wifiConnected();
    bool xair  = isSystemReady();
    bool sync  = xairRequestRunning();
    int  bat   = batterieGetPercentage();

    // Modus bestimmen
    // Reihenfolge wichtig: sync darf nur angezeigt werden wenn auch wirklich verbunden
    int mode = 0;
    if (!wifi)               mode = 0;   // kein WiFi
    else if (!xair && !sync) mode = 1;   // WiFi da, XAir suchen
    else if (xair && sync)   mode = 2;   // verbunden UND sync läuft
    else if (xair)           mode = 3;   // verbunden, kein sync
    else                     mode = 1;   // WiFi da aber xair noch nicht bereit

    // Detect what changed
    bool modeChanged  = (mode != lastMode);
    bool stateChanged = (wifi != lastWifi) || (xair != lastXair) || (sync != lastSync)
                        || (bat != lastBat) || (strcmp(lastName, mixerName) != 0);
    bool animTick     = false;

    // Animation tick (only for modes 1 and 2)
    if ((mode == 1 || mode == 2) && millis() - lastAnimT > 130)
    {
        animTick = true;
        lastAnimT = millis();
    }

    if (!modeChanged && !stateChanged && !animTick) return;

    const int W   = tft.width();
    const int SBW = W - VU_WIDTH;  // Statusbar endet vor VU-Spalte
    const int MID = 10;

    // ---- FULL REDRAW bei Mode-Wechsel oder State-Änderung ----
    if (modeChanged || stateChanged)
    {
        tft.fillRect(0, 0, SBW, 20, TFT_BLACK);

        // Trennlinie mit Gradient
        uint16_t lineCol = xair ? tft.color565(0, 55, 28) : tft.color565(30, 30, 30);
        tft.drawFastHLine(8, 19, SBW - 16, lineCol);
        // Fade-Enden
        tft.drawPixel(6, 19, tft.color565(12, 12, 12));
        tft.drawPixel(7, 19, tft.color565(20, 20, 20));
        tft.drawPixel(SBW - 8, 19, tft.color565(20, 20, 20));
        tft.drawPixel(SBW - 7, 19, tft.color565(12, 12, 12));

        // Icons (nur bei State-Änderung)
        displayDrawBattery(SBW - 36, MID - 5);
        displayDrawWiFi(SBW - 54, MID + 3);
        displayDrawXAir(22, MID - 6);

        // Mitte: statischer Text je nach Mode
        const int MX0 = 42;
        const int MXE = SBW - 60;
        const int MW  = MXE - MX0;

        tft.setTextSize(1);

        if (mode == 3)
        {
            // Connected: Name + Model
            bool hasName  = strlen(mixerName) > 0;
            bool hasModel = strlen(mixerModel) > 0 && strcmp(mixerName, mixerModel) != 0;

            if (hasName && hasModel)
            {
                tft.setTextColor(tft.color565(50, 220, 90), TFT_BLACK);
                int nw = strlen(mixerName) * 6;
                tft.setCursor(constrain(MX0 + (MW - nw) / 2, MX0, MXE - nw), 2);
                tft.print(mixerName);

                tft.setTextColor(tft.color565(0, 120, 45), TFT_BLACK);
                int mw = strlen(mixerModel) * 6;
                tft.setCursor(constrain(MX0 + (MW - mw) / 2, MX0, MXE - mw), 11);
                tft.print(mixerModel);
            }
            else if (hasName)
            {
                tft.setTextColor(tft.color565(50, 220, 90), TFT_BLACK);
                int nw = strlen(mixerName) * 6;
                tft.setCursor(constrain(MX0 + (MW - nw) / 2, MX0, MXE - nw), MID - 4);
                tft.print(mixerName);
            }
            else if (strlen(mixerModel) > 0)
            {
                tft.setTextColor(tft.color565(50, 220, 90), TFT_BLACK);
                int mw = strlen(mixerModel) * 6;
                tft.setCursor(MX0 + (MW - mw) / 2, MID - 4);
                tft.print(mixerModel);
            }
        }
        else if (mode == 2)
        {
            // Syncing: statischer "Syncing" Text (Punkte werden separat animiert)
            uint16_t syncCol = tft.color565(0, 190, 220);
            int syncTextX = MX0 + (MW - 10 * 6) / 2;
            tft.setTextColor(syncCol, TFT_BLACK);
            tft.setCursor(syncTextX, MID - 4);
            tft.print("Syncing");
        }
        else if (mode == 1)
        {
            // Searching: statischer Text
            int searchX = MX0 + (MW - 12 * 6) / 2;
            tft.setTextColor(tft.color565(100, 100, 100), TFT_BLACK);
            tft.setCursor(searchX, MID - 4);
            tft.print("Searching");
        }
        else
        {
            // No WiFi
            tft.setTextColor(tft.color565(60, 60, 60), TFT_BLACK);
            tft.setCursor(MX0 + (MW - 7 * 6) / 2, MID - 4);
            tft.print("No WiFi");
        }

        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    // ---- ANIMATION UPDATE (nur Punkte und Ladebalken) ----
    if (animTick || modeChanged)
    {
        const int MX0 = 42;
        const int MXE = SBW - 60;
        const int MW  = MXE - MX0;

        tft.setTextSize(1);

        if (mode == 2)
        {
            // Sync-Dots: nur den Punkt-Bereich neuzeichnen
            static int syncDot = 0;
            static int barPos  = 0;

            syncDot = (syncDot + 1) % 4;
            barPos  = (barPos + 10) % MW;

            uint16_t syncCol = tft.color565(0, 190, 220);
            int syncTextX = MX0 + (MW - 10 * 6) / 2;
            int dotX = syncTextX + 7 * 6;  // Nach "Syncing"

            // Nur die 3 Punkte (18px breit) löschen und neuzeichnen
            tft.fillRect(dotX, MID - 4, 18, 8, TFT_BLACK);
            tft.setCursor(dotX, MID - 4);
            for (int d = 0; d < 3; d++)
            {
                tft.setTextColor(
                    d < syncDot ? syncCol : tft.color565(30, 30, 30),
                    TFT_BLACK);
                tft.print(".");
            }

            // Ladebalken
            const int barMargin = 20;
            const int BY = 17;
            const int BL = MW - barMargin * 2;
            const int barStartX = MX0 + barMargin;
            int localBarPos = barPos % BL;

            tft.drawFastHLine(barStartX, BY, BL, tft.color565(20, 20, 20));
            const int SW = 24;
            for (int px = 0; px < SW; px++)
            {
                int drawX = barStartX + ((localBarPos + px) % BL);
                float t = fabsf(px - SW / 2.0f) / (SW / 2.0f);
                uint8_t br = (uint8_t)(200 * (1.0f - t * 0.7f));
                tft.drawPixel(drawX, BY, tft.color565(0, br, br));
            }
        }
        else if (mode == 1)
        {
            // Search-Dots
            static int searchDot = 0;
            searchDot = (searchDot + 1) % 4;

            int searchX = MX0 + (MW - 12 * 6) / 2;
            int dotX = searchX + 9 * 6;

            tft.fillRect(dotX, MID - 4, 18, 8, TFT_BLACK);
            tft.setCursor(dotX, MID - 4);
            for (int d = 0; d < 3; d++)
            {
                tft.setTextColor(
                    d < searchDot ? tft.color565(100, 100, 100)
                                  : tft.color565(30, 30, 30),
                    TFT_BLACK);
                tft.print(".");
            }
        }

        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }

    // Cache
    lastWifi = wifi;
    lastXair = xair;
    lastSync = sync;
    lastBat  = bat;
    lastMode = mode;
    strncpy(lastName, mixerName, sizeof(lastName));
}
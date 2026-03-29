#include "mini_display_manager.h"

static Adafruit_SSD1306 display1(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);
static Adafruit_SSD1306 display2(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);
static Adafruit_SSD1306 display3(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);

static void miniDisplayInit(uint8_t channel, Adafruit_SSD1306 &display)
{
    DBG("Mini displays init");

    if(!tcaAvailable)
    {
        DBG("TCA9548A not found - mini displays disabled");
        return;
    }

    i2cSelectChannel(channel);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR))
    {
        DBG2("SSD1306 init failed CH:", channel);
        return;
    }

    DBG2("SSD1306 ready CH:", channel);

    display.clearDisplay();
    display.setRotation(2);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    //display.print("Display:");
    //display.print(channel);
    display.display();
}

void miniDisplayBegin()
{
    DBG("Mini displays init");

    if(!i2cDeviceAvailable(TCA9548A_ADDR))
    {
        DBG("TCA9548A not found - mini displays disabled");
        return;
    }

    miniDisplayInit(SSD1306_CH_1, display1);
    miniDisplayInit(SSD1306_CH_2, display2);
    miniDisplayInit(SSD1306_CH_3, display3);
}

// =====================================================
// TEXT + VU-METER (rechter Rand, vertikal, 8px breit)
// =====================================================
// Layout: [ Text 108px | sep | VU 10px ]
// VU-Balken läuft von unten nach oben (0..31px)
// Segmentierter Look, Peak-Hold pro Display-Slot
// =====================================================

#define MINI_VU_SLOTS 3
static float  vuPeak[MINI_VU_SLOTS]     = {0, 0, 0};
static uint32_t vuPeakTime[MINI_VU_SLOTS] = {0, 0, 0};

void miniDisplayTextWithVU(uint8_t displayIndex, String line1, String line2, float vuLevel)
{
    if(!tcaAvailable) return;
    if(displayIndex >= MINI_VU_SLOTS) return;

    Adafruit_SSD1306* display;
    uint8_t ch;

    if(displayIndex == 0)      { display = &display1; ch = SSD1306_CH_1; }
    else if(displayIndex == 1) { display = &display2; ch = SSD1306_CH_2; }
    else                       { display = &display3; ch = SSD1306_CH_3; }

    i2cSelectChannel(ch);
    display->clearDisplay();

    // ----- VU-Geometrie -----
    const int VU_W  = 8;
    const int VU_H  = MINI_DISPLAY_HEIGHT;
    const int VU_X  = MINI_DISPLAY_WIDTH - VU_W - 1;
    const int TEXT_W = VU_X - 3;  // Platz für Text

    // ----- Text (linker Bereich) -----
    // Zeile 1: Label klein (Size 1)
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->print(line1);

    // Zeile 2: Wert größer (Size 2) für bessere Lesbarkeit
    if (line2.length() > 0)
    {
        // Wenn Wert kurz genug für Size 2, nutze das
        int textW = line2.length() * 12;  // Size 2 = 12px pro Zeichen
        if (textW <= TEXT_W)
        {
            display->setTextSize(2);
            display->setCursor(0, 14);
        }
        else
        {
            // Fallback auf Size 1
            display->setTextSize(1);
            display->setCursor(0, 16);
        }
        display->print(line2);
    }

    // ----- Trennlinie (gestrichelt für schickeren Look) -----
    for (int y = 0; y < VU_H; y += 3)
        display->drawPixel(VU_X - 2, y, SSD1306_WHITE);

    // ----- Peak-Hold aktualisieren -----
    vuLevel = constrain(vuLevel, 0.0f, 1.0f);

    if(vuLevel >= vuPeak[displayIndex])
    {
        vuPeak[displayIndex]     = vuLevel;
        vuPeakTime[displayIndex] = millis();
    }
    else if(vuLevel < 0.001f)
    {
        // Instant reset on disconnect
        vuPeak[displayIndex] = 0;
    }
    else if(millis() - vuPeakTime[displayIndex] > 1200)
    {
        vuPeak[displayIndex] -= 0.03f;
        if(vuPeak[displayIndex] < 0.0f) vuPeak[displayIndex] = 0.0f;
    }

    // ----- Balken zeichnen -----
    int filled   = (int)(vuLevel              * VU_H);
    int peakPx   = (int)(vuPeak[displayIndex] * VU_H);

    for(int i = 0; i < filled; i++)
    {
        if(i % 3 == 2) continue;   // Lücke → Segment-Look
        int py = VU_H - 1 - i;
        display->drawFastHLine(VU_X, py, VU_W, SSD1306_WHITE);
    }

    // Peak-Linie
    if(peakPx > 1)
    {
        int peakY = VU_H - peakPx;
        if(peakY >= 0 && peakY < VU_H)
            display->drawFastHLine(VU_X, peakY, VU_W, SSD1306_WHITE);
    }

    // Rahmen (dünn, subtil)
    display->drawRect(VU_X, 0, VU_W, VU_H, SSD1306_WHITE);

    display->display();
}

// Legacy: reiner Text ohne VU (intern auf TextWithVU umgelenkt mit Level 0)
void miniDisplayText(uint8_t displayIndex, String text)
{
    if(!tcaAvailable)
    {
        DBG("TCA9548A not found - mini displays disabled");
        return;
    }

    Adafruit_SSD1306* display;
    uint8_t channel =
        (displayIndex == 0) ? SSD1306_CH_1 :
        (displayIndex == 1) ? SSD1306_CH_2 :
                              SSD1306_CH_3;

    if(displayIndex == 0) display = &display1;
    else if(displayIndex == 1) display = &display2;
    else display = &display3;

    i2cSelectChannel(channel);

    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);

    int split = text.indexOf('\n');

    if(split >= 0)
    {
        display->print(text.substring(0, split));
        display->setCursor(0, 16);
        display->print(text.substring(split + 1));
    }
    else
    {
        display->print(text);
    }

    display->display();
}

// =====================================================
// Display komplett ausschalten (leer, schwarz)
// =====================================================
void miniDisplayClear(uint8_t displayIndex)
{
    if(!tcaAvailable) return;

    Adafruit_SSD1306* display;
    uint8_t channel =
        (displayIndex == 0) ? SSD1306_CH_1 :
        (displayIndex == 1) ? SSD1306_CH_2 :
                              SSD1306_CH_3;

    if(displayIndex == 0) display = &display1;
    else if(displayIndex == 1) display = &display2;
    else display = &display3;

    i2cSelectChannel(channel);
    display->clearDisplay();
    display->display();
}

// =====================================================
// Parameter-Anzeige ohne VU – gleicher Style wie TextWithVU
// Label oben (Size 1), Wert groß (Size 2), kein VU-Balken
// =====================================================
void miniDisplayParam(uint8_t displayIndex, const char* label, const char* value, const char* unit)
{
    if(!tcaAvailable) return;

    Adafruit_SSD1306* display;
    uint8_t channel =
        (displayIndex == 0) ? SSD1306_CH_1 :
        (displayIndex == 1) ? SSD1306_CH_2 :
                              SSD1306_CH_3;

    if(displayIndex == 0) display = &display1;
    else if(displayIndex == 1) display = &display2;
    else display = &display3;

    i2cSelectChannel(channel);
    display->clearDisplay();

    // Zeile 1: Label (Size 1) – gleich wie TextWithVU
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->print(label);

    // Zeile 2: Wert groß (Size 2) – gleich wie TextWithVU
    String valStr = String(value);
    if (unit && strlen(unit) > 0)
        valStr += String(unit);

    int textW = valStr.length() * 12;
    if (textW <= MINI_DISPLAY_WIDTH)
    {
        display->setTextSize(2);
        display->setCursor(0, 14);
    }
    else
    {
        display->setTextSize(1);
        display->setCursor(0, 16);
    }
    display->print(valStr);

    display->display();
}

void miniDisplayLoop()
{
    // wird von menu.cpp / updateMiniDisplays() gesteuert
}

void miniDisplayStartupAnimation()
{
    if(!tcaAvailable) return;

    DBG("Mini display startup animation");

    Adafruit_SSD1306* displays[3] = { &display1, &display2, &display3 };
    uint8_t channels[3] = { SSD1306_CH_1, SSD1306_CH_2, SSD1306_CH_3 };

    // 1. Clear + Titel
    for(int i=0;i<3;i++)
    {
        i2cSelectChannel(channels[i]);

        displays[i]->clearDisplay();
        displays[i]->setTextSize(1);
        displays[i]->setCursor(0,0);
        displays[i]->print("CH ");
        displays[i]->print(i+1);
        displays[i]->display();
    }

    delay(200);

    // 2. Scan Animation
    for(int x=0;x<MINI_DISPLAY_WIDTH;x+=4)
    {
        for(int i=0;i<3;i++)
        {
            i2cSelectChannel(channels[i]);

            displays[i]->drawLine(x, 10, x, 30, SSD1306_WHITE);
            displays[i]->display();
        }
        delay(5);
    }

    delay(200);

    // 3. Level Meter Animation
    for(int level=0; level<MINI_DISPLAY_WIDTH; level+=6)
    {
        for(int i=0;i<3;i++)
        {
            i2cSelectChannel(channels[i]);

            displays[i]->fillRect(0, 20, level, 8, SSD1306_WHITE);
            displays[i]->display();
        }
        delay(10);
    }

    delay(200);

    // 4. READY Text
    for(int i=0;i<3;i++)
    {
        i2cSelectChannel(channels[i]);

        displays[i]->clearDisplay();
        displays[i]->setCursor(0,10);
        displays[i]->setTextSize(2);
        displays[i]->print("OK");
        displays[i]->display();
    }

    delay(400);

    // 5. Clean Screen
    for(int i=0;i<3;i++)
    {
        i2cSelectChannel(channels[i]);

        displays[i]->clearDisplay();
        displays[i]->display();
    }

    DBG("Mini display animation done");
}
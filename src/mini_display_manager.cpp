#include "mini_display_manager.h"

static Adafruit_SSD1306 display1(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);
static Adafruit_SSD1306 display2(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);
static Adafruit_SSD1306 display3(MINI_DISPLAY_WIDTH, MINI_DISPLAY_HEIGHT, &Wire);

static void miniDisplayInit(uint8_t channel, Adafruit_SSD1306 &display)
{
    i2cSelectChannel(channel);

    if(!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR))
    {
        DBG2("SSD1306 init failed CH:", channel);
        return;
    }

    DBG2("SSD1306 ready CH:", channel);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
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

void miniDisplayText(uint8_t displayIndex, String text)
{
    Adafruit_SSD1306* display;

    if(displayIndex == 0) display = &display1;
    else if(displayIndex == 1) display = &display2;
    else display = &display3;

    uint8_t channel =
        (displayIndex == 0) ? SSD1306_CH_1 :
        (displayIndex == 1) ? SSD1306_CH_2 :
                              SSD1306_CH_3;

    i2cSelectChannel(channel);

    display->clearDisplay();
    display->setCursor(0,0);
    display->print(text);
    display->display();
}

void miniDisplayLoop()
{
    // später UI
}
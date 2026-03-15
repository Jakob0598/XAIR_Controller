#include "config.h"

TFT_eSPI tft = TFT_eSPI();

int freqToX(float f) {

    float minF=log10(20);
    float maxF=log10(20000);

    float logF=log10(f);

    return (logF-minF)/(maxF-minF)*240;
}

int gainToY(float g) {

    return (15-g)/30.0*135;
}

float peq(float f,EQBand &b) {

    float A = pow(10,b.gain/40);
    float w0 = 2*PI*b.freq/48000;
    float alpha = sin(w0)/(2*b.q);

    float num = 1+alpha*A;
    float den = 1+alpha/A;

    return num/den;
}

float computeEQ(float f) {

    float g=1;

    for(int i=0;i<4;i++)
        g*=peq(f,mixer.ch[0].eq[i]);

    return 20*log10(g);
}

void drawEQ() {

    tft.fillScreen(TFT_BLACK);

    int lastx=0;
    int lasty=gainToY(computeEQ(20));

    for(int x=1;x<240;x++) {

        float f=pow(10,log10(20)+((float)x/240)*(log10(20000)-log10(20)));

        int y=gainToY(computeEQ(f));

        tft.drawLine(lastx,lasty,x,y,TFT_GREEN);

        lastx=x;
        lasty=y;
    }

}

void displayBegin()
{
    DBG("Init display");

    tft.init();
    tft.setRotation(1);

    DBG("Display ready");
}

void displayLoop() {

    drawEQ();
}
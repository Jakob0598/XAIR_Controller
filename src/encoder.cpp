#include "encoder.h"

ESP32Encoder encoder;

long lastEncoder = 0;



void encoderBegin()
{
    ESP32Encoder::useInternalWeakPullResistors = puType::up;

    encoder.attachHalfQuad(
        ENCODER_A_PIN,
        ENCODER_B_PIN
    );

    encoder.clearCount();
    DBG2("Encoder pins:", ENCODER_A_PIN);
    DBG2("Encoder pins:", ENCODER_B_PIN);
}



int encoderGetDelta()
{
    long value = encoder.getCount()/2;
    int delta = value - lastEncoder;

    lastEncoder = value;

    // optional begrenzen für Menü
    if(delta > 1) delta = 1;
    if(delta < -1) delta = -1;

    if(delta != 0)
    {
        DBG2("Encoder delta:", delta);
    }

    return delta;
}



void encoderLoop()
{
    int delta = encoderGetDelta();

    if(delta != 0)
    {
        //
    }
}
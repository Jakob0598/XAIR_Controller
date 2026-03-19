#pragma once
#include "config.h"

class MotorFader
{
public:

    void begin(uint8_t adcPin, uint8_t pinA, uint8_t pinB, uint8_t chA, uint8_t chB);

    void update();

    void setTarget(uint16_t value);
    uint16_t getPosition();

private:

    uint8_t _adcPin;
    uint8_t _pinA;
    uint8_t _pinB;
    uint8_t _chA;
    uint8_t _chB;

    float _position = 0;
    float _target = 0;

    // ADC filtering
    float _filtered = 0;

    // PID control
    float kp = 0.35;
    float ki = 0.0;
    float kd = 0.12;

    float integral = 0;
    float lastError = 0;

    // touch detection
    uint16_t lastRaw = 0;
    uint32_t lastMoveTime = 0;
    bool touched = false;

    void motorDrive(int pwm);
};


// global fader control
void faderBegin();
void faderLoop();

void faderSet(uint8_t index, uint16_t value);
uint16_t faderGet(uint8_t index);

// startup animation
void faderStartupAnimation();
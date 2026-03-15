#include "fader.h"

#define MAX_FADERS 3

MotorFader faders[MAX_FADERS];
uint8_t faderCount = 0;



void MotorFader::begin(uint8_t adcPin, uint8_t pinA, uint8_t pinB)
{
    _adcPin = adcPin;
    _pinA = pinA;
    _pinB = pinB;

    pinMode(_pinA, OUTPUT);
    pinMode(_pinB, OUTPUT);

    _filtered = analogRead(_adcPin);
}



uint16_t MotorFader::getPosition()
{
    return (uint16_t)_position;
}



void MotorFader::setTarget(uint16_t value)
{
    _target = value;
}



void MotorFader::motorDrive(int pwm)
{
    pwm = constrain(pwm, -255, 255);

    if (pwm > 0)
    {
        analogWrite(_pinA, pwm);
        analogWrite(_pinB, 0);
    }
    else if (pwm < 0)
    {
        analogWrite(_pinA, 0);
        analogWrite(_pinB, -pwm);
    }
    else
    {
        analogWrite(_pinA, 0);
        analogWrite(_pinB, 0);
    }
}



void MotorFader::update()
{
    uint16_t raw = analogRead(_adcPin);
    // ---------- ADC FILTER ----------
    _filtered = (_filtered * 7 + raw * 3) / 10;
    _position = _filtered;



    // ---------- TOUCH DETECTION ----------
    int movement = abs(raw - lastRaw);
    lastRaw = raw;

    if (movement > 8)
    {
        lastMoveTime = millis();
        touched = true;
    }

    if (millis() - lastMoveTime > 150)
    {
        touched = false;
    }

    if (touched)
    {
        motorDrive(0);
        return;
    }



    // ---------- PID CONTROL ----------
    float error = _target - _position;

    integral += error;
    float derivative = error - lastError;

    float output = kp * error +
                   ki * integral +
                   kd * derivative;

    lastError = error;



    // deadband
    if (abs(error) < 5)
    {
        motorDrive(0);
        return;
    }



    output = constrain(output, -255, 255);

    motorDrive((int)output);
}




void faderBegin()
{
    faderCount = 0;

    faders[faderCount++].begin(
        FADER_1_PIN,
        FADER_MOTOR_1_A_PIN,
        FADER_MOTOR_1_B_PIN
    );

    faders[faderCount++].begin(
        FADER_2_PIN,
        FADER_MOTOR_2_A_PIN,
        FADER_MOTOR_2_B_PIN
    );

    faders[faderCount++].begin(
        FADER_3_PIN,
        FADER_MOTOR_3_A_PIN,
        FADER_MOTOR_3_B_PIN
    );

    DBG2("Faders initialized:", faderCount);
}



void faderLoop()
{
    for (int i = 0; i < faderCount; i++)
    {
        faders[i].update();
    }
}



void faderSet(uint8_t index, uint16_t value)
{
    DBG3("faderSet: ", index, value);

    if (index >= faderCount) return;

    faders[index].setTarget(value);
}



uint16_t faderGet(uint8_t index)
{
    if (index >= faderCount) return 0;

    return faders[index].getPosition();
}
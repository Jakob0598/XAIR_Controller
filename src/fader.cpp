#include "fader.h"

#define FADER_DEADBAND 20
#define MAX_PWM 255

MotorFader faders[FADER_COUNT];

void MotorFader::begin(uint8_t adcPin, uint8_t pinA, uint8_t pinB, uint8_t chA, uint8_t chB)
{
    _adcPin = adcPin;
    _pinA = pinA;
    _pinB = pinB;
    _chA = chA;
    _chB = chB;

    ledcSetup(_chA, FADER_PWM_FREQ, FADER_PWM_RES);
    ledcSetup(_chB, FADER_PWM_FREQ, FADER_PWM_RES);

    ledcAttachPin(_pinA, _chA);
    ledcAttachPin(_pinB, _chB);

    uint16_t raw = analogRead(_adcPin);

    _filtered = raw;
    _position = raw;
    _target = raw;

    lastRaw = raw;
}

void MotorFader::setTarget(uint16_t value)
{
    _target = constrain(value, 0, 4095);
}

uint16_t MotorFader::getPosition()
{
    return _position;
}

void MotorFader::motorDrive(int pwm)
{
    pwm = constrain(pwm, -MAX_PWM, MAX_PWM);

    if (pwm > 0)
    {
        ledcWrite(_chA, pwm);
        ledcWrite(_chB, 0);
    }
    else if (pwm < 0)
    {
        ledcWrite(_chA, 0);
        ledcWrite(_chB, -pwm);
    }
    else
    {
        ledcWrite(_chA, 0);
        ledcWrite(_chB, 0);
    }
}

void MotorFader::update()
{
    uint16_t raw = analogRead(_adcPin);

    // stabiles Filtering
    _filtered = _filtered * 0.6f + raw * 0.4f;
    _position = _filtered;

    float error = _target - _position;
    float absError = abs(error);

    // ✅ große Deadzone (WICHTIG!)
    if (absError < 20)
    {
        motorDrive(0);
        return;
    }

    int pwm = 0;

    // ✅ grobe Bewegung (schnell)
    if (absError > 300)
    {
        pwm = 180;
    }
    // ✅ mittlere Zone
    else if (absError > 100)
    {
        pwm = 120;
    }
    // ✅ fein bewegen
    else if (absError > 40)
    {
        pwm = 80;
    }
    else
    {
        pwm = 50;
    }

    // Richtung setzen
    if (error < 0)
        pwm = -pwm;

    motorDrive(pwm);
}

void faderBegin()
{
    faders[0].begin(FADER_1_PIN, FADER_MOTOR_1_A_PIN, FADER_MOTOR_1_B_PIN, FADER1_CH_A, FADER1_CH_B);
    faders[1].begin(FADER_2_PIN, FADER_MOTOR_2_A_PIN, FADER_MOTOR_2_B_PIN, FADER2_CH_A, FADER2_CH_B);
    faders[2].begin(FADER_3_PIN, FADER_MOTOR_3_A_PIN, FADER_MOTOR_3_B_PIN, FADER3_CH_A, FADER3_CH_B);

    DBG("Faders initialized (LEDC)");

    delay(200);
    faderStartupAnimation();
}

void faderLoop()
{
    for (int i = 0; i < FADER_COUNT; i++)
    {
        faders[i].update();
    }
}

void faderSet(uint8_t index, uint16_t value)
{
    if (index >= FADER_COUNT) return;

    faders[index].setTarget(value);
}

uint16_t faderGet(uint8_t index)
{
    if (index >= FADER_COUNT) return 0;

    return faders[index].getPosition();
}

void faderStartupAnimation()
{
    DBG("Fader startup animation");

    // hoch
    for(int i=0;i<FADER_COUNT;i++)
        faderSet(i,4095);

    uint32_t t = millis();
    while(millis() - t < 1200)
        faderLoop();

    // runter
    for(int i=0;i<FADER_COUNT;i++)
        faderSet(i,0);

    t = millis();
    while(millis() - t < 1200)
        faderLoop();

    // mitte
    for(int i=0;i<FADER_COUNT;i++)
        faderSet(i,2048);

    t = millis();
    while(millis() - t < 800)
        faderLoop();

    DBG("Fader startup done");
}
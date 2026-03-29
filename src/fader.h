#pragma once
#include "config.h"

// ======================================================
// FADER-REGELUNG: "Continuous Drive + Early Stop"
// ======================================================
//
// Einfache, robuste Strategie für 9V Motoren:
//
// 1. KICK:  Kurzer Vollgas-Impuls um Haftreibung zu brechen (30ms)
// 2. DRIVE: Kontinuierlicher Antrieb mit sqrt-Profil
//           Motor AUS bei großer Distanz (STOP_ZONE)
// 3. BRAKE: Harte Bremse (beide Pins HIGH)
// 4. COAST: Motor stromlos, ADC beruhigen, messen
// 5. Wenn nicht nah genug: EIN weiterer Versuch mit kleinerem STOP
// 6. DONE
//
// KEIN Pulse-Creep. KEINE Oszillation. Maximal 2 Annäherungen.

// ---- Akzeptanz ----
#define ACCEPT_ZONE           35.0f    // ~0.85% – unhörbar

// ---- KICK Phase (Haftreibung brechen) ----
#define KICK_PWM              220      // Kurzer voller Impuls
#define KICK_TIME_MS          20       // Reicht um Haftreibung zu brechen

// ---- DRIVE Phase ----
#define STOP_ZONE_1           450.0f   // Erster Versuch: Motor früh aus
#define STOP_ZONE_2           200.0f   // Zweiter Versuch: näher ran
#define DRIVE_FACTOR          3.5f     // PWM = sqrt(error) * factor
#define DRIVE_MAX_PWM         240
#define DRIVE_MIN_PWM         120      // Gleitreibung bei 9V
#define DRIVE_APPROACH_PWM    120      // Konstant in der Nähe des Ziels

// ---- Endanschlag-Schutz ----
#define ENDSTOP_ZONE          300.0f
#define ENDSTOP_MAX_PWM       140

// ---- BRAKE Phase ----
#define BRAKE_TIME_MS         20      // 9V Motor: lange Bremse nötig

// ---- COAST Phase ----
#define COAST_TIME_MS         40       // ADC muss sich beruhigen

// ---- ADC ----
#define ADC_OVERSAMPLE        4
#define FILTER_ALPHA_FAST     0.5f     // Während Motor
#define FILTER_ALPHA_SLOW     0.12f    // Motor aus: sehr glatt

// ---- Timeouts & Block ----
#define MAX_MOVE_MS           3000
#define BLOCK_MOVE_THRESHOLD  8.0f
#define BLOCK_TIMEOUT_MS      400
#define MAX_STUCK_COUNT       3

// ---- Takeover ----
#define TAKEOVER_THRESHOLD    60
#define TAKEOVER_SETTLE_MS    120

// ---- Kalibrierung ----
#define CAL_PWM               150
#define CAL_MIN_TRAVEL        400
#define CAL_MIN_RANGE         1500
#define CAL_TIMEOUT_MS        4000
#define CAL_STOP_MS           180
#define CAL_STUCK_MS          600
#define CAL_MOVE_THRESHOLD    4

// ======================================================
// KLASSE
// ======================================================

class MotorFader
{
public:
    void     begin(uint8_t adcPin, uint8_t pinA, uint8_t pinB, uint8_t chA, uint8_t chB);
    void     update();

    void     setTarget(uint16_t value);
    uint16_t getPosition();
    uint16_t getTarget();
    bool     isAtTarget();
    bool     isMoving();

    uint32_t doneTime() const { return _doneTime; }

    void  calibrateStart(int direction);
    bool  calibrateTick();
    bool  calibrateFinish(uint16_t measuredMin, uint16_t measuredMax);

    bool     _calibrated  = false;
    bool     _calDone     = false;
    bool     _calSuccess  = false;
    uint16_t _calMeasured = 0;

private:
    enum State { IDLE, KICK, DRIVE, BRAKE, COAST, DONE };
    enum CalState { CAL_IDLE, CAL_GOING_UP, CAL_GOING_DOWN };

    State    _state    = IDLE;
    CalState _calState = CAL_IDLE;

    uint8_t _adcPin, _pinA, _pinB, _chA, _chB;

    float    _filtered     = 0;
    float    _position     = 0;
    float    _target       = 0;
    int      _direction    = 0;      // +1 oder -1
    int      _attempt      = 0;      // 0 = erster Versuch, 1 = zweiter

    uint16_t _minPos = 0;
    uint16_t _maxPos = 4095;

    uint32_t _moveStartTime    = 0;
    uint32_t _kickStartTime    = 0;
    uint32_t _brakeStartTime   = 0;
    uint32_t _coastStartTime   = 0;
    uint32_t _doneTime         = 0;

    uint32_t _lastProgressTime = 0;
    float    _lastProgressPos  = 0;
    int      _stuckCount       = 0;

    uint16_t _calStartPos = 0;
    uint16_t _calLastPos  = 0;
    uint32_t _calLastMove = 0;
    uint32_t _calStart    = 0;

    uint16_t readADC();
    void     motorForward(int pwm);
    void     motorReverse(int pwm);
    void     motorCoast();
    void     motorBrake();
    float    toScaled(float raw);
};

extern MotorFader faders[FADER_COUNT];

void     faderBegin();
void     faderLoop();
void     faderSet(uint8_t index, uint16_t value);
uint16_t faderGet(uint8_t index);
uint16_t faderGetTarget(uint8_t index);
bool     faderIsAtTarget(uint8_t index);
void     faderCalibrateAll();

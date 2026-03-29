#include "fader.h"

#define MAX_PWM 255

MotorFader faders[FADER_COUNT];

// ======================================================
// INIT
// ======================================================

void MotorFader::begin(uint8_t adcPin, uint8_t pinA, uint8_t pinB, uint8_t chA, uint8_t chB)
{
    _adcPin = adcPin;
    _pinA   = pinA;
    _pinB   = pinB;
    _chA    = chA;
    _chB    = chB;

    ledcSetup(_chA, FADER_PWM_FREQ, FADER_PWM_RES);
    ledcSetup(_chB, FADER_PWM_FREQ, FADER_PWM_RES);
    ledcAttachPin(_pinA, _chA);
    ledcAttachPin(_pinB, _chB);
    motorCoast();

    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) { sum += analogRead(_adcPin); delayMicroseconds(250); }
    float raw = sum / 16.0f;

    _filtered         = raw;
    _position         = toScaled(raw);
    _target           = _position;
    _lastProgressPos  = _position;
    _stuckCount       = 0;
    _attempt          = 0;
    _lastProgressTime = millis();
    _moveStartTime    = millis();
    _doneTime         = millis();
    _state            = IDLE;
}

// ======================================================
// ADC
// ======================================================

uint16_t MotorFader::readADC()
{
    uint32_t sum = 0;
    for (int i = 0; i < ADC_OVERSAMPLE; i++)
        sum += analogRead(_adcPin);
    return (uint16_t)(sum / ADC_OVERSAMPLE);
}

float MotorFader::toScaled(float raw)
{
    if (_calibrated && _maxPos > _minPos + 10)
        return constrain((raw - _minPos) / (float)(_maxPos - _minPos) * 4095.0f, 0.0f, 4095.0f);
    return constrain(raw, 0.0f, 4095.0f);
}

// ======================================================
// GET / SET
// ======================================================

uint16_t MotorFader::getPosition()  { return (uint16_t)constrain(_position, 0, 4095); }
uint16_t MotorFader::getTarget()    { return (uint16_t)_target; }
bool     MotorFader::isAtTarget()   { return (_state == DONE || _state == IDLE); }
bool     MotorFader::isMoving()     { return !isAtTarget(); }

void MotorFader::setTarget(uint16_t value)
{
    value = constrain(value, 0, 4095);

    float error = fabsf((float)value - _position);

    if (error < ACCEPT_ZONE && (_state == DONE || _state == IDLE))
        return;

    if (fabsf((float)value - _target) < 2.0f && _state != IDLE && _state != DONE)
        return;

    _target           = value;
    _direction        = ((float)value > _position) ? 1 : -1;
    _attempt          = 0;
    _moveStartTime    = millis();
    _lastProgressTime = millis();
    _lastProgressPos  = _position;
    _stuckCount       = 0;

    // Start mit KICK um Haftreibung zu brechen
    _kickStartTime = millis();
    _state = KICK;
}

// ======================================================
// UPDATE – Einfacher State-Machine
// ======================================================

void MotorFader::update()
{
    // ---- ADC ----
    float raw = (float)readADC();
    float alpha = (_state == DRIVE || _state == KICK) ? FILTER_ALPHA_FAST : FILTER_ALPHA_SLOW;
    _filtered = _filtered * (1.0f - alpha) + raw * alpha;
    _position = toScaled(_filtered);

    float error    = _target - _position;
    float absError = fabsf(error);

    // ---- IDLE / DONE ----
    if (_state == IDLE || _state == DONE)
    {
        motorCoast();
        return;
    }

    // ---- Hard-Timeout ----
    if (millis() - _moveStartTime > MAX_MOVE_MS)
    {
        motorCoast();
        _state = DONE; _doneTime = millis();
        return;
    }

    // ==============================
    // KICK: Kurzer Vollgas-Impuls
    // Bricht Haftreibung, dann sofort in DRIVE
    // ==============================
    if (_state == KICK)
    {
        if (millis() - _kickStartTime < KICK_TIME_MS)
        {
            if (_direction > 0) motorForward(KICK_PWM);
            else                motorReverse(KICK_PWM);
        }
        else
        {
            _state = DRIVE;
        }
        return;
    }

    // ==============================
    // DRIVE: Kontinuierlich mit sqrt-Profil
    // ==============================
    if (_state == DRIVE)
    {
        // Aktuelle Stop-Zone je nach Versuch
        float stopZone = (_attempt == 0) ? STOP_ZONE_1 : STOP_ZONE_2;

        // In der Stop-Zone angekommen → Motor aus, bremsen
        if (absError < stopZone)
        {
            motorBrake();
            _brakeStartTime = millis();
            _state = BRAKE;
            return;
        }

        // Bereits am Ziel vorbeigeschossen?
        bool overshot = (error > 0) != (_direction > 0);
        if (overshot)
        {
            motorBrake();
            _brakeStartTime = millis();
            _state = BRAKE;
            return;
        }

        // Block-Detection
        if (fabsf(_position - _lastProgressPos) > BLOCK_MOVE_THRESHOLD)
        {
            _lastProgressTime = millis();
            _lastProgressPos  = _position;
            _stuckCount       = 0;
        }
        if (millis() - _lastProgressTime > BLOCK_TIMEOUT_MS)
        {
            _stuckCount++;
            if (_stuckCount > MAX_STUCK_COUNT)
            {
                motorCoast();
                _state = DONE; _doneTime = millis();
                return;
            }
            _lastProgressTime = millis();
        }

        // ---- PWM berechnen ----
        // sqrt-Profil: natürliches Abbremsen
        int pwm = (int)(sqrtf(absError) * DRIVE_FACTOR);
        pwm = constrain(pwm, DRIVE_MIN_PWM, DRIVE_MAX_PWM);

        // In der Nähe des Ziels (aber noch über stopZone): konstante niedrige PWM
        if (absError < stopZone * 2.0f)
            pwm = min(pwm, DRIVE_APPROACH_PWM);

        // Endanschlag-Schutz
        float posFromEnd = min(_position, 4095.0f - _position);
        if (posFromEnd < ENDSTOP_ZONE)
        {
            float t = posFromEnd / ENDSTOP_ZONE;
            int maxPWM = DRIVE_MIN_PWM + (int)(t * (ENDSTOP_MAX_PWM - DRIVE_MIN_PWM));
            pwm = min(pwm, maxPWM);
        }

        if (error > 0) motorForward(pwm);
        else           motorReverse(pwm);

        return;
    }

    // ==============================
    // BRAKE: Aktive Bremse
    // ==============================
    if (_state == BRAKE)
    {
        motorBrake();
        if (millis() - _brakeStartTime >= BRAKE_TIME_MS)
        {
            motorCoast();
            _coastStartTime = millis();
            _state = COAST;
        }
        return;
    }

    // ==============================
    // COAST: Motor stromlos, ADC stabilisieren, dann entscheiden
    // ==============================
    if (_state == COAST)
    {
        motorCoast();
        if (millis() - _coastStartTime < COAST_TIME_MS)
            return;

        // Jetzt Position prüfen
        if (absError < ACCEPT_ZONE)
        {
            // Ziel erreicht!
            _state = DONE; _doneTime = millis();
            return;
        }

        // Noch nicht da. Zweiter Versuch?
        if (_attempt == 0)
        {
            _attempt = 1;
            _direction = (error > 0) ? 1 : -1;
            _lastProgressTime = millis();
            _lastProgressPos  = _position;
            _stuckCount       = 0;

            // Zweiter Versuch: direkt DRIVE (kein KICK nötig, Motor war gerade aktiv)
            _state = DRIVE;
            return;
        }

        // Zweiter Versuch auch vorbei → akzeptieren
        _state = DONE; _doneTime = millis();
        return;
    }
}

// ======================================================
// DRV8833 MOTOR-STEUERUNG
// ======================================================

void MotorFader::motorForward(int pwm)
{
    pwm = constrain(pwm, 0, MAX_PWM);
    ledcWrite(_chA, pwm);
    ledcWrite(_chB, 0);
}

void MotorFader::motorReverse(int pwm)
{
    pwm = constrain(pwm, 0, MAX_PWM);
    ledcWrite(_chA, 0);
    ledcWrite(_chB, pwm);
}

void MotorFader::motorCoast()
{
    ledcWrite(_chA, 0);
    ledcWrite(_chB, 0);
}

void MotorFader::motorBrake()
{
    ledcWrite(_chA, MAX_PWM);
    ledcWrite(_chB, MAX_PWM);
}

// ======================================================
// KALIBRIERUNG (unverändert)
// ======================================================

void MotorFader::calibrateStart(int direction)
{
    _calState    = (direction > 0) ? CAL_GOING_UP : CAL_GOING_DOWN;
    _calStartPos = analogRead(_adcPin);
    _calLastPos  = _calStartPos;
    _calLastMove = millis();
    _calStart    = millis();
    _calDone     = false;
    _calSuccess  = false;
    _calMeasured = 0;

    if (direction > 0) motorForward(CAL_PWM);
    else               motorReverse(CAL_PWM);
}

bool MotorFader::calibrateTick()
{
    if (_calDone) return true;

    uint16_t pos = analogRead(_adcPin);

    if (abs((int)pos - (int)_calLastPos) > CAL_MOVE_THRESHOLD)
    {
        _calLastMove = millis();
        _calLastPos  = pos;
    }

    if (millis() - _calStart > CAL_TIMEOUT_MS)
    {
        motorCoast();
        _calDone = true; _calSuccess = false;
        return true;
    }

    bool enoughTravel = abs((int)pos - (int)_calStartPos) > CAL_MIN_TRAVEL;
    bool stopped      = millis() - _calLastMove > CAL_STOP_MS;

    bool atEndstop = !enoughTravel && (millis() - _calLastMove > 400)
                     && (millis() - _calStart > 500);

    if (atEndstop)
    {
        motorCoast();
        delay(60);
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) { sum += analogRead(_adcPin); delayMicroseconds(250); }
        _calMeasured = sum / 16;
        _calDone = true; _calSuccess = true;
        return true;
    }

    bool blockedEarly = (millis() - _calLastMove > CAL_STUCK_MS) && !enoughTravel;
    if (blockedEarly)
    {
        motorCoast();
        _calDone = true; _calSuccess = false;
        return true;
    }

    if (enoughTravel && stopped)
    {
        motorCoast();
        delay(60);
        uint32_t sum = 0;
        for (int i = 0; i < 16; i++) { sum += analogRead(_adcPin); delayMicroseconds(250); }
        _calMeasured = sum / 16;
        _calDone = true; _calSuccess = true;
        return true;
    }

    return false;
}

bool MotorFader::calibrateFinish(uint16_t measuredMin, uint16_t measuredMax)
{
    if ((int)measuredMax - (int)measuredMin < CAL_MIN_RANGE || measuredMax <= measuredMin)
        return false;

    _minPos     = measuredMin;
    _maxPos     = measuredMax;
    _calibrated = true;

    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) { sum += analogRead(_adcPin); delayMicroseconds(250); }
    float scaled = toScaled(sum / 16.0f);

    _filtered = sum / 16.0f;
    _position = scaled;
    _target   = scaled;
    _lastProgressPos  = scaled;
    _stuckCount       = 0;
    _attempt          = 0;
    _lastProgressTime = millis();
    _moveStartTime    = millis();
    _doneTime         = millis();
    _state = IDLE;

    return true;
}

// ======================================================
// PARALLELE KALIBRIERUNG
// ======================================================

void faderCalibrateAll()
{
    for (int attempt = 0; attempt < 3; attempt++)
    {
        bool needs[FADER_COUNT];
        int  count = 0;
        for (int i = 0; i < FADER_COUNT; i++)
        {
            needs[i] = !faders[i]._calibrated;
            if (needs[i]) count++;
        }
        if (count == 0) break;

        for (int i = 0; i < FADER_COUNT; i++)
            if (needs[i]) faders[i].calibrateStart(+1);

        while (true)
        {
            bool done = true;
            for (int i = 0; i < FADER_COUNT; i++)
                if (needs[i] && !faders[i]._calDone)
                { faders[i].calibrateTick(); done = false; }
            if (done) break;
            delay(4);
        }

        uint16_t maxMeasured[FADER_COUNT] = {0};
        for (int i = 0; i < FADER_COUNT; i++)
        {
            if (!needs[i]) continue;
            if (faders[i]._calSuccess) maxMeasured[i] = faders[i]._calMeasured;
            else { needs[i] = false; }
        }

        delay(60);

        for (int i = 0; i < FADER_COUNT; i++)
            if (needs[i]) faders[i].calibrateStart(-1);

        while (true)
        {
            bool done = true;
            for (int i = 0; i < FADER_COUNT; i++)
                if (needs[i] && !faders[i]._calDone)
                { faders[i].calibrateTick(); done = false; }
            if (done) break;
            delay(4);
        }

        delay(60);

        for (int i = 0; i < FADER_COUNT; i++)
        {
            if (!needs[i]) continue;
            if (!faders[i]._calSuccess) continue;
            faders[i].calibrateFinish(faders[i]._calMeasured, maxMeasured[i]);
        }

        delay(100);
    }
}

// ======================================================
// GLOBALE API
// ======================================================

void faderBegin()
{
    faders[0].begin(FADER_1_PIN, FADER_MOTOR_1_A_PIN, FADER_MOTOR_1_B_PIN, FADER1_CH_A, FADER1_CH_B);
    faders[1].begin(FADER_2_PIN, FADER_MOTOR_2_A_PIN, FADER_MOTOR_2_B_PIN, FADER2_CH_A, FADER2_CH_B);
    faders[2].begin(FADER_3_PIN, FADER_MOTOR_3_A_PIN, FADER_MOTOR_3_B_PIN, FADER3_CH_A, FADER3_CH_B);
}

void faderLoop()
{
    for (int i = 0; i < FADER_COUNT; i++)
        faders[i].update();
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

uint16_t faderGetTarget(uint8_t index)
{
    if (index >= FADER_COUNT) return 0;
    return faders[index].getTarget();
}

bool faderIsAtTarget(uint8_t index)
{
    if (index >= FADER_COUNT) return true;
    return faders[index].isAtTarget();
}

#include "menu.h"

// ==========================
// REDRAW FLAGS
// ==========================
bool needRedraw      = false;   // Haupt-Display
bool needMiniRedraw  = false;   // Mini-Displays

// Layout-Konstanten für Channel-Config (außerhalb der Funktion für Partial-Updates)
static const int CFG_ITEM_Y[] = { 40, 68, 92, 116, 140, 164 };
static const int CFG_ITEM_H   = 22;
static const int CFG_ITEM_COUNT = 6;

// Early forward declaration (needed by processFaders and button handlers)
static void drawChannelConfigItem(int i);

// ==========================
// TAKEOVER STATE
// Korrekte Logik:
//   armed  = Motor hat sein Ziel erreicht → Fader liegt an der richtigen Position
//   active = User hat den Fader seitdem bewegt → ab jetzt werden Werte übertragen
// ==========================
struct FaderTakeover
{
    bool     armed    = false;
    bool     active   = false;
    uint16_t armPos   = 0;   // Position zum Zeitpunkt des Arming
};

static FaderTakeover takeover[FADER_COUNT];

// ==========================
// EQ BAND ENABLE/DISABLE STATE
// Merkt sich den Gain-Wert wenn ein Band deaktiviert wird
// ==========================
float savedBandGain[MAX_CHANNELS][MAX_EQ_BANDS];
bool  bandDisabled[MAX_CHANNELS][MAX_EQ_BANDS];

// ==========================
// SEITEN
// ==========================
enum MenuPage
{
    PAGE_MAIN,
    PAGE_CHANNEL_POPUP,
    PAGE_CHANNEL_CONFIG,      // neu: Channel-Strip-Einstellungen
    PAGE_NAME_EDITOR,         // neu: Zeicheneditor für Channel-Namen
    PAGE_FX_POPUP,            // neu: FX-Insert-Auswahl
    PAGE_EQ,
    PAGE_EQ_TYPE_POPUP,
    PAGE_COMPRESSOR,
    PAGE_GATE,
    PAGE_SENDS,
    PAGE_ROUTING,
    PAGE_SCENES,
    PAGE_NETWORK,
    PAGE_SETTINGS
};

static struct
{
    MenuPage page             = PAGE_MAIN;
    int      selected         = 0;
    int      channel          = 1;
    int      eqBand           = 0;
    int      eqTypeSelection  = 0;
    bool     redraw           = true;

    // Channel-Config
    int      cfgCursor        = 0;  // 0=Gain, 1=Phantom, 2=Polarity, 3=Link, 4=FX, 5=Name

    // Name-Editor
    char     nameEdit[32]     = {0};
    int      nameCursorPos    = 0;   // Zeichenposition im String
    int      nameCharIdx      = 0;   // Zeichen im Alphabet
    bool     nameInsertMode   = true;

    // FX-Popup
    int      fxSelection      = 0;   // 0=off, 1-4=FX slot
} ui;

// ==========================
// MENÜ-EINTRÄGE
// ==========================
struct MenuItem {
    const char* label;
    const char* sublabel;
    uint16_t    accentR, accentG, accentB;
};

static const MenuItem menuItems[] = {
    { "Channel",    "Strip",          0,   180, 255  },  // hellblau
    { "EQ",         "Equalizer",      40,  210, 80   },  // grün
    { "Dynamics",   "Comp + Gate",    240, 130, 0    },  // orange
    { "Sends",      "Aux Routing",    180, 60,  255  },  // violett
    { "Routing",    "Patching",       0,   200, 180  },  // türkis
    { "Scenes",     "Snapshots",      255, 200, 0    },  // gelb
    { "Meters",     "Levels",         0,   160, 255  },  // blau
    { "Network",    "OSC / WiFi",     255, 80,  80   },  // rot
    { "Settings",   "System",         130, 130, 130  },  // grau
    { "Info",       "About",          80,  80,  80   },  // dunkelgrau
};

#define MENU_COUNT 10

// Helpers use VU_WIDTH from menu.h

// ==========================
// HELPERS
// ==========================
// Stereolink-bewusstes Fader-zu-Kanal-Mapping:
// Ein Stereo-Paar belegt genau EINEN Fader-Slot (immer der niedrigere Kanal).
// Der Slot des Partner-Kanals wird übersprungen.
// Beispiel ui.channel=2, CH3&4 gelinkt:
//   Fader 0 → CH2 (normal)
//   Fader 1 → CH3 (erstes des Paares 3&4)
//   Fader 2 → CH5 (CH4 wird übersprungen, weil es zu CH3 gehört)
int mapFaderToChannel(int i)
{
    int ch     = ui.channel;
    int offset = 0;   // wie viele Kanal-Slots wir bereits durchlaufen haben

    // Schrittweise vorwärts gehen bis wir i echte Fader-Slots gezählt haben
    while (offset < i)
    {
        ch++;
        if (ch > mixerMaxChannels) ch -= mixerMaxChannels;

        // Wenn dieser Kanal der ZWEITE (höhere) eines Stereopaares ist,
        // überspringen (er teilt sich einen Fader mit dem niedrigeren)
        if (ch >= 1 && ch < MAX_CHANNELS && channels[ch].stereoLinked)
        {
            int partner = mixerStereoPartner(ch);
            if (partner < ch)
            {
                // ch ist der zweite/höhere des Paares → skip, kein eigener Slot
                continue;
            }
        }
        offset++;
    }

    // Wenn ch selbst gelinkt ist: immer den niedrigeren (Primär-)Kanal zurückgeben
    if (ch >= 1 && ch < MAX_CHANNELS && channels[ch].stereoLinked)
    {
        int partner = mixerStereoPartner(ch);
        if (partner >= 1 && partner < MAX_CHANNELS)
            ch = min(ch, partner);
    }
    return ch;
}

// Liefert true wenn Fader i ein Stereolink-Paar steuert
static bool faderIsStereo(int i)
{
    int ch = mapFaderToChannel(i);
    return (ch >= 1 && ch < MAX_CHANNELS && channels[ch].stereoLinked);
}

// ==========================
// TAKEOVER RESET
// ==========================
void resetTakeover()
{
    for (int i = 0; i < FADER_COUNT; i++)
    {
        takeover[i].armed  = false;
        takeover[i].active = false;
        takeover[i].armPos = 0;
    }
}

// ==========================
// FADER SYNC → Motor fährt auf Mixer-Wert
// ==========================
void syncFadersToMixer()
{
    for (int i = 0; i < FADER_COUNT; i++)
    {
        int ch = mapFaderToChannel(i);
        faderSet(i, (uint16_t)(channels[ch].fader * 4095.0f));
    }
    resetTakeover();
}

// Gain auf Fader 0, Fader 1+2 unbenutzt (nach unten)
void syncFadersToChannelConfig()
{
    int ch = ui.channel;
    faderSet(0, (uint16_t)(channels[ch].gain * 4095.0f));
    faderSet(1, 0);
    faderSet(2, 0);
    resetTakeover();
}

void syncFadersToEQ()
{
    int ch = ui.channel;

    if (ui.eqBand == 0)
    {
        // HPF: nur Fader 0 = Frequenz, Fader 1+2 unbenutzt → unten
        faderSet(0, (uint16_t)(channels[ch].hpfFreq * 4095.0f));
        faderSet(1, 0);
        faderSet(2, 0);
    }
    else
    {
        auto& b = channels[ch].eq[ui.eqBand];
        faderSet(0, (uint16_t)(b.gain * 4095.0f));
        faderSet(1, (uint16_t)(b.freq * 4095.0f));
        faderSet(2, (uint16_t)(b.q   * 4095.0f));
    }

    resetTakeover();
}

// ==========================
// FADER INPUT – Takeover
// ==========================
void processFaders()
{
    for (int i = 0; i < FADER_COUNT; i++)
    {
        uint16_t pos = faderGet(i);

        // --- Schritt 1: Arming ---
        // Fader muss DONE sein UND SETTLE_TIME abgelaufen sein
        // damit ADC-Rauschen nach Motor-Stop nicht sofort triggert
        if (!takeover[i].armed)
        {
            if (faders[i].isAtTarget()
                && (millis() - faders[i].doneTime() > TAKEOVER_SETTLE_MS))
            {
                takeover[i].armed  = true;
                takeover[i].armPos = pos;   // Stabile Position nach Settle
            }
            else
            {
                takeover[i].armPos = pos;   // Mitführen während Motor läuft
            }
            continue;   // NIEMALS Werte senden solange nicht armed!
        }

        // --- Schritt 2: User-Bewegung erkennen ---
        // Motor muss definitiv aus sein (nicht bewegen)
        if (!takeover[i].active)
        {
            // Falls Motor wieder gestartet wurde → zurücksetzen
            if (faders[i].isMoving())
            {
                takeover[i].armed  = false;
                takeover[i].active = false;
                continue;
            }

            uint16_t moved = (uint16_t)abs((int)pos - (int)takeover[i].armPos);
            if (moved > TAKEOVER_THRESHOLD)
            {
                takeover[i].active = true;
            }
            else
            {
                continue;   // Noch keine Bewegung → keine Werte senden
            }
        }

        // --- Schritt 3: Wert senden (nur wenn active!) ---
        float v = (float)pos / 4095.0f;

        // ---- CHANNEL CONFIG PAGE – Gain auf Fader 0 ----
        if (ui.page == PAGE_CHANNEL_CONFIG)
        {
            if (i == 0)
            {
                int ch = ui.channel;
                if (fabsf(channels[ch].gain - v) > 0.004f)
                {
                    mixerSetGain(ch, v);
                    char addr[32];
                    snprintf(addr, sizeof(addr), "/headamp/%02d/gain", ch);
                    oscSendFloat(addr, v);
                    // Nur die Gain-Zeile neu zeichnen, nicht das ganze Display
                    drawChannelConfigItem(0);
                    needMiniRedraw = true;
                }
            }
            // Fader 1+2 unbenutzt auf dieser Page
        }

        // ---- CHANNEL / MAIN PAGE ----
        else if (ui.page == PAGE_MAIN || ui.page == PAGE_CHANNEL_POPUP)
        {
            int ch = mapFaderToChannel(i);

            if (fabsf(channels[ch].fader - v) > 0.005f)
            {
                mixerSetFader(ch, v);

                char addr[32];
                snprintf(addr, sizeof(addr), "/ch/%02d/mix/fader", ch);
                oscSendFloat(addr, v);

                // Kein needRedraw hier: Fader-Bewegung ändert nichts am Menü-Display
                // Mini-Displays müssen aktualisiert werden (Kanalname/VU)
                needMiniRedraw = true;
            }
        }

        // ---- EQ PAGE ----
        else if (ui.page == PAGE_EQ)
        {
            int ch = ui.channel;

            if (ui.eqBand == 0)
            {
                // HPF: Nur Fader 0 = Frequenz
                if (i == 0 && fabsf(channels[ch].hpfFreq - v) > 0.005f)
                {
                    mixerSetHPFFreq(ch, v);

                    char addr[32];
                    snprintf(addr, sizeof(addr), "/ch/%02d/preamp/hpf", ch);
                    oscSendFloat(addr, v);

                    needRedraw     = true;
                    needMiniRedraw = true;
                }
                // Fader 1 und 2 haben auf HPF-Seite keine Funktion
            }
            else
            {
                auto& b = channels[ch].eq[ui.eqBand];

                if (i == 0 && fabsf(b.gain - v) > 0.005f)
                {
                    mixerSetEqGain(ch, ui.eqBand, v);

                    // Wenn Band disabled war und User den Gain-Fader bewegt → wieder aktivieren
                    if (bandDisabled[ch][ui.eqBand])
                    {
                        bandDisabled[ch][ui.eqBand] = false;
                    }

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/g", ch, ui.eqBand);
                    oscSendFloat(addr, v);

                    needRedraw     = true;
                    needMiniRedraw = true;
                }
                if (i == 1 && fabsf(b.freq - v) > 0.005f)
                {
                    mixerSetEqFreq(ch, ui.eqBand, v);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/f", ch, ui.eqBand);
                    oscSendFloat(addr, v);

                    needRedraw     = true;
                    needMiniRedraw = true;
                }
                if (i == 2 && fabsf(b.q - v) > 0.005f)
                {
                    mixerSetEqQ(ch, ui.eqBand, v);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/q", ch, ui.eqBand);
                    oscSendFloat(addr, v);

                    needRedraw     = true;
                    needMiniRedraw = true;
                }
            }
        }
    }
}

// ==========================
// MINI DISPLAY
// ==========================

// Gain 0.0-1.0 → dB: XAir preamp/gain range = -12..+60 dB (72 dB)
static float gainToDb(float v) { return -12.0f + v * 72.0f; }

static float scaleFreq(float v)
{
    float logMin = log10f(20.0f);
    float logMax = log10f(20000.0f);
    return powf(10.0f, logMin + v * (logMax - logMin));
}

static float scaleHPFFreq(float v)
{
    // HPF: 0.0-1.0 → 20-400 Hz
    float logMin = log10f(20.0f);
    float logMax = log10f(400.0f);
    return powf(10.0f, logMin + v * (logMax - logMin));
}

void updateMiniDisplays()
{
    if (ui.page == PAGE_EQ)
    {
        int ch = ui.channel;

        if (ui.eqBand == 0)
        {
            // HPF-Seite: Fader 0 = Freq, Fader 1+2 unbenutzt
            float freqHz = scaleHPFFreq(channels[ch].hpfFreq);
            char freqBuf[16];
            if (freqHz >= 100) snprintf(freqBuf, sizeof(freqBuf), "%.0f", freqHz);
            else               snprintf(freqBuf, sizeof(freqBuf), "%.1f", freqHz);

            // Channel VU auf Display 0
            float vu = (ch >= 1 && ch <= 16) ? meters[2 + (ch - 1)] : 0.0f;

            miniDisplayTextWithVU(0, channels[ch].hpfOn ? "HPF ON" : "HPF OFF",
                                  String(freqBuf) + "Hz", vu);

            // Display 1+2 ausgeschaltet
            miniDisplayClear(1);
            miniDisplayClear(2);

            // LEDs: Button 1/2 = HPF On/Off, Button 3 = EQ On/Off
            ledSet(0, channels[ch].hpfOn);
            ledSet(1, channels[ch].hpfOn);
            ledSet(2, channels[ch].eqOn);
        }
        else
        {
            auto& b = channels[ch].eq[ui.eqBand];

            float gainDb = -15.0f + b.gain * 30.0f;
            float freqHz = scaleFreq(b.freq);
            float qVal   = 10.0f * powf(0.03f, b.q);

            // Channel-VU NUR auf Display 0
            float vu = (ch >= 1 && ch <= 16) ? meters[2 + (ch - 1)] : 0.0f;

            // Gain-Anzeige mit dB-Vorzeichen + VU
            char gainBuf[16];
            snprintf(gainBuf, sizeof(gainBuf), "%+.1f", gainDb);
            miniDisplayTextWithVU(0, "GAIN", String(gainBuf) + "dB", vu);

            // Freq-Anzeige – schickes Param-Layout ohne VU
            char freqBuf[16];
            if (freqHz >= 10000)     snprintf(freqBuf, sizeof(freqBuf), "%.1fk", freqHz / 1000.0f);
            else if (freqHz >= 1000) snprintf(freqBuf, sizeof(freqBuf), "%.2fk", freqHz / 1000.0f);
            else                     snprintf(freqBuf, sizeof(freqBuf), "%.0f", freqHz);
            miniDisplayParam(1, "FREQ", freqBuf, "Hz");

            // Q-Anzeige – schickes Param-Layout ohne VU
            char qBuf[16];
            if (qVal >= 10.0f)      snprintf(qBuf, sizeof(qBuf), "%.1f", qVal);
            else if (qVal >= 1.0f)  snprintf(qBuf, sizeof(qBuf), "%.2f", qVal);
            else                    snprintf(qBuf, sizeof(qBuf), "%.3f", qVal);
            miniDisplayParam(2, "Q", qBuf);

            // LEDs: Button 1/2 = Band enabled, Button 3 = EQ On
            bool bEnabled = !bandDisabled[ch][ui.eqBand];
            ledSet(0, bEnabled);
            ledSet(1, bEnabled);
            ledSet(2, channels[ch].eqOn);
        }
    }
    else if (ui.page == PAGE_CHANNEL_CONFIG
          || ui.page == PAGE_NAME_EDITOR
          || ui.page == PAGE_FX_POPUP)
    {
        // Fader 0 = Gain des ausgewählten Kanals
        int ch = ui.channel;
        float gainDb = gainToDb(channels[ch].gain);
        char gainBuf[12];
        if (gainDb >= 0)
            snprintf(gainBuf, sizeof(gainBuf), "+%.0fdB", gainDb);
        else
            snprintf(gainBuf, sizeof(gainBuf), "%.0fdB", gainDb);

        float vu = (ch >= 1 && ch <= 16) ? meters[2 + (ch - 1)] : 0.0f;

        // Display 0: Gain + VU
        miniDisplayTextWithVU(0, "GAIN", gainBuf, vu);

        // Display 1: Phantom + Polarity
        char line1[16], line2[16];
        snprintf(line1, sizeof(line1), "%s  %s",
                 channels[ch].phantom ? "48V:ON" : "48V:--",
                 channels[ch].polarity ? "POL:INV" : "POL:---");
        snprintf(line2, sizeof(line2), "CH %02d", ch);
        miniDisplayText(1, String(line1) + "\n" + String(line2));

        // Display 2: Stereolink-Status oder Name
        if (channels[ch].stereoLinked)
        {
            int p = mixerStereoPartner(ch);
            char lbuf[16];
            snprintf(lbuf, sizeof(lbuf), "%d & %d", min(ch, p), max(ch, p));
            miniDisplayText(2, String("STEREO\n") + String(lbuf));
        }
        else if (channels[ch].insOn && channels[ch].insMode != INS_OFF)
        {
            char fbuf[16];
            snprintf(fbuf, sizeof(fbuf), "FX%d INSERT", (int)channels[ch].insMode);
            miniDisplayText(2, String("INSERT\n") + String(fbuf));
        }
        else
        {
            miniDisplayText(2, strlen(channels[ch].name) > 0
                ? String("NAME\n") + String(channels[ch].name)
                : String("NAME\n---"));
        }

        // LEDs: Phantom/Polarity/Link
        ledSet(0, channels[ch].phantom);
        ledSet(1, channels[ch].polarity);
        ledSet(2, channels[ch].stereoLinked);
    }
    else
    {
        // PAGE_MAIN / PAGE_CHANNEL_POPUP: pro Fader = Channel-Name + Channel-VU
        for (int i = 0; i < FADER_COUNT; i++)
        {
            int ch = mapFaderToChannel(i);

            // Meter-Slot: 0/1 = Master L/R, 2+ = Channel 1-16
            float vu = (ch >= 1 && ch <= 16) ? meters[2 + (ch - 1)] : 0.0f;

            // Formatierte Fader-dB-Anzeige
            float faderVal = channels[ch].fader;
            char dbBuf[12];
            if (faderVal < 0.001f) snprintf(dbBuf, sizeof(dbBuf), "-inf");
            else {
                float db = 20.0f * log10f(faderVal);
                snprintf(dbBuf, sizeof(dbBuf), "%.1fdB", db);
            }

            // Stereolink: Zeile 1 zeigt "1&2" statt "CH1"
            String line1;
            if (faderIsStereo(i))
            {
                int partner = mixerStereoPartner(ch);
                line1 = "CH" + String(min(ch, partner)) + "&" + String(max(ch, partner));
            }
            else
            {
                line1 = "CH" + String(ch) + " " + String(channels[ch].name);
            }
            String line2 = String(dbBuf);

            miniDisplayTextWithVU(i, line1, line2, vu);

            ledSet(i, channels[ch].mute);
        }
    }
}

// ==========================
// RETURN LED (blinkt außerhalb PAGE_MAIN)
// ==========================
void updateReturnLED()
{
    static uint32_t t = 0;
    static bool     s = false;

    if (ui.page == PAGE_MAIN)
    {
        ledSet(3, false);
        return;
    }

    if (millis() - t > 400)
    {
        t = millis();
        s = !s;
        ledSet(3, s);
    }
}

// ==========================
// INPUT
// ==========================
// Forward-Declaration (Definition weiter unten bei den Draw-Funktionen)
static void drawMenuItem(int index);
static void drawChannelPopupContent();
static void drawChannelConfigItem(int i);
static void drawEQTypePopupItem(int i);
void drawChannelConfig();
void drawNameEditor();
void drawFXPopup();

void handleInput()
{
    int d = encoderGetDelta();

    if (d != 0)
    {
        if (ui.page == PAGE_MAIN)
        {
            int oldSelected = ui.selected;
            ui.selected = (ui.selected + d + MENU_COUNT) % MENU_COUNT;

            // Nur die zwei betroffenen Zeilen neu zeichnen – kein Full-Redraw
            drawMenuItem(oldSelected);
            drawMenuItem(ui.selected);
            // ui.redraw NICHT setzen – kein komplettes drawMenu() nötig
        }
        else if (ui.page == PAGE_CHANNEL_CONFIG)
        {
            int old = ui.cfgCursor;
            ui.cfgCursor = (ui.cfgCursor + d + CFG_ITEM_COUNT) % CFG_ITEM_COUNT;
            // Nur die zwei betroffenen Zeilen neu zeichnen
            drawChannelConfigItem(old);
            drawChannelConfigItem(ui.cfgCursor);
            // ui.redraw NICHT setzen
        }
        else if (ui.page == PAGE_NAME_EDITOR)
        {
            const int NAME_CHARS = 38;
            ui.nameCharIdx = (ui.nameCharIdx + d + NAME_CHARS) % NAME_CHARS;
            const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
            if (ui.nameCursorPos < 31)
                ui.nameEdit[ui.nameCursorPos] = charset[ui.nameCharIdx];
            // Nur den Zeichenbereich im Popup neu rendern (kein Full-Redraw)
            drawNameEditor();
        }
        else if (ui.page == PAGE_FX_POPUP)
        {
            ui.fxSelection = (ui.fxSelection + d + 5) % 5;
            // Nur das Popup neu rendern
            drawFXPopup();
        }
        else if (ui.page == PAGE_CHANNEL_POPUP)
        {
            ui.channel += d;
            if (ui.channel < 1)                ui.channel = mixerMaxChannels;
            if (ui.channel > mixerMaxChannels)  ui.channel = 1;

            syncFadersToMixer();
            needMiniRedraw = true;
            // Nur den veränderlichen Inhalt des Popups neu rendern
            drawChannelPopupContent();
        }
        else if (ui.page == PAGE_EQ)
        {
            ui.eqBand += d;
            if (ui.eqBand < 0) ui.eqBand = 4;
            if (ui.eqBand > 4) ui.eqBand = 0;

            syncFadersToEQ();
            needMiniRedraw = true;
            ui.redraw = true;
        }
        else if (ui.page == PAGE_EQ_TYPE_POPUP)
        {
            int old = ui.eqTypeSelection;
            ui.eqTypeSelection = (ui.eqTypeSelection + d + 6) % 6;
            // Nur die zwei betroffenen Zeilen neuzeichnen
            tft.setTextSize(1);
            drawEQTypePopupItem(old);
            drawEQTypePopupItem(ui.eqTypeSelection);
            tft.setTextSize(2);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            // KEIN ui.redraw!
        }
        else
        {
            ui.redraw = true;
        }
    }

    // ---- MUTE BUTTONS ----
    if (ui.page == PAGE_MAIN || ui.page == PAGE_CHANNEL_POPUP)
    {
        // Hauptmenü: Mute-Taster für Channels
        for (int i = 0; i < FADER_COUNT; i++)
        {
            if (buttonPressed((ButtonID)i))
            {
                int  ch = mapFaderToChannel(i);
                bool m  = !channels[ch].mute;

                mixerSetMute(ch, m);

                char addr[32];
                snprintf(addr, sizeof(addr), "/ch/%02d/mix/on", ch);
                oscSendInt(addr, m ? 0 : 1);   // XAir: on=0 bedeutet gemuted

                needMiniRedraw = true;
                ui.redraw      = true;
            }
        }
    }
    else if (ui.page == PAGE_CHANNEL_CONFIG)
    {
        // Button 1 = 48V Phantom toggle
        if (buttonPressed(BUTTON_1))
        {
            int ch = ui.channel;
            bool newVal = !channels[ch].phantom;
            mixerSetPhantom(ch, newVal);
            char addr[48];
            snprintf(addr, sizeof(addr), "/headamp/%02d/phantom", ch);
            oscSendInt(addr, newVal ? 1 : 0);
            drawChannelConfigItem(1);  // Nur 48V-Zeile neu
            needMiniRedraw = true;
        }
        // Button 2 = Polarity toggle
        if (buttonPressed(BUTTON_2))
        {
            int ch = ui.channel;
            bool newVal = !channels[ch].polarity;
            mixerSetPolarity(ch, newVal);
            char addr[48];
            snprintf(addr, sizeof(addr), "/ch/%02d/preamp/invert", ch);
            oscSendInt(addr, newVal ? 1 : 0);
            drawChannelConfigItem(2);  // Nur Polarity-Zeile neu
            needMiniRedraw = true;
        }
        // Button 3 = Stereo Link toggle
        if (buttonPressed(BUTTON_3))
        {
            int ch = ui.channel;
            bool newVal = !channels[ch].stereoLinked;
            mixerSetStereoLink(ch, newVal);
            int partner = mixerStereoPartner(ch);
            int lo = min(ch, partner);
            int hi = max(ch, partner);
            char addr[48];
            snprintf(addr, sizeof(addr), "/config/chlink/%d-%d", lo, hi);
            oscSendInt(addr, newVal ? 1 : 0);
            drawChannelConfigItem(3);  // Nur Stereolink-Zeile neu
            needMiniRedraw = true;
        }
    }
    else if (ui.page == PAGE_EQ)
    {
        int ch = ui.channel;

        // ---- Button 1: HPF On/Off ODER Band 1-4 Enable/Disable ----
        if (buttonPressed(BUTTON_1))
        {
            if (ui.eqBand == 0)
            {
                // HPF On/Off Toggle
                bool newState = !channels[ch].hpfOn;
                mixerSetHPFOn(ch, newState);

                char addr[32];
                snprintf(addr, sizeof(addr), "/ch/%02d/preamp/hpon", ch);
                oscSendInt(addr, newState ? 1 : 0);
            }
            else
            {
                // Aktuelles EQ-Band Enable/Disable
                int b = ui.eqBand;
                if (!bandDisabled[ch][b])
                {
                    // Deaktivieren: Gain merken und auf 0.5 (0dB) setzen
                    savedBandGain[ch][b] = channels[ch].eq[b].gain;
                    bandDisabled[ch][b] = true;

                    mixerSetEqGain(ch, b, 0.5f);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/g", ch, b);
                    oscSendFloat(addr, 0.5f);
                }
                else
                {
                    // Aktivieren: gespeicherten Gain wiederherstellen
                    bandDisabled[ch][b] = false;
                    float restoreGain = savedBandGain[ch][b];

                    mixerSetEqGain(ch, b, restoreGain);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/g", ch, b);
                    oscSendFloat(addr, restoreGain);
                }

                // Fader auf neuen Gain-Wert fahren
                syncFadersToEQ();
            }

            needRedraw     = true;
            needMiniRedraw = true;
            ui.redraw      = true;
        }

        // ---- Button 2: (reserviert / gleiche Funktion wie Button 1 für Komfort) ----
        if (buttonPressed(BUTTON_2))
        {
            // Gleiche Logik wie Button 1 – zweiter Taster für selbe Funktion
            if (ui.eqBand == 0)
            {
                bool newState = !channels[ch].hpfOn;
                mixerSetHPFOn(ch, newState);

                char addr[32];
                snprintf(addr, sizeof(addr), "/ch/%02d/preamp/hpon", ch);
                oscSendInt(addr, newState ? 1 : 0);
            }
            else
            {
                int b = ui.eqBand;
                if (!bandDisabled[ch][b])
                {
                    savedBandGain[ch][b] = channels[ch].eq[b].gain;
                    bandDisabled[ch][b] = true;
                    mixerSetEqGain(ch, b, 0.5f);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/g", ch, b);
                    oscSendFloat(addr, 0.5f);
                }
                else
                {
                    bandDisabled[ch][b] = false;
                    float restoreGain = savedBandGain[ch][b];
                    mixerSetEqGain(ch, b, restoreGain);

                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/g", ch, b);
                    oscSendFloat(addr, restoreGain);
                }
                syncFadersToEQ();
            }

            needRedraw     = true;
            needMiniRedraw = true;
            ui.redraw      = true;
        }

        // ---- Button 3: Gesamt-EQ On/Off ----
        if (buttonPressed(BUTTON_3))
        {
            bool newEqState = !channels[ch].eqOn;
            mixerSetEqOn(ch, newEqState);

            char addr[32];
            snprintf(addr, sizeof(addr), "/ch/%02d/eq/on", ch);
            oscSendInt(addr, newEqState ? 1 : 0);

            needRedraw     = true;
            needMiniRedraw = true;
            ui.redraw      = true;
        }
    }

    // ---- ENCODER BUTTON (Auswahl / Bestätigung) ----
    if (buttonPressed(BUTTON_ENCODER))
    {
        if (ui.page == PAGE_MAIN)
        {
            switch (ui.selected)
            {
                case 0: ui.page = PAGE_CHANNEL_POPUP;                             break;
                case 1: ui.page = PAGE_EQ;      syncFadersToEQ();                 break;
                case 2: ui.page = PAGE_COMPRESSOR;                                break;
                case 3: ui.page = PAGE_SENDS;                                     break;
                case 4: ui.page = PAGE_ROUTING;                                   break;
                case 5: ui.page = PAGE_SCENES;                                    break;
                case 6: ui.page = PAGE_MAIN;   /* Meters: kein Sub-Menü */        break;
                case 7: ui.page = PAGE_NETWORK;                                   break;
                case 8: ui.page = PAGE_SETTINGS;                                  break;
                default: break;
            }
            needMiniRedraw = true;
        }
        // ---- CHANNEL POPUP: Bestätigung → Channel-Config öffnen ----
        else if (ui.page == PAGE_CHANNEL_POPUP)
        {
            ui.page = PAGE_CHANNEL_CONFIG;
            ui.cfgCursor = 0;
            syncFadersToChannelConfig();
            needMiniRedraw = true;
        }
        // ---- CHANNEL CONFIG: Aktion für ausgewähltes Item ----
        else if (ui.page == PAGE_CHANNEL_CONFIG)
        {
            int ch = ui.channel;
            switch (ui.cfgCursor)
            {
                case 0: // Gain – wird per Fader gesteuert, kein Button-Toggle
                    break;
                case 1: // 48V Phantom
                {
                    bool newVal = !channels[ch].phantom;
                    mixerSetPhantom(ch, newVal);
                    char addr[48];
                    snprintf(addr, sizeof(addr), "/headamp/%02d/phantom", ch);
                    oscSendInt(addr, newVal ? 1 : 0);
                    break;
                }
                case 2: // Polarity
                {
                    bool newVal = !channels[ch].polarity;
                    mixerSetPolarity(ch, newVal);
                    char addr[48];
                    snprintf(addr, sizeof(addr), "/ch/%02d/preamp/invert", ch);
                    oscSendInt(addr, newVal ? 1 : 0);
                    break;
                }
                case 3: // Stereo Link
                {
                    bool newVal = !channels[ch].stereoLinked;
                    mixerSetStereoLink(ch, newVal);
                    int partner = mixerStereoPartner(ch);
                    int lo = min(ch, partner);
                    int hi = max(ch, partner);
                    char addr[48];
                    snprintf(addr, sizeof(addr), "/config/chlink/%d-%d", lo, hi);
                    oscSendInt(addr, newVal ? 1 : 0);
                    break;
                }
                case 4: // FX-Insert → Popup öffnen
                    ui.fxSelection = (int)channels[ch].insMode;
                    ui.page = PAGE_FX_POPUP;
                    break;
                case 5: // Name-Editor öffnen
                    strncpy(ui.nameEdit, channels[ch].name, sizeof(ui.nameEdit) - 1);
                    ui.nameEdit[sizeof(ui.nameEdit) - 1] = '\0';
                    ui.nameCursorPos = 0;
                    // Startzeichen im Charset suchen
                    {
                        const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
                        char c = (ui.nameEdit[0] != '\0') ? ui.nameEdit[0] : ' ';
                        ui.nameCharIdx = 0;
                        for (int k = 0; charset[k]; k++)
                            if (charset[k] == c) { ui.nameCharIdx = k; break; }
                    }
                    ui.page = PAGE_NAME_EDITOR;
                    break;
            }
            ui.redraw = true;
            needMiniRedraw = true;
        }
        // ---- NAME EDITOR: Cursor eine Stelle weiter ----
        else if (ui.page == PAGE_NAME_EDITOR)
        {
            int ch = ui.channel;
            // Leerzeichen-Trailing trimmen
            int len = strlen(ui.nameEdit);
            while (len > 0 && ui.nameEdit[len - 1] == ' ') ui.nameEdit[--len] = '\0';

            ui.nameCursorPos++;
            if (ui.nameCursorPos >= 16 || ui.nameCursorPos >= len + 1)
            {
                // Namen senden
                mixerSetName(ch, ui.nameEdit);
                char addr[48];
                snprintf(addr, sizeof(addr), "/ch/%02d/config/name", ch);
                oscSendString(addr, ui.nameEdit);
                ui.page = PAGE_CHANNEL_CONFIG;
            }
            else
            {
                // Nächste Stelle: passendes Zeichen vorladen
                const char charset[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
                char c = (ui.nameEdit[ui.nameCursorPos] != '\0')
                         ? ui.nameEdit[ui.nameCursorPos] : ' ';
                ui.nameCharIdx = 0;
                for (int k = 0; charset[k]; k++)
                    if (charset[k] == c) { ui.nameCharIdx = k; break; }
            }
            ui.redraw = true;
        }
        // ---- FX POPUP: Auswahl bestätigen ----
        else if (ui.page == PAGE_FX_POPUP)
        {
            int ch = ui.channel;
            InsMode newMode = (InsMode)ui.fxSelection;
            mixerSetInsMode(ch, newMode);
            bool insOn = (newMode != INS_OFF);
            mixerSetInsOn(ch, insOn);

            char addrMode[48], addrOn[48];
            snprintf(addrMode, sizeof(addrMode), "/ch/%02d/insert/sel", ch);
            snprintf(addrOn,   sizeof(addrOn),   "/ch/%02d/insert/on",  ch);
            oscSendInt(addrMode, (int)newMode);
            oscSendInt(addrOn,   insOn ? 1 : 0);

            ui.page = PAGE_CHANNEL_CONFIG;
            ui.redraw = true;
        }
        else if (ui.page == PAGE_EQ)
        {
            // Nur für Bänder 1-4 das Typ-Popup öffnen, NICHT für HPF (Band 0)
            if (ui.eqBand >= 1 && ui.eqBand <= 4)
            {
                ui.eqTypeSelection = (int)channels[ui.channel].eq[ui.eqBand].type;
                ui.page = PAGE_EQ_TYPE_POPUP;
            }
        }
        else if (ui.page == PAGE_EQ_TYPE_POPUP)
        {
            EqType newType = (EqType)ui.eqTypeSelection;
            channels[ui.channel].eq[ui.eqBand].type = newType;
            char addr[48];
            snprintf(addr, sizeof(addr), "/ch/%02d/eq/%d/t", ui.channel, ui.eqBand);
            oscSendInt(addr, (int)newType);
            ui.page = PAGE_EQ;
        }
        else
        {
            ui.page = PAGE_MAIN;
            syncFadersToMixer();
            needMiniRedraw = true;
        }

        ui.redraw = true;
    }

    // ---- RETURN BUTTON ----
    if (buttonPressed(BUTTON_RETURN))
    {
        if (ui.page == PAGE_EQ_TYPE_POPUP)
            ui.page = PAGE_EQ;
        else if (ui.page == PAGE_NAME_EDITOR)
        {
            // Abbruch ohne Speichern
            ui.page = PAGE_CHANNEL_CONFIG;
        }
        else if (ui.page == PAGE_FX_POPUP)
        {
            ui.page = PAGE_CHANNEL_CONFIG;
        }
        else if (ui.page == PAGE_CHANNEL_CONFIG)
        {
            ui.page = PAGE_CHANNEL_POPUP;
            syncFadersToMixer();
            needMiniRedraw = true;
        }
        else
        {
            ui.page = PAGE_MAIN;
            syncFadersToMixer();
            needMiniRedraw = true;
        }
        ui.redraw = true;
    }
}

// ==========================
// DRAW HELPERS
// ==========================

// Farbiger Akzentpunkt links vom Menü-Label
static void drawAccentDot(int x, int y, int r, int g, int b, bool filled)
{
    uint16_t col = tft.color565(r, g, b);
    if (filled)
    {
        tft.fillCircle(x, y, 4, col);
        tft.drawCircle(x, y, 4, TFT_WHITE);
    }
    else
    {
        tft.fillCircle(x, y, 4, tft.color565(20, 20, 20));
        tft.drawCircle(x, y, 4, col);
    }
}

// Horizontale Trennlinie mit sanftem Verlauf an den Rändern
static void drawSeparator(int y, int x0, int x1)
{
    uint16_t col = tft.color565(40, 40, 40);
    // Hauptlinie
    tft.drawFastHLine(x0 + 4, y, x1 - x0 - 8, col);
    // Weicher Einlauf/Auslauf
    tft.drawPixel(x0 + 2, y, tft.color565(15, 15, 15));
    tft.drawPixel(x0 + 3, y, tft.color565(25, 25, 25));
    tft.drawPixel(x1 - 3, y, tft.color565(25, 25, 25));
    tft.drawPixel(x1 - 2, y, tft.color565(15, 15, 15));
}

// ==========================
// MAIN MENÜ
// ==========================
// Konstanten für Menü-Layout (auch von drawMenuItem verwendet)
static const int MENU_ITEM_H   = 21;
static const int MENU_START_Y  = 22;
static const int MENU_PAD_LEFT = 18;
static const int MENU_LEFT_M   = 3;

// Zeichnet genau EINE Menüzeile (für effizientes Partial-Update beim Scrollen)
static void drawMenuItem(int i)
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    int  y   = MENU_START_Y + i * MENU_ITEM_H;
    bool sel = (i == ui.selected);

    uint16_t accentCol = tft.color565(
        menuItems[i].accentR, menuItems[i].accentG, menuItems[i].accentB);
    uint16_t dimCol = tft.color565(
        menuItems[i].accentR / 4, menuItems[i].accentG / 4, menuItems[i].accentB / 4);

    // Hintergrund + linker Balken
    if (sel)
    {
        tft.fillRect(MENU_LEFT_M, y, CONTENT_W - MENU_LEFT_M, MENU_ITEM_H - 1, dimCol);
        tft.fillRect(MENU_LEFT_M, y + 1, 3, MENU_ITEM_H - 3, accentCol);
    }
    else
    {
        tft.fillRect(0, y, CONTENT_W, MENU_ITEM_H - 1, TFT_BLACK);
        tft.fillRect(MENU_LEFT_M, y + 1, 2, MENU_ITEM_H - 3, tft.color565(28, 28, 28));
    }

    // Akzent-Punkt
    drawAccentDot(MENU_LEFT_M + 8, y + MENU_ITEM_H / 2,
                  menuItems[i].accentR, menuItems[i].accentG, menuItems[i].accentB, sel);

    tft.setTextSize(1);

    // Label
    if (sel) tft.setTextColor(TFT_WHITE, dimCol);
    else     tft.setTextColor(tft.color565(160, 160, 160), TFT_BLACK);
    tft.setCursor(MENU_PAD_LEFT, y + 4);
    tft.print(menuItems[i].label);

    // Sublabel
    tft.setTextColor(sel ? accentCol : tft.color565(70, 70, 70),
                     sel ? dimCol    : TFT_BLACK);
    tft.setCursor(MENU_PAD_LEFT + strlen(menuItems[i].label) * 6 + 4, y + 4);
    tft.print(menuItems[i].sublabel);

    // Channel-Nummer rechts beim ersten Eintrag
    if (i == 0)
    {
        char buf[8];
        snprintf(buf, sizeof(buf), "CH%d", ui.channel);
        tft.setTextColor(accentCol, sel ? dimCol : TFT_BLACK);
        tft.setCursor(CONTENT_W - (int)strlen(buf) * 6 - 6, y + 4);
        tft.print(buf);
    }

    // Trennlinie (nicht nach letztem Element)
    if (i < MENU_COUNT - 1)
        drawSeparator(y + MENU_ITEM_H - 1, 6, CONTENT_W - 6);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawMenu()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;

    tft.fillRect(0, MENU_START_Y, CONTENT_W, tft.height() - MENU_START_Y, TFT_BLACK);

    for (int i = 0; i < MENU_COUNT; i++)
        drawMenuItem(i);

    // Rahmen-Linien
    tft.drawFastVLine(0, MENU_START_Y + 6, tft.height() - MENU_START_Y - 12,
                      tft.color565(22, 22, 22));
    tft.drawFastVLine(CONTENT_W - 1, MENU_START_Y, tft.height() - MENU_START_Y,
                      tft.color565(20, 20, 20));

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// CHANNEL POPUP
// ==========================
// Zeichnet nur den veränderlichen Inhalt des Channel-Popups (Nummer + Name)
static void drawChannelPopupContent()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    const int w = 160, h = 90;
    int x = (CONTENT_W - w) / 2;
    int y = (tft.height() - h) / 2;
    uint16_t popupBg = tft.color565(25, 25, 25);

    // Kanal-Nummer (verändert sich beim Scrollen)
    tft.fillRect(x + 1, y + 19, w - 2, 46, popupBg);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE, popupBg);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", ui.channel);
    int tw = strlen(buf) * 18;
    tft.setCursor(x + (w - tw) / 2, y + 30);
    tft.print(buf);

    // Channel-Name
    tft.setTextSize(1);
    tft.fillRect(x + 1, y + 64, w - 2, 11, popupBg);
    const char* chName = channels[ui.channel].name;
    if (strlen(chName) > 0)
    {
        tft.setTextColor(tft.color565(120, 120, 120), popupBg);
        int nw = strlen(chName) * 6;
        tft.setCursor(x + (w - nw) / 2, y + 66);
        tft.print(chName);
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void drawChannelPopup()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    const int w = 160, h = 90;
    int x = (CONTENT_W - w) / 2;
    int y = (tft.height() - h) / 2;
    uint16_t popupBg = tft.color565(25, 25, 25);

    tft.fillRect(x + 3, y + 3, w, h, tft.color565(10, 10, 10));
    tft.fillRect(x, y, w, h, popupBg);
    tft.drawRect(x, y, w, h, tft.color565(0, 180, 255));

    tft.fillRect(x, y, w, 18, tft.color565(0, 60, 90));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(0, 180, 255), tft.color565(0, 60, 90));
    tft.setCursor(x + 8, y + 5);
    tft.print("SELECT CHANNEL");

    // Inhalt (Nummer + Name)
    drawChannelPopupContent();

    // Hinweis unten – statisch
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(0, 120, 180), popupBg);
    tft.setCursor(x + 6, y + 76);
    tft.print("< > scroll  ENC=config");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// CHANNEL CONFIG PAGE

// Zeichnet genau EINE Zeile der Channel-Config (für effizientes Partial-Update beim Scrollen)
static void drawChannelConfigItem(int i)
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    int ch = ui.channel;
    ChannelState& c = channels[ch];

    int  iy  = CFG_ITEM_Y[i];
    bool sel = (i == ui.cfgCursor);

    const char* labels[] = { "GAIN", "48V", "POLARITY", "STEREO LINK", "FX INSERT", "NAME" };
    uint16_t aColors[6] = {
        tft.color565(0,   180, 255),
        tft.color565(255, 200, 0),
        tft.color565(240, 100, 0),
        tft.color565(100, 220, 100),
        tft.color565(180, 60,  255),
        tft.color565(130, 130, 130),
    };

    uint16_t ac = aColors[i];
    uint16_t bg = sel ? tft.color565(15, 20, 30) : TFT_BLACK;

    tft.fillRect(2,  iy, CONTENT_W - 4, CFG_ITEM_H, bg);
    if (sel) tft.fillRect(2, iy, 3, CFG_ITEM_H, ac);

    tft.setTextSize(1);
    tft.setTextColor(sel ? ac : tft.color565(100, 100, 100), bg);
    tft.setCursor(10, iy + 4);
    tft.print(labels[i]);

    char vbuf[32] = "";
    switch(i)
    {
        case 0:
        {
            float db = gainToDb(c.gain);
            if (db >= 0)
                snprintf(vbuf, sizeof(vbuf), "+%.0f dB", db);
            else
                snprintf(vbuf, sizeof(vbuf), "%.0f dB", db);
            tft.setTextColor(TFT_WHITE, bg);
            break;
        }
        case 1:
            snprintf(vbuf, sizeof(vbuf), c.phantom ? "ON" : "OFF");
            tft.setTextColor(c.phantom ? tft.color565(255,200,0) : tft.color565(70,70,70), bg);
            break;
        case 2:
            snprintf(vbuf, sizeof(vbuf), c.polarity ? "INV" : "NORM");
            tft.setTextColor(c.polarity ? tft.color565(240,100,0) : tft.color565(70,70,70), bg);
            break;
        case 3:
            if (c.stereoLinked) {
                int p = mixerStereoPartner(ch);
                snprintf(vbuf, sizeof(vbuf), "CH%d&%d", min(ch,p), max(ch,p));
                tft.setTextColor(tft.color565(100,220,100), bg);
            } else {
                snprintf(vbuf, sizeof(vbuf), "MONO");
                tft.setTextColor(tft.color565(70,70,70), bg);
            }
            break;
        case 4:
            if (c.insOn && c.insMode != INS_OFF) {
                snprintf(vbuf, sizeof(vbuf), "FX%d", (int)c.insMode);
                tft.setTextColor(tft.color565(180,60,255), bg);
            } else {
                snprintf(vbuf, sizeof(vbuf), "OFF");
                tft.setTextColor(tft.color565(70,70,70), bg);
            }
            break;
        case 5:
            snprintf(vbuf, sizeof(vbuf), strlen(c.name) > 0 ? c.name : "---");
            tft.setTextColor(tft.color565(160,160,160), bg);
            break;
    }

    int vw = strlen(vbuf) * 6;
    tft.setCursor(CONTENT_W - vw - 8, iy + 4);
    tft.print(vbuf);

    // Trennlinie
    if (i < CFG_ITEM_COUNT - 1)
        tft.drawFastHLine(4, iy + CFG_ITEM_H, CONTENT_W - 8, tft.color565(30,30,30));

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================

void drawChannelConfig()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    int ch = ui.channel;
    ChannelState& c = channels[ch];

    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    // Header
    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(0, 180, 255), tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print("CHANNEL");
    tft.setTextColor(TFT_WHITE, tft.color565(20, 20, 20));
    char hbuf[24];
    snprintf(hbuf, sizeof(hbuf), " %02d", ch);
    tft.print(hbuf);
    if (strlen(c.name) > 0)
    {
        tft.setTextColor(tft.color565(80, 80, 80), tft.color565(20, 20, 20));
        tft.print("  ");
        tft.print(c.name);
    }
    if (c.stereoLinked)
    {
        int partner = mixerStereoPartner(ch);
        tft.setTextColor(tft.color565(255, 180, 0), tft.color565(20, 20, 20));
        char lbuf[8];
        snprintf(lbuf, sizeof(lbuf), " [%d&%d]", min(ch,partner), max(ch,partner));
        tft.print(lbuf);
    }

    // Alle Items zeichnen
    for (int i = 0; i < CFG_ITEM_COUNT; i++)
        drawChannelConfigItem(i);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// NAME EDITOR POPUP
// ==========================
void drawNameEditor()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    const int w = CONTENT_W - 16, h = 100;
    int x = 8;
    int y = (tft.height() - h) / 2;
    uint16_t bg = tft.color565(22, 22, 22);

    tft.fillRect(x + 2, y + 2, w, h, tft.color565(8, 8, 8));
    tft.fillRect(x, y, w, h, bg);
    tft.drawRect(x, y, w, h, tft.color565(130, 130, 130));

    // Header – kurz und passend
    tft.fillRect(x, y, w, 16, tft.color565(40, 40, 40));
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, tft.color565(40, 40, 40));
    tft.setCursor(x + 6, y + 4);
    tft.print("NAME EDITOR");

    // Name-Zeile mit Cursor (textSize=2 → jedes Zeichen 12px breit)
    tft.setTextSize(2);
    tft.setCursor(x + 6, y + 22);

    char display[17] = "                ";
    int  len = strlen(ui.nameEdit);
    for (int k = 0; k < 16 && k < len; k++) display[k] = ui.nameEdit[k];
    display[16] = '\0';

    for (int k = 0; k < 16; k++)
    {
        if (k == ui.nameCursorPos)
            tft.setTextColor(TFT_BLACK, tft.color565(0, 180, 255));
        else
            tft.setTextColor(TFT_WHITE, bg);
        char c[2] = { display[k], '\0' };
        tft.print(c);
    }

    // Positions-Info
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(80, 80, 80), bg);
    tft.setCursor(x + 6, y + 55);
    char posBuf[24];
    snprintf(posBuf, sizeof(posBuf), "Pos %d/16", ui.nameCursorPos + 1);
    tft.print(posBuf);

    // Steuerung
    tft.setTextColor(tft.color565(0, 120, 180), bg);
    tft.setCursor(x + 6, y + 70);
    tft.print("ENC: Buchstabe");
    tft.setCursor(x + 6, y + 82);
    tft.print("BTN: bestatigen  RTN: abbruch");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// FX INSERT POPUP
// ==========================
void drawFXPopup()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    const int w = 200, h = 110;
    int x = (CONTENT_W - w) / 2;
    int y = (tft.height() - h) / 2;

    tft.fillRect(x + 2, y + 2, w, h, tft.color565(8, 8, 8));
    tft.fillRect(x, y, w, h, tft.color565(22, 22, 22));
    tft.drawRect(x, y, w, h, tft.color565(180, 60, 255));

    tft.fillRect(x, y, w, 16, tft.color565(50, 15, 80));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(180, 60, 255), tft.color565(50, 15, 80));
    tft.setCursor(x + 6, y + 4);
    tft.print("FX INSERT");

    const char* opts[] = { "OFF  (no insert)", "FX1  (Insert FX 1)", "FX2  (Insert FX 2)",
                            "FX3  (Insert FX 3)", "FX4  (Insert FX 4)" };

    for (int i = 0; i < 5; i++)
    {
        int iy  = y + 18 + i * 17;
        bool sel = (i == ui.fxSelection);

        if (sel)
        {
            tft.fillRect(x + 2, iy, w - 4, 16, tft.color565(50, 15, 80));
            tft.drawRect(x + 2, iy, w - 4, 16, tft.color565(180, 60, 255));
            tft.setTextColor(TFT_WHITE, tft.color565(50, 15, 80));
        }
        else
        {
            tft.fillRect(x + 2, iy, w - 4, 16, tft.color565(22, 22, 22));
            tft.setTextColor(tft.color565(90, 90, 90), tft.color565(22, 22, 22));
        }
        tft.setCursor(x + 8, iy + 3);
        tft.print(opts[i]);
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}



// Nur EQ-Header (Leiste oben)
void drawEQHeader()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;

    tft.fillRect(0, 20, CONTENT_W, 18, tft.color565(20, 20, 20));
    tft.drawFastHLine(0, 38, CONTENT_W, tft.color565(10, 10, 10));

    tft.setTextSize(1);
    tft.setTextColor(tft.color565(40, 210, 80), tft.color565(20, 20, 20));
    tft.setCursor(8, 24);
    tft.print("EQ");

    tft.setTextColor(TFT_WHITE, tft.color565(20, 20, 20));
    char buf[16];
    snprintf(buf, sizeof(buf), "CH %02d", ui.channel);
    tft.setCursor(30, 24);
    tft.print(buf);

    const char* chName = channels[ui.channel].name;
    if (strlen(chName) > 0)
    {
        tft.setTextColor(tft.color565(80, 80, 80), tft.color565(20, 20, 20));
        tft.setCursor(80, 24);
        tft.print(chName);
    }

    bool eqOn = channels[ui.channel].eqOn;
    int pillX = CONTENT_W - 26;
    int pillY = 22;
    int pillW = 22;
    int pillH = 11;
    uint16_t pillBg = eqOn ? tft.color565(20, 80, 30) : tft.color565(35, 35, 35);
    uint16_t pillFg = eqOn ? tft.color565(40, 210, 80) : tft.color565(70, 70, 70);
    tft.fillRect(pillX + 1, pillY, pillW - 2, pillH, pillBg);
    tft.fillRect(pillX, pillY + 1, pillW, pillH - 2, pillBg);
    tft.setTextColor(pillFg, pillBg);
    tft.setCursor(pillX + 3, pillY + 2);
    tft.print(eqOn ? "ON" : "OF");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// Nur EQ-Tabs unten + Hint-Text
void drawEQTabs()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    const char* labels[] = { "HPF", "B1", "B2", "B3", "B4" };
    uint16_t bgColors[5] = {
        tft.color565(160, 160, 160),
        EQ_COLOR_BAND1,
        EQ_COLOR_BAND2,
        EQ_COLOR_BAND3,
        EQ_COLOR_BAND4
    };

    int tabY = tft.height() - 20;
    tft.fillRect(0, tabY - 2, CONTENT_W, 22, tft.color565(10, 10, 10));

    ChannelState& c = channels[ui.channel];
    int ch = ui.channel;

    tft.setTextSize(1);

    for (int i = 0; i < 5; i++)
    {
        int tabMargin = 4;
        int tabX = tabMargin + i * ((CONTENT_W - tabMargin * 2) / 5);
        int tabW = (CONTENT_W - tabMargin * 2) / 5 - 1;

        uint16_t bandBg = bgColors[i];

        bool dimmed = false;
        if (i == 0 && !c.hpfOn)
            dimmed = true;
        else if (i > 0 && (!c.eqOn || bandDisabled[ch][i]))
            dimmed = true;

        if (dimmed)
            bandBg = tft.color565(50, 50, 50);

        bool active = (i == ui.eqBand);

        if (active)
        {
            tft.fillRect(tabX, tabY - 2, tabW, 19, bandBg);
            tft.setTextColor(dimmed ? tft.color565(120, 120, 120) : TFT_BLACK, bandBg);
        }
        else
        {
            tft.fillRect(tabX, tabY - 2, tabW, 19, tft.color565(20, 20, 20));
            tft.drawRect(tabX, tabY - 2, tabW, 19, bandBg);
            tft.setTextColor(bandBg, tft.color565(20, 20, 20));
        }

        int lw = strlen(labels[i]) * 6;
        tft.setCursor(tabX + (tabW - lw) / 2, tabY + 3);
        tft.print(labels[i]);

        if (i > 0 && bandDisabled[ch][i])
        {
            tft.setTextColor(tft.color565(200, 60, 60), active ? bandBg : tft.color565(20, 20, 20));
            tft.setCursor(tabX + tabW - 8, tabY - 1);
            tft.print("x");
        }
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// Nur EQ-Kurven neuzeichnen (Plotbereich)
void drawEQPlot()
{
    displayDrawEq(ui.channel);
}

// Vollständiger EQ-Aufbau (nur bei Seitenwechsel oder Band-Wechsel)
void drawEQ()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;

    // Nur den Bereich zwischen Header und Tabs löschen (Plot-Bereich)
    tft.fillRect(0, 38, CONTENT_W, tft.height() - 38 - 33, TFT_BLACK);

    drawEQHeader();
    drawEQPlot();
    drawEQTabs();
}

// ==========================
// EQ TYP POPUP
// ==========================

// Layout-Konstanten für EQ Type Popup
static int eqTypePopupX = 0;
static int eqTypePopupY = 0;
static const int eqTypePopupW = 170;
static const int eqTypePopupH = 120;

static void drawEQTypePopupItem(int i)
{
    const char* types[] = { "LCUT", "LSHV", "PEQ", "VEQ", "HSHV", "HCUT" };
    const char* typeDesc[] = {
        "Low Cut Filter", "Low Shelf", "Parametric EQ",
        "Vintage EQ", "High Shelf", "High Cut Filter"
    };

    int x = eqTypePopupX;
    int y = eqTypePopupY;
    int w = eqTypePopupW;

    int yy  = y + 18 + i * 16;
    bool sel = (i == ui.eqTypeSelection);

    uint16_t popupBg = tft.color565(22, 22, 22);

    if (sel)
    {
        tft.fillRect(x + 2, yy, w - 4, 15, tft.color565(20, 80, 30));
        tft.drawRect(x + 2, yy, w - 4, 15, tft.color565(40, 210, 80));
        tft.setTextColor(TFT_WHITE, tft.color565(20, 80, 30));
    }
    else
    {
        tft.fillRect(x + 2, yy, w - 4, 15, popupBg);
        tft.setTextColor(tft.color565(100, 100, 100), popupBg);
    }

    tft.setCursor(x + 8, yy + 3);
    tft.print(types[i]);

    if (sel)
    {
        tft.setTextColor(tft.color565(40, 210, 80), tft.color565(20, 80, 30));
        tft.setCursor(x + 46, yy + 3);
        tft.print(typeDesc[i]);
    }
}

void drawEQTypePopup()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    int x = (CONTENT_W - eqTypePopupW) / 2;
    int y = (tft.height() - eqTypePopupH) / 2;

    // Cache Position für drawEQTypePopupItem
    eqTypePopupX = x;
    eqTypePopupY = y;

    int w = eqTypePopupW;
    int h = eqTypePopupH;

    tft.fillRect(x + 3, y + 3, w, h, tft.color565(10, 10, 10));
    tft.fillRect(x, y, w, h, tft.color565(22, 22, 22));
    tft.drawRect(x, y, w, h, tft.color565(40, 210, 80));

    tft.fillRect(x, y, w, 16, tft.color565(20, 80, 30));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(40, 210, 80), tft.color565(20, 80, 30));
    tft.setCursor(x + 6, y + 4);
    tft.print("EQ BAND TYPE");

    for (int i = 0; i < 6; i++)
        drawEQTypePopupItem(i);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// DYNAMICS (COMPRESSOR + GATE)
// ==========================
void drawDynamics()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    // Header
    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(240, 130, 0), tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print("DYNAMICS");
    tft.setTextColor(TFT_WHITE, tft.color565(20, 20, 20));
    char buf[16];
    snprintf(buf, sizeof(buf), " CH %02d", ui.channel);
    tft.print(buf);

    ChannelState& c = channels[ui.channel];

    // ---- COMPRESSOR ----
    int cy = 42;
    tft.fillRect(4, cy, CONTENT_W - 8, 82, tft.color565(18, 18, 18));
    tft.drawRect(4, cy, CONTENT_W - 8, 82, tft.color565(240, 130, 0));
    tft.fillRect(4, cy, CONTENT_W - 8, 14, tft.color565(80, 40, 0));
    tft.setTextColor(tft.color565(240, 130, 0), tft.color565(80, 40, 0));
    tft.setCursor(8, cy + 3);
    tft.print("COMPRESSOR");

    auto drawParam = [&](int px, int py, const char* name, float val, const char* unit)
    {
        tft.setTextColor(tft.color565(100, 100, 100), tft.color565(18, 18, 18));
        tft.setCursor(px, py);
        tft.print(name);
        tft.setTextColor(TFT_WHITE, tft.color565(18, 18, 18));
        char vbuf[16];
        snprintf(vbuf, sizeof(vbuf), "%.1f%s", val, unit);
        tft.setCursor(px, py + 10);
        tft.print(vbuf);
    };

    // Threshold, Ratio, Attack, Release
    float thr  = -60.0f + c.comp.threshold * 60.0f;
    float rat  = 1.0f + c.comp.ratio * 9.0f;
    float atk  = c.comp.attack  * 200.0f;
    float rel  = c.comp.release * 800.0f;

    drawParam(10,  cy + 18, "THR",  thr, "dB");
    drawParam(62,  cy + 18, "RATIO", rat, ":1");
    drawParam(120, cy + 18, "ATK",  atk, "ms");
    drawParam(175, cy + 18, "REL",  rel, "ms");

    // Mini Gain-Reduction Balken
    tft.drawFastHLine(8, cy + 56, CONTENT_W - 16, tft.color565(50, 50, 50));
    tft.setTextColor(tft.color565(70, 70, 70), tft.color565(18, 18, 18));
    tft.setCursor(8, cy + 60);
    tft.print("GR");
    int grBar = (int)(meters[0] * (CONTENT_W - 30));
    tft.fillRect(24, cy + 60, CONTENT_W - 30, 8, tft.color565(15, 15, 15));
    if (grBar > 0)
        tft.fillRect(24, cy + 60, grBar, 8, tft.color565(240, 130, 0));

    // ---- GATE ----
    int gy = cy + 90;
    tft.fillRect(4, gy, CONTENT_W - 8, 52, tft.color565(18, 18, 18));
    tft.drawRect(4, gy, CONTENT_W - 8, 52, tft.color565(180, 60, 255));
    tft.fillRect(4, gy, CONTENT_W - 8, 14, tft.color565(50, 15, 80));
    tft.setTextColor(tft.color565(180, 60, 255), tft.color565(50, 15, 80));
    tft.setCursor(8, gy + 3);
    tft.print("GATE");

    float gThr = -80.0f + c.gate.threshold * 80.0f;
    drawParam(10, gy + 18, "THR", gThr, "dB");

    tft.setTextColor(tft.color565(60, 60, 60), tft.color565(18, 18, 18));
    tft.setCursor(8, gy + 36);
    tft.print("Use Faders: Threshold");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// SENDS VIEW
// ==========================
void drawSends()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(180, 60, 255), tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print("SENDS");
    tft.setTextColor(TFT_WHITE, tft.color565(20, 20, 20));
    char buf[16];
    snprintf(buf, sizeof(buf), " CH %02d", ui.channel);
    tft.print(buf);

    ChannelState& c = channels[ui.channel];

    // Pro Bus ein Balken
    int busCount = constrain(mixerMaxBuses, 1, 6);
    int barW     = (CONTENT_W - 16) / busCount - 4;
    int barH     = tft.height() - 60;

    for (int b = 1; b <= busCount; b++)
    {
        int bx  = 8 + (b - 1) * (barW + 4);
        int by  = 38;
        float v = (b < MAX_BUSES) ? c.sends[b] : 0.0f;

        // Rahmen
        tft.fillRect(bx, by, barW, barH, tft.color565(15, 15, 15));
        tft.drawRect(bx, by, barW, barH, tft.color565(50, 50, 50));

        // Füllstand
        int filled = (int)(v * barH);
        tft.fillRect(bx + 1, by + barH - filled, barW - 2, filled,
                     tft.color565(120, 40, 200));

        // Label
        char lbuf[4];
        snprintf(lbuf, sizeof(lbuf), "A%d", b);
        tft.setTextColor(tft.color565(120, 120, 120), TFT_BLACK);
        tft.setCursor(bx + (barW - 12) / 2, by + barH + 3);
        tft.print(lbuf);

        // dB-Wert
        float db = (v > 0.001f) ? 20.0f * log10f(v) : -99.0f;
        tft.setTextColor(tft.color565(180, 60, 255), TFT_BLACK);
        tft.setCursor(bx, by + barH + 13);
        if (db < -50.0f) tft.print("-inf");
        else { char dbuf[8]; snprintf(dbuf, sizeof(dbuf), "%.0f", db); tft.print(dbuf); }
    }

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// ROUTING (Platzhalter mit Hinweis)
// ==========================
void drawInfoPage(const char* title, uint16_t accentCol, const char* line1, const char* line2)
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(accentCol, tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print(title);

    // Zentrierter Inhalt
    int midX = CONTENT_W / 2;
    int midY = tft.height() / 2;

    // Icon-Kreis
    tft.drawCircle(midX, midY - 20, 22, accentCol);
    tft.setTextColor(accentCol, TFT_BLACK);
    tft.setCursor(midX - 3, midY - 24);
    tft.print("i");

    tft.setTextColor(tft.color565(120, 120, 120), TFT_BLACK);
    int lw1 = strlen(line1) * 6;
    tft.setCursor(midX - lw1 / 2, midY + 8);
    tft.print(line1);

    tft.setTextColor(tft.color565(60, 60, 60), TFT_BLACK);
    int lw2 = strlen(line2) * 6;
    tft.setCursor(midX - lw2 / 2, midY + 22);
    tft.print(line2);

    // Return-Hinweis
    tft.setTextColor(tft.color565(45, 45, 45), TFT_BLACK);
    tft.setCursor(midX - 36, tft.height() - 16);
    tft.print("RETURN = back");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// NETWORK
// ==========================
void drawNetwork()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(255, 80, 80), tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print("NETWORK");

    auto drawRow = [&](int y, const char* label, const char* value, uint16_t valCol)
    {
        tft.setTextColor(tft.color565(70, 70, 70), TFT_BLACK);
        tft.setCursor(8, y);
        tft.print(label);
        tft.setTextColor(valCol, TFT_BLACK);
        tft.setCursor(80, y);
        tft.print(value);
        tft.drawFastHLine(8, y + 10, CONTENT_W - 16, tft.color565(25, 25, 25));
    };

    // WiFi Status
    bool wifiOk = wifiConnected();
    drawRow(42,  "WiFi",    wifiOk ? "Connected"    : "Disconnected",
            wifiOk ? tft.color565(40, 210, 80) : tft.color565(255, 80, 80));

    // IP
    char ipBuf[24] = "---";
    if (wifiOk) WiFi.localIP().toString().toCharArray(ipBuf, sizeof(ipBuf));
    drawRow(56,  "IP",      ipBuf, tft.color565(180, 180, 180));

    // XAir
    bool xairOk = isSystemReady();
    drawRow(70,  "XAir",   xairOk ? mixerModel : "Not found",
            xairOk ? tft.color565(40, 210, 80) : tft.color565(255, 80, 80));

    // Mixer Name
    drawRow(84,  "Name",   strlen(mixerName) > 0 ? mixerName : "---",
            tft.color565(180, 180, 180));

    // SSID
    char ssid[32] = "---";
    if (wifiOk) WiFi.SSID().toCharArray(ssid, sizeof(ssid));
    drawRow(98,  "SSID",   ssid,  tft.color565(180, 180, 180));

    // RSSI
    char rssiBuf[12] = "---";
    if (wifiOk) snprintf(rssiBuf, sizeof(rssiBuf), "%d dBm", WiFi.RSSI());
    drawRow(112, "RSSI",   rssiBuf, tft.color565(180, 180, 180));

    // OSC Port
    char portBuf[12];
    snprintf(portBuf, sizeof(portBuf), "%d", settings.sendPort);
    drawRow(126, "OSC Port", portBuf, tft.color565(180, 180, 180));

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// SETTINGS
// ==========================
void drawSettings()
{
    const int CONTENT_W = tft.width() - VU_WIDTH;
    tft.fillRect(0, 20, CONTENT_W, tft.height() - 20, TFT_BLACK);

    tft.fillRect(0, 20, CONTENT_W, 16, tft.color565(20, 20, 20));
    tft.setTextSize(1);
    tft.setTextColor(tft.color565(130, 130, 130), tft.color565(20, 20, 20));
    tft.setCursor(5, 24);
    tft.print("SETTINGS");

    const char* items[] = {
        "Brightness",
        "XAir IP",
        "OSC Port",
        "WiFi SSID",
        "WiFi Pass",
        "Calibrate Faders",
        "Factory Reset"
    };
    const char* hints[] = {
        "Display backlight",
        "Target mixer IP",
        "Default: 10024",
        "Network name",
        "WPA key",
        "Move faders to limits",
        "Clear all settings"
    };

    for (int i = 0; i < 7; i++)
    {
        int iy = 40 + i * 26;
        tft.fillRect(4, iy, CONTENT_W - 8, 24, tft.color565(18, 18, 18));
        tft.drawRect(4, iy, CONTENT_W - 8, 24, tft.color565(35, 35, 35));

        tft.setTextColor(TFT_WHITE, tft.color565(18, 18, 18));
        tft.setCursor(10, iy + 4);
        tft.print(items[i]);

        tft.setTextColor(tft.color565(60, 60, 60), tft.color565(18, 18, 18));
        tft.setCursor(10, iy + 14);
        tft.print(hints[i]);
    }

    tft.setTextColor(tft.color565(50, 50, 50), TFT_BLACK);
    tft.setCursor(8, tft.height() - 12);
    tft.print("Use WebUI for full config");

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

// ==========================
// LOOP
// ==========================
void menuLoop()
{
    handleInput();
    processFaders();

    // ---- EQ-Seite: differenziertes Redraw ----
    if (ui.page == PAGE_EQ || ui.page == PAGE_EQ_TYPE_POPUP)
    {
        if (ui.redraw)
        {
            // Vollständiger Neuaufbau (Seitenwechsel, Band-Wechsel, Button)
            drawEQ();
            if (ui.page == PAGE_EQ_TYPE_POPUP)
                drawEQTypePopup();
            ui.redraw  = false;
            needRedraw = false;
        }
        else if (needRedraw)
        {
            // Nur EQ-Kurven neuzeichnen (Fader bewegt → Parameter geändert)
            drawEQPlot();
            needRedraw = false;
        }
    }
    // ---- Alle anderen Seiten: wie bisher ----
    else if (ui.redraw || needRedraw)
    {
        switch (ui.page)
        {
            case PAGE_MAIN:
                drawMenu();
                break;
            case PAGE_CHANNEL_POPUP:
                drawMenu();
                drawChannelPopup();
                break;
            case PAGE_CHANNEL_CONFIG:
                drawChannelConfig();
                break;
            case PAGE_NAME_EDITOR:
                drawChannelConfig();   // Hintergrund
                drawNameEditor();      // Popup darüber
                break;
            case PAGE_FX_POPUP:
                drawChannelConfig();   // Hintergrund
                drawFXPopup();         // Popup darüber
                break;
            case PAGE_COMPRESSOR:
            case PAGE_GATE:
                drawDynamics();
                break;
            case PAGE_SENDS:
                drawSends();
                break;
            case PAGE_ROUTING:
                drawInfoPage("ROUTING", tft.color565(0, 200, 180),
                             "Coming soon", "Use WebUI for routing");
                break;
            case PAGE_SCENES:
                drawInfoPage("SCENES", tft.color565(255, 200, 0),
                             "Snapshot support", "Coming soon");
                break;
            case PAGE_NETWORK:
                drawNetwork();
                break;
            case PAGE_SETTINGS:
                drawSettings();
                break;
            default:
                drawMenu();
                break;
        }

        ui.redraw  = false;
        needRedraw = false;
    }

    updateReturnLED();

    displayDrawStatusBar();
    displayDrawStereoVUMeter(0, 1);

    static uint32_t miniTimer = 0;
    if (needMiniRedraw || millis() - miniTimer > 80)
    {
        updateMiniDisplays();
        needMiniRedraw = false;
        miniTimer = millis();
    }
}

// ==========================
// INIT
// ==========================
void menuBegin()
{
    // EQ Band Memory initialisieren
    for (int ch = 0; ch < MAX_CHANNELS; ch++)
    {
        for (int b = 0; b < MAX_EQ_BANDS; b++)
        {
            savedBandGain[ch][b] = 0.5f;  // 0dB default
            bandDisabled[ch][b]  = false;
        }
    }

    syncFadersToMixer();
    updateMiniDisplays();
    ui.redraw = true;
}
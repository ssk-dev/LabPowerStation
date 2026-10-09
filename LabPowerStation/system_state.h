#pragma once
#include <stdint.h>

/** Logical channels, independent of ADC wiring. */
enum PowerId { POWER_3V3, POWER_5V, POWER_12V, POWER_VARIABLE, POWER_COUNT };
enum OutputMode { OUTPUT_OFF, OUTPUT_CV, OUTPUT_CC, OUTPUT_FAULT };
enum class Fault { None, OVP, OCP, OTP, ShortCircuit, ADC, Sensor, Fan, Communication };
enum class MeterMode { DCV, ACV, DCA, ACA, Resistance, Continuity, Diode, Capacitance, Frequency };

struct PowerChannel {
    float setVoltage = 0;
    float voltage = 0;
    float current = 0;
    float power = 0;
    float energy = 0; // Wh, integrated using actual elapsed time
    float currentLimit = 2;
    bool enabled = false;
    OutputMode mode = OUTPUT_OFF;
};

struct MeasurementData {
    float voltage = 0;
    float current = 0;
    float power = 0;
    float energy = 0;
    float temperature = 0;
    uint32_t timestamp = 0;
};

struct MeterState {
    MeterMode mode = MeterMode::DCV;
    float raw = 0;
    float value = 0;
    float minimum = 0;
    float maximum = 0;
    float average = 0;
    float relativeBase = 0;
    uint32_t samples = 0;
    bool hold = false;
    bool relative = false;
    bool autorange = true;
    bool statistics = true;
};

struct Settings {
    unsigned updateMs = 100;
    unsigned contrast = 100; // Software dimming, NOT PWM backlight brightness
    bool night = false;
    bool fanAuto = true;
    unsigned fanManual = 35;
    bool logging = false; // RAM recording only
};

/** Single owner: Arduino loop. No UI or ISR may mutate this from another task. */
struct SystemState {
    PowerChannel channels[POWER_COUNT];
    MeterState meter;
    Settings settings;
    MeasurementData latest;
    Fault fault = Fault::None;
    float faultValue = 0;
    float faultLimit = 0;
    float inputPower = 0;
    float outputPower = 0;
    float efficiency = 0;
    float temperature = 0;
    unsigned fanPercent = 0;
    uint32_t uptimeMs = 0;
    uint32_t flashBytes = 0;
    uint32_t psramBytes = 0;
    uint32_t freeHeap = 0;
    uint32_t freePsram = 0;
    bool boardReady = false;
    bool touchConfirmed = false;
    bool simulation = true;
    bool wifiConnected = false;
    bool sdMounted = false;
    float sdUsedGB = 0;
    float sdTotalGB = 0;
    bool exportRequested = false;
    bool restartRequested = false;
    bool settingsDirty = false; // Set by the UI when a persisted Settings field changes.
};

/** Reset values, with all outputs OFF; does not touch any hardware. */
void initializeState(SystemState &state);
const char *faultName(Fault fault);
const char *channelName(PowerId channel);
const char *modeName(OutputMode mode);

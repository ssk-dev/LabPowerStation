#include "system_state.h"
#include "config.h"

void initializeState(SystemState &state)
{
    state = SystemState{};
    state.simulation = SIMULATION_MODE;
    const float voltages[] = {3.3f, 5.0f, 12.0f, 24.0f};
    for (unsigned i = 0; i < POWER_COUNT; ++i) {
        state.channels[i].setVoltage = voltages[i];
        state.channels[i].currentLimit = i == POWER_VARIABLE ? 5.0f : 2.0f;
    }
}

const char *faultName(Fault fault)
{
    switch (fault) {
        case Fault::None: return "OK";
        case Fault::OVP: return "OVER VOLTAGE";
        case Fault::OCP: return "OVER CURRENT";
        case Fault::OTP: return "OVER TEMPERATURE";
        case Fault::ShortCircuit: return "SHORT CIRCUIT";
        case Fault::ADC: return "ADC ERROR";
        case Fault::Sensor: return "SENSOR ERROR";
        case Fault::Fan: return "FAN ERROR";
        case Fault::Communication: return "COMMUNICATION ERROR";
    }
    return "UNKNOWN";
}

const char *channelName(PowerId channel)
{
    static const char *names[] = {"3.3 V", "5 V", "12 V", "VARIABLE"};
    return channel >= POWER_3V3 && channel < POWER_COUNT ? names[channel] : "INVALID";
}

const char *modeName(OutputMode mode)
{
    switch (mode) {
        case OUTPUT_CV: return "CV";
        case OUTPUT_CC: return "CC";
        case OUTPUT_FAULT: return "FAULT";
        default: return "OFF";
    }
}

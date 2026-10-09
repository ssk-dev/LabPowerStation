#include "power_supply.h"
#include "config.h"
#include <math.h>

bool PowerSupplyManager::setEnabled(PowerId channel, bool enabled)
{
    if (channel < POWER_3V3 || channel >= POWER_COUNT) return false;
    if (enabled && (!state_.simulation || state_.fault != Fault::None)) return false;
    auto &output = state_.channels[channel];
    output.enabled = enabled;
    output.mode = enabled ? OUTPUT_CV : OUTPUT_OFF;
    if (!enabled) output.voltage = output.current = output.power = 0;
    return true;
}

bool PowerSupplyManager::setVoltage(float voltage)
{
    if (!isfinite(voltage) || voltage < 0 || voltage > Config::maxVoltage) return false;
    state_.channels[POWER_VARIABLE].setVoltage = voltage;
    return true;
}

bool PowerSupplyManager::setCurrentLimit(float current)
{
    if (!isfinite(current) || current < 0 || current > Config::maxCurrent) return false;
    state_.channels[POWER_VARIABLE].currentLimit = current;
    return true;
}

void PowerSupplyManager::allOff()
{
    for (unsigned i = 0; i < POWER_COUNT; ++i) setEnabled(static_cast<PowerId>(i), false);
}

void PowerSupplyManager::trip(Fault fault, float value, float limit)
{
    if (fault == Fault::None) return;
    allOff();
    state_.fault = fault;
    state_.faultValue = value;
    state_.faultLimit = limit;
    for (auto &channel : state_.channels) channel.mode = OUTPUT_FAULT;
}

bool PowerSupplyManager::acknowledge()
{
    if (!state_.simulation) return false;
    state_.fault = Fault::None;
    state_.faultValue = state_.faultLimit = 0;
    allOff();
    return true;
}

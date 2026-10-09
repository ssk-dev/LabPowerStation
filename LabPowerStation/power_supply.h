#pragma once
#include "system_state.h"

/** Simulation command boundary. Never writes GPIOs or claims hardware protection. */
class PowerSupplyManager {
public:
    explicit PowerSupplyManager(SystemState &state) : state_(state) {}
    /** Reject enable when a fault is latched or physical drivers are absent. */
    bool setEnabled(PowerId channel, bool enabled);
    /** Accept finite values within the documented simulation range only. */
    bool setVoltage(float voltage);
    bool setCurrentLimit(float current);
    void allOff();
    /** Latch a fault and synchronously disable all simulated channels. */
    void trip(Fault fault, float value = 0, float limit = 0);
    /** Clear simulated faults without re-enabling outputs. Real mode stays locked. */
    bool acknowledge();
    const PowerChannel &getChannel(PowerId channel) const { return state_.channels[channel]; }
private:
    SystemState &state_;
};

#pragma once
#include "system_state.h"
#include "config.h"

/** Replace this interface with a validated ADC backend, not UI-side sensor code. */
class MeasurementSource {
public:
    virtual ~MeasurementSource() = default;
    virtual bool sample(SystemState &state, uint32_t now, float elapsedSeconds) = 0;
};

/** DEMO ONLY: resistive loads and time-varying readings, no electrical measurement. */
class SimulationSource final : public MeasurementSource {
public:
    bool sample(SystemState &state, uint32_t now, float elapsedSeconds) override;
};

/** Sampling, metering statistics, energy integration and bounded 30-minute history. */
class MeasurementManager {
public:
    MeasurementManager(SystemState &state, MeasurementSource &source) : state_(state), source_(source) {}
    bool begin();
    void update(uint32_t now);
    void selectMeter(MeterMode mode);
    void resetMeterStatistics();
    void toggleRelative();
    /** Returns a sample at age seconds (0=newest); false before enough data exists. */
    bool history(unsigned age, MeasurementData &result) const;
    unsigned historyCount() const { return count_; }
private:
    SystemState &state_;
    MeasurementSource &source_;
    MeasurementData *history_ = nullptr;
    unsigned head_ = 0;
    unsigned count_ = 0;
    uint32_t lastSample_ = 0;
    uint32_t lastHistory_ = 0;
};

const char *meterName(MeterMode mode);
const char *meterUnit(MeterMode mode);
const char *meterRange(MeterMode mode, bool automatic);

#include "measurement.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <math.h>

bool SimulationSource::sample(SystemState &state, uint32_t now, float seconds)
{
    if (!state.simulation) return false;
    const float phase = (now % 60000) * 0.001f;
    const float resistances[] = {4.0f, 3.5f, 8.0f, 11.2f};
    state.outputPower = 0;
    for (unsigned i = 0; i < POWER_COUNT; ++i) {
        auto &channel = state.channels[i];
        if (channel.enabled && state.fault == Fault::None) {
            float target = channel.setVoltage * (1.0f + 0.001f * sinf(phase + i));
            float demand = target / resistances[i];
            channel.current = fminf(demand, channel.currentLimit);
            channel.voltage = demand > channel.currentLimit ? channel.current * resistances[i] : target;
            channel.mode = demand > channel.currentLimit ? OUTPUT_CC : OUTPUT_CV;
        } else {
            channel.current = channel.voltage = 0;
            channel.mode = state.fault == Fault::None ? OUTPUT_OFF : OUTPUT_FAULT;
        }
        channel.power = channel.voltage * channel.current;
        channel.energy += channel.power * seconds / 3600.0f;
        state.outputPower += channel.power;
    }
    state.inputPower = state.outputPower / 0.91f + 2.4f;
    state.efficiency = state.outputPower * 100.0f / state.inputPower;
    state.temperature = 32.0f + state.outputPower * 0.08f + sinf(phase * 0.1f);
    auto &variable = state.channels[POWER_VARIABLE];
    state.latest = {variable.voltage, variable.current, variable.power, variable.energy, state.temperature, now};
    // Dedicated multimeter demo source, unrelated to connected terminals.
    const float base[] = {24.382f, 12.04f, 1.425f, 0.825f, 4700.0f, 0.24f, 0.681f, 100.0f, 1000.0f};
    auto &meter = state.meter;
    meter.raw = base[static_cast<unsigned>(meter.mode)] * (1.0f + 0.002f * sinf(phase * 1.7f));
    return true;
}

bool MeasurementManager::begin()
{
    history_ = static_cast<MeasurementData *>(heap_caps_calloc(
        Config::historySize, sizeof(MeasurementData), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    lastSample_ = lastHistory_ = millis();
    return history_ != nullptr;
}

void MeasurementManager::update(uint32_t now)
{
    const uint32_t elapsed = now - lastSample_;
    if (elapsed < state_.settings.updateMs) return;
    lastSample_ = now;
    if (!source_.sample(state_, now, elapsed * 0.001f)) {
        state_.fault = Fault::ADC;
        for (auto &channel : state_.channels) {
            channel.enabled = false;
            channel.voltage = channel.current = channel.power = 0;
            channel.mode = OUTPUT_FAULT;
        }
        return;
    }
    auto &meter = state_.meter;
    if (!meter.hold) {
        meter.value = meter.raw - (meter.relative ? meter.relativeBase : 0);
        if (!meter.samples) meter.minimum = meter.maximum = meter.average = meter.value;
        meter.minimum = fminf(meter.minimum, meter.value);
        meter.maximum = fmaxf(meter.maximum, meter.value);
        // Bound the averaging horizon to avoid counter overflow on long runs.
        if (meter.samples < 1000000) ++meter.samples;
        meter.average += (meter.value - meter.average) / meter.samples;
    }
    if (history_ && now - lastHistory_ >= 1000) {
        lastHistory_ = now;
        history_[head_] = state_.latest;
        head_ = (head_ + 1) % Config::historySize;
        if (count_ < Config::historySize) ++count_;
    }
}

void MeasurementManager::selectMeter(MeterMode mode)
{
    state_.meter = MeterState{};
    state_.meter.mode = mode;
}

void MeasurementManager::resetMeterStatistics()
{
    state_.meter.samples = 0;
}

void MeasurementManager::toggleRelative()
{
    auto &meter = state_.meter;
    meter.relative = !meter.relative;
    meter.relativeBase = meter.relative ? meter.raw : 0;
    meter.hold = false;
    resetMeterStatistics();
}

bool MeasurementManager::history(unsigned age, MeasurementData &result) const
{
    if (!history_ || age >= count_) return false;
    result = history_[(head_ + Config::historySize - 1 - age) % Config::historySize];
    return true;
}

const char *meterName(MeterMode mode)
{
    static const char *names[] = {"DC VOLTAGE", "AC VOLTAGE", "DC CURRENT", "AC CURRENT", "RESISTANCE", "CONTINUITY", "DIODE", "CAPACITANCE", "FREQUENCY"};
    return names[static_cast<unsigned>(mode)];
}

const char *meterUnit(MeterMode mode)
{
    static const char *units[] = {"V", "V RMS", "A", "A RMS", "Ohm", "Ohm", "V", "uF", "Hz"};
    return units[static_cast<unsigned>(mode)];
}

const char *meterRange(MeterMode mode, bool automatic)
{
    static const char *ranges[] = {"0-100 V", "0-100 V RMS", "0-10 A", "0-10 A RMS", "0-10 kOhm", "0-50 Ohm", "0-3 V", "0-1000 uF", "0-10 kHz"};
    return automatic ? "AUTO (DEMO)" : ranges[static_cast<unsigned>(mode)];
}

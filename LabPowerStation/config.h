#pragma once

#define SIMULATION_MODE true
#define DEBUG_ENABLED true

namespace Config {
constexpr unsigned width = 800;
constexpr unsigned height = 480;
constexpr unsigned measurementMs = 100;
constexpr unsigned guiMs = 40;
constexpr unsigned statusMs = 500;
constexpr unsigned systemMs = 1000;
constexpr unsigned drawLines = 40;
constexpr unsigned historySize = 1801;
// These are DEMO UI limits, NOT ratings of a real power stage.
constexpr float minVoltage = 12.0f;
constexpr float maxVoltage = 97.0f;
constexpr float maxCurrent = 30.0f;
constexpr const char *firmware = "0.1.0-phase1";
}

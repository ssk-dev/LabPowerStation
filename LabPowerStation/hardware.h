#pragma once
#include <esp_display_panel.hpp>
#include "system_state.h"

/** Owns the real onboard display/touch assembly; no external instrument hardware. */
class HardwareManager {
public:
    /** Validate S3, 16 MB flash and 8 MB PSRAM, then start the manufacturer's board driver. */
    bool begin(SystemState &state);
    void update(SystemState &state);
    esp_panel::board::Board &board() { return board_; }
private:
    esp_panel::board::Board board_;
};

/** Simulated fan percentage only; no PWM pin has been assigned. */
class FanManager {
public:
    void update(SystemState &state);
};

/** Offline service boundary; never invents an SSID/IP or connection status. */
class NetworkManager {
public:
    void update(SystemState &state) { state.wifiConnected = false; }
};

#pragma once
#include "system_state.h"

namespace esp_panel::board { class Board; }

/**
 * Real SD/TF card storage: mounts the card via SPI (MOSI=11, CLK=12, MISO=13)
 * with chip-select routed through the board's existing CH422G IO expander
 * (EXIO4), and persists the user-editable Settings struct as /settings.json.
 *
 * The RAM ring-buffer logger (120 samples) and its nonblocking Serial CSV
 * export are unchanged and still work with or without an SD card present.
 */
class StorageManager {
public:
    /** Mount the card (if present) and load /settings.json into state.settings. Safe to call once no card is found. */
    bool begin(SystemState &state, esp_panel::board::Board &board);
    void update(SystemState &state);
private:
    bool mountCard(SystemState &state, esp_panel::board::Board &board);
    bool loadSettings(SystemState &state);
    bool saveSettings(const SystemState &state);
    void refreshCapacity(SystemState &state);

    static constexpr unsigned capacity_ = 120;
    MeasurementData records_[capacity_] = {};
    unsigned head_ = 0;
    unsigned count_ = 0;
    unsigned exportIndex_ = 0;
    unsigned exportCount_ = 0;
    uint32_t lastRecord_ = 0;
    bool exporting_ = false;
    bool headerSent_ = false;
    bool cardPresent_ = false;
};

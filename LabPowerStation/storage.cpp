#include "storage.h"
#include "config.h"
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <esp_display_panel.hpp>
#include <esp_io_expander.hpp>
#include <cJSON.h>

namespace {
// Confirmed via Waveshare's official 03_SD_Test example and ESP32-S3-Touch-LCD-4.3 pinout docs.
// SD_CS is NOT a raw ESP32 GPIO; it is EXIO4 on the board's own CH422G IO expander.
constexpr int SD_MOSI = 11;
constexpr int SD_CLK = 12;
constexpr int SD_MISO = 13;
constexpr int SD_SS = -1;   // Unused: chip-select is done via the IO expander instead.
constexpr int SD_CS_EXIO = 4;
constexpr const char *SETTINGS_PATH = "/settings.json";
}

bool StorageManager::begin(SystemState &state, esp_panel::board::Board &board)
{
    cardPresent_ = mountCard(state, board);
    state.sdMounted = cardPresent_;
    if (!cardPresent_) {
        state.sdUsedGB = 0;
        state.sdTotalGB = 0;
#if DEBUG_ENABLED
        Serial.println("StorageManager: no SD/TF card found, settings.json persistence disabled.");
#endif
        return false;
    }
    refreshCapacity(state);
    if (!loadSettings(state)) {
#if DEBUG_ENABLED
        Serial.println("StorageManager: no settings.json yet, writing current defaults.");
#endif
        saveSettings(state);
        refreshCapacity(state);
    }
    return true;
}

void StorageManager::refreshCapacity(SystemState &state)
{
    if (!cardPresent_) {
        state.sdUsedGB = 0;
        state.sdTotalGB = 0;
        return;
    }
    // Decimal GB (1e9 bytes), matching the "13.4GB / 15GB" convention printed on card packaging.
    state.sdTotalGB = static_cast<float>(SD.totalBytes() / 1000000000.0);
    state.sdUsedGB = static_cast<float>(SD.usedBytes() / 1000000000.0);
}

bool StorageManager::mountCard(SystemState &state, esp_panel::board::Board &board)
{
    auto expanderDriver = board.getIO_Expander();
    if (!expanderDriver) return false;
    auto expander = static_cast<esp_expander::CH422G *>(expanderDriver->getBase());
    if (!expander) return false;
    // The board already initialized this expander for touch-reset/backlight; just drive EXIO4 low to select the SD card.
    expander->digitalWrite(SD_CS_EXIO, LOW);
    SPI.setHwCs(false);
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_SS);
    if (!SD.begin(SD_SS)) return false;
    return SD.cardType() != CARD_NONE;
}

bool StorageManager::loadSettings(SystemState &state)
{
    File file = SD.open(SETTINGS_PATH, FILE_READ);
    if (!file) return false;
    String text = file.readString();
    file.close();
    cJSON *root = cJSON_Parse(text.c_str());
    if (!root) {
#if DEBUG_ENABLED
        Serial.println("StorageManager: settings.json is not valid JSON, keeping RAM defaults.");
#endif
        return false;
    }
    auto &settings = state.settings;
    cJSON *item;
    if (cJSON *display = cJSON_GetObjectItemCaseSensitive(root, "display")) {
        if ((item = cJSON_GetObjectItemCaseSensitive(display, "contrast")) && cJSON_IsNumber(item)) settings.contrast = static_cast<unsigned>(item->valuedouble);
        if ((item = cJSON_GetObjectItemCaseSensitive(display, "night")) && cJSON_IsBool(item)) settings.night = cJSON_IsTrue(item);
    }
    if (cJSON *measurement = cJSON_GetObjectItemCaseSensitive(root, "measurement")) {
        if ((item = cJSON_GetObjectItemCaseSensitive(measurement, "updateMs")) && cJSON_IsNumber(item)) settings.updateMs = static_cast<unsigned>(item->valuedouble);
    }
    if (cJSON *fan = cJSON_GetObjectItemCaseSensitive(root, "fan")) {
        if ((item = cJSON_GetObjectItemCaseSensitive(fan, "fanAuto")) && cJSON_IsBool(item)) settings.fanAuto = cJSON_IsTrue(item);
        if ((item = cJSON_GetObjectItemCaseSensitive(fan, "fanManual")) && cJSON_IsNumber(item)) settings.fanManual = static_cast<unsigned>(item->valuedouble);
    }
    if (cJSON *logging = cJSON_GetObjectItemCaseSensitive(root, "logging")) {
        if ((item = cJSON_GetObjectItemCaseSensitive(logging, "logging")) && cJSON_IsBool(item)) settings.logging = cJSON_IsTrue(item);
    }
    cJSON_Delete(root);
#if DEBUG_ENABLED
    Serial.println("StorageManager: settings.json loaded from SD card.");
#endif
    return true;
}

bool StorageManager::saveSettings(const SystemState &state)
{
    if (!cardPresent_) return false;
    cJSON *root = cJSON_CreateObject();
    if (!root) return false;
    const auto &settings = state.settings;
    cJSON *display = cJSON_AddObjectToObject(root, "display");
    cJSON_AddNumberToObject(display, "contrast", settings.contrast);
    cJSON_AddBoolToObject(display, "night", settings.night);
    cJSON *measurement = cJSON_AddObjectToObject(root, "measurement");
    cJSON_AddNumberToObject(measurement, "updateMs", settings.updateMs);
    cJSON *fan = cJSON_AddObjectToObject(root, "fan");
    cJSON_AddBoolToObject(fan, "fanAuto", settings.fanAuto);
    cJSON_AddNumberToObject(fan, "fanManual", settings.fanManual);
    cJSON *logging = cJSON_AddObjectToObject(root, "logging");
    cJSON_AddBoolToObject(logging, "logging", settings.logging);
    char *text = cJSON_Print(root);
    cJSON_Delete(root);
    if (!text) return false;
    File file = SD.open(SETTINGS_PATH, FILE_WRITE);
    bool ok = false;
    if (file) {
        ok = file.print(text) > 0;
        file.close();
    }
    cJSON_free(text);
#if DEBUG_ENABLED
    Serial.printf("StorageManager: settings.json %s.\n", ok ? "saved" : "save FAILED");
#endif
    return ok;
}

void StorageManager::update(SystemState &state)
{
    state.sdMounted = cardPresent_;
    if (cardPresent_ && state.settingsDirty) {
        state.settingsDirty = false;
        saveSettings(state);
        refreshCapacity(state);
    }
    if (state.exportRequested && !exporting_) {
        state.exportRequested = false;
        exporting_ = true;
        headerSent_ = false;
        exportCount_ = count_;
        exportIndex_ = 0;
    }
    // Freeze ring contents during export so wrapping cannot corrupt the CSV.
    if (exporting_) {
        char line[128];
        int length;
        if (!headerSent_) {
            length = snprintf(line, sizeof(line), "demo,timestamp_ms,voltage_V,current_A,power_W,energy_Wh,temp_C\r\n");
        } else if (exportIndex_ < exportCount_) {
            const auto &item = records_[(head_ + capacity_ - exportCount_ + exportIndex_) % capacity_];
            length = snprintf(line, sizeof(line), "1,%lu,%.3f,%.3f,%.3f,%.6f,%.2f\r\n",
                static_cast<unsigned long>(item.timestamp), item.voltage, item.current, item.power, item.energy, item.temperature);
        } else {
            exporting_ = false;
            return;
        }
        if (length > 0 && length < static_cast<int>(sizeof(line)) && Serial.availableForWrite() >= length) {
            Serial.write(reinterpret_cast<const uint8_t *>(line), length);
            if (!headerSent_) headerSent_ = true;
            else ++exportIndex_;
        }
        return;
    }
    if (state.simulation && state.settings.logging && state.uptimeMs - lastRecord_ >= 1000) {
        lastRecord_ = state.uptimeMs;
        records_[head_] = state.latest;
        head_ = (head_ + 1) % capacity_;
        if (count_ < capacity_) ++count_;
    }
}

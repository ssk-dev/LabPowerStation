#include "hardware.h"
#include "config.h"
#include <Arduino.h>
#include <math.h>

bool HardwareManager::begin(SystemState &state)
{
    state.flashBytes = ESP.getFlashChipSize();
    state.psramBytes = ESP.getPsramSize();
#if DEBUG_ENABLED
    Serial.printf("ESP32: %s\nFlash: %lu bytes\nPSRAM: %lu bytes\n",
        ESP.getChipModel(), static_cast<unsigned long>(state.flashBytes),
        static_cast<unsigned long>(state.psramBytes));
#endif
    if (String(ESP.getChipModel()) != "ESP32-S3" || !psramFound() ||
        state.flashBytes < 16UL * 1024 * 1024 || state.psramBytes < 8UL * 1024 * 1024) return false;
    if (!board_.init() || !board_.begin()) return false;
    auto lcd = board_.getLCD();
    if (!lcd || !board_.getTouch() || !board_.getBacklight()) return false;
    if (lcd->getFrameWidth() != Config::width || lcd->getFrameHeight() != Config::height) return false;
    if (!board_.getBacklight()->setBrightness(100)) return false;
    state.boardReady = true;
    update(state);
    return true;
}

void HardwareManager::update(SystemState &state)
{
    state.freeHeap = ESP.getFreeHeap();
    state.freePsram = ESP.getFreePsram();
}

void FanManager::update(SystemState &state)
{
    if (!state.simulation) {
        state.fanPercent = 0;
        return;
    }
    state.fanPercent = state.settings.fanAuto ?
        static_cast<unsigned>(fminf(100, fmaxf(20, (state.temperature - 25) * 4))) :
        state.settings.fanManual;
}

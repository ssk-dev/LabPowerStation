#include <Arduino.h>
#include "config.h"
#include "hardware.h"
#include "display.h"
#include "touch.h"
#include "system_state.h"
#include "measurement.h"
#include "power_supply.h"
#include "ui.h"
#include "storage.h"

namespace {
SystemState state;
HardwareManager hardware;
DisplayManager display;
TouchManager touch;
SimulationSource simulation;
MeasurementManager measurements(state, simulation);
PowerSupplyManager power(state);
FanManager fan;
StorageManager storage;
NetworkManager network;
UI::Context uiContext{state, measurements, power};
bool ready = false;
uint32_t lastGui = 0;
uint32_t lastSystem = 0;

void fail(const char *reason)
{
    power.allOff();
#if DEBUG_ENABLED
    Serial.printf("FATAL: %s\nOutputs OFF. Check board settings / libraries / wiring.\n", reason);
#endif
}
}

/** Real hardware validation first; no instrument outputs are ever driven in phase 1. */
void setup()
{
    Serial.begin(115200);
    initializeState(state);
#if DEBUG_ENABLED
    Serial.println("\n================================\nLAB POWER STATION\n================================");
#endif
    if (!hardware.begin(state)) { fail("Board, flash or PSRAM initialization"); return; }
    if (!display.begin(hardware.board().getLCD())) { fail("LVGL / display buffer"); return; }
    if (!touch.begin(hardware.board().getTouch())) { fail("GT911 input registration"); return; }
    if (!measurements.begin()) { fail("PSRAM history allocation"); return; }
    storage.begin(state, hardware.board()); // Optional: no SD card is not a fatal error, only disables settings.json persistence.
    UI::begin(uiContext);
    if (!SIMULATION_MODE) power.trip(Fault::ADC);
#if DEBUG_ENABLED
    Serial.printf("LCD: 800x480\nBacklight: ON\nGT911 driver: initialized\nLVGL: %d.%d.%d\n",
        LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    Serial.println("Waiting for visual/touch confirmation. External measurement hardware: NOT IMPLEMENTED.");
#endif
    ready = true;
}

/** One LVGL owner, independent measurement/UI/service schedules, wrap-safe millis math. */
void loop()
{
    if (!ready) { delay(5); return; }
    const uint32_t now = millis();
    state.uptimeMs = now;
    measurements.update(now);
    storage.update(state);
    if (now - lastSystem >= Config::systemMs) {
        lastSystem = now;
        hardware.update(state);
        fan.update(state);
        network.update(state);
    }
    if ((!display.healthy() || !touch.healthy()) && state.fault == Fault::None) power.trip(Fault::Communication);
    if (now - lastGui >= Config::guiMs) {
        lastGui = now;
        UI::update(now);
    }
    display.update(now);
    if (state.restartRequested) {
        power.allOff();
        ESP.restart();
    }
    delay(1); // Yield only; no long blocking work in normal operation.
}

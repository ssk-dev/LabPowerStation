#pragma once
#include <lvgl.h>
#include "system_state.h"
#include "measurement.h"
#include "power_supply.h"

namespace UI {
enum class Screen { Dashboard, Power, Multimeter, Components, Graph, Settings, Count };
constexpr uint32_t background = 0x10151e;
constexpr uint32_t panelColor = 0x1b2432;
constexpr uint32_t borderColor = 0x334356;
constexpr uint32_t foreground = 0xe8eef5;
constexpr uint32_t muted = 0x95a9bd;
constexpr uint32_t cyan = 0x42c9ef;
constexpr uint32_t green = 0x67de9a;
constexpr uint32_t amber = 0xffc563;
constexpr uint32_t red = 0xff6574;

/** All references are owned by the sketch and accessed only in the Arduino task. */
struct Context {
    SystemState &state;
    MeasurementManager &measurements;
    PowerSupplyManager &power;
};
Context &context();
/** Build cached pages and show a touch-gated hardware validation screen. */
void begin(Context &context);
void update(uint32_t now);
void navigate(Screen screen);
Screen activeScreen();

/** Shared fixed-position controls, with zero container padding. */
lv_obj_t *panel(lv_obj_t *parent, int x, int y, int width, int height);
lv_obj_t *label(lv_obj_t *parent, int x, int y, int width, const char *text,
    const lv_font_t *font = &lv_font_montserrat_16, uint32_t color = foreground);
lv_obj_t *button(lv_obj_t *parent, int x, int y, int width, int height, const char *text,
    lv_event_cb_t callback, intptr_t value = 0, uint32_t color = borderColor);
/** Avoid invalidation/layout work when formatted text did not change. */
void text(lv_obj_t *label, const char *format, ...);
void buttonText(lv_obj_t *button, const char *text);
void checked(lv_obj_t *button, bool checked);
void message(const char *title, const char *body);
/** Strict decimal parsing, bounded values, explicit confirmation; no live output changes. */
void numeric(bool currentLimit);
/** Test overlay: color, touch coordinates, button counter and live memory statistics. */
void testMode();
void hardwareTest();
}

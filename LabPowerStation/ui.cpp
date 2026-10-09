#include "ui.h"
#include "config.h"
#include "ui_dashboard.h"
#include "ui_power_supply.h"
#include "ui_multimeter.h"
#include "ui_component_tester.h"
#include "ui_graph.h"
#include "ui_settings.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

namespace UI {
namespace {
Context *ctx = nullptr;
lv_obj_t *pages[static_cast<unsigned>(Screen::Count)] = {};
lv_obj_t *nav[5] = {};
lv_obj_t *status = nullptr;
lv_obj_t *modal = nullptr;
lv_obj_t *faultOverlay = nullptr;
lv_obj_t *boot = nullptr;
lv_obj_t *dimmer = nullptr;
lv_obj_t *entry = nullptr;
lv_obj_t *entryError = nullptr;
lv_obj_t *testInfo = nullptr;
lv_obj_t *testColor = nullptr;
Screen current = Screen::Dashboard;
Fault shownFault = Fault::None;
bool editingCurrent = false;
unsigned presses = 0;
uint32_t lastStatus = 0;

void closeModal(lv_event_t *)
{
    if (modal) lv_obj_del_async(modal);
    modal = entry = entryError = testInfo = testColor = nullptr;
}

lv_obj_t *openModal(const char *title)
{
    closeModal(nullptr);
    modal = panel(lv_layer_top(), 0, 0, 800, 480);
    lv_obj_set_style_bg_color(modal, lv_color_hex(background), 0);
    lv_obj_set_style_radius(modal, 0, 0);
    label(modal, 24, 14, 620, title, &lv_font_montserrat_28, cyan);
    button(modal, 654, 8, 128, 48, "ZURUECK", closeModal);
    return modal;
}

void navEvent(lv_event_t *event)
{
    navigate(static_cast<Screen>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
}

void bootConfirmed(lv_event_t *)
{
    context().state.touchConfirmed = true;
    if (boot) lv_obj_del_async(boot);
    boot = nullptr;
    navigate(Screen::Dashboard);
}

void acknowledge(lv_event_t *)
{
    if (context().power.acknowledge()) {
        lv_obj_del_async(faultOverlay);
        faultOverlay = nullptr;
        shownFault = Fault::None;
    }
}

void keypadEvent(lv_event_t *event)
{
    auto matrix = lv_event_get_target(event);
    const char *key = lv_btnmatrix_get_btn_text(matrix, lv_btnmatrix_get_selected_btn(matrix));
    if (!key) return;
    if (!strcmp(key, LV_SYMBOL_BACKSPACE)) lv_textarea_del_char(entry);
    else if (!strcmp(key, ".") && strchr(lv_textarea_get_text(entry), '.')) return;
    else lv_textarea_add_text(entry, key);
}

void acceptNumber(lv_event_t *)
{
    const char *input = lv_textarea_get_text(entry);
    char *end = nullptr;
    float value = strtof(input, &end);
    if (!*input || end == input || *end || !isfinite(value) ||
        !(editingCurrent ? context().power.setCurrentLimit(value) : context().power.setVoltage(value))) {
        text(entryError, "Erlaubt: 0 ... %.1f %s", editingCurrent ? Config::maxCurrent : Config::maxVoltage,
            editingCurrent ? "A" : "V");
        return;
    }
    closeModal(nullptr);
}

void testEvent(lv_event_t *event)
{
    const intptr_t action = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    if (action == 0) ++presses;
    else {
        static const uint32_t colors[] = {red, green, cyan, 0xffffff, 0x000000};
        lv_obj_set_style_bg_color(testColor, lv_color_hex(colors[(++presses) % 5]), 0);
    }
}

void faultEvent(lv_event_t *event)
{
    const auto fault = static_cast<Fault>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    closeModal(nullptr);
    context().power.trip(fault, fault == Fault::OCP ? 5.42f : 0, fault == Fault::OCP ? 5.0f : 0);
}
}

Context &context() { return *ctx; }
Screen activeScreen() { return current; }

lv_obj_t *panel(lv_obj_t *parent, int x, int y, int width, int height)
{
    auto object = lv_obj_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_style_border_width(object, 1, 0);
    lv_obj_set_style_border_color(object, lv_color_hex(borderColor), 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(panelColor), 0);
    lv_obj_set_style_radius(object, 8, 0);
    return object;
}

lv_obj_t *label(lv_obj_t *parent, int x, int y, int width, const char *value, const lv_font_t *font, uint32_t color)
{
    auto object = lv_label_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_width(object, width);
    lv_obj_set_style_text_font(object, font, 0);
    lv_obj_set_style_text_color(object, lv_color_hex(color), 0);
    lv_label_set_text(object, value);
    return object;
}

lv_obj_t *button(lv_obj_t *parent, int x, int y, int width, int height, const char *value,
    lv_event_cb_t callback, intptr_t data, uint32_t color)
{
    auto object = lv_btn_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, height);
    lv_obj_set_style_radius(object, 6, 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(color), 0);
    lv_obj_set_style_bg_color(object, lv_color_hex(0x256580), LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(object, 0, 0);
    lv_obj_set_style_pad_all(object, 2, 0);
    auto caption = lv_label_create(object);
    lv_label_set_text(caption, value);
    lv_obj_set_style_text_color(caption, lv_color_hex(foreground), 0);
    lv_obj_set_style_text_font(caption, &lv_font_montserrat_16, 0);
    lv_obj_center(caption);
    // SHORT_CLICKED deliberately suppresses auto-repeat and long-press release clicks.
    if (callback) lv_obj_add_event_cb(object, callback, LV_EVENT_SHORT_CLICKED, reinterpret_cast<void *>(data));
    return object;
}

void text(lv_obj_t *object, const char *format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (strcmp(lv_label_get_text(object), buffer)) lv_label_set_text(object, buffer);
}

void buttonText(lv_obj_t *object, const char *value)
{
    text(lv_obj_get_child(object, 0), "%s", value);
}

void checked(lv_obj_t *object, bool value)
{
    if (value) lv_obj_add_state(object, LV_STATE_CHECKED);
    else lv_obj_clear_state(object, LV_STATE_CHECKED);
}

void navigate(Screen screen)
{
    if (screen >= Screen::Count) return;
    current = screen;
    for (unsigned i = 0; i < static_cast<unsigned>(Screen::Count); ++i) {
        if (i == static_cast<unsigned>(screen)) lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (unsigned i = 0; i < 5; ++i) checked(nav[i], static_cast<unsigned>(screen) == i + 1);
}

void begin(Context &newContext)
{
    ctx = &newContext;
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(background), 0);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    status = label(lv_scr_act(), 14, 13, 680, "INITIALISIERUNG...", &lv_font_montserrat_14, muted);
    button(lv_scr_act(), 702, 0, 96, 44, "HOME", navEvent, static_cast<intptr_t>(Screen::Dashboard));
    for (auto &page : pages) {
        page = panel(lv_scr_act(), 0, 48, 800, 366);
        lv_obj_set_style_bg_color(page, lv_color_hex(background), 0);
        lv_obj_set_style_border_width(page, 0, 0);
    }
    buildDashboard(pages[0]);
    buildPower(pages[1]);
    buildMultimeter(pages[2]);
    buildComponents(pages[3]);
    buildGraph(pages[4]);
    buildSettings(pages[5]);
    const char *names[] = {"NETZTEIL", "MULTIMETER", "BAUTEILE", "VERLAUF", "EINSTELL."};
    for (unsigned i = 0; i < 5; ++i) nav[i] = button(lv_scr_act(), i * 160 + 3, 420, 154, 56, names[i], navEvent, i + 1);
    navigate(Screen::Dashboard);
    // Software dim overlay preserves actual backlight operation (EXIO2 is on/off only).
    dimmer = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(dimmer);
    lv_obj_set_size(dimmer, 800, 480);
    lv_obj_set_style_bg_color(dimmer, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(dimmer, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(dimmer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    hardwareTest();
}

void hardwareTest()
{
    if (boot) return;
    boot = panel(lv_layer_top(), 0, 0, 800, 480);
    lv_obj_set_style_bg_color(boot, lv_color_hex(background), 0);
    label(boot, 48, 28, 710, "LAB POWER STATION", &lv_font_montserrat_40, cyan);
    label(boot, 48, 82, 710, "Portable Electronics Laboratory / Hardware Test", &lv_font_montserrat_20);
    auto result = label(boot, 64, 132, 680, "", &lv_font_montserrat_20);
    char sdInfo[32];
    if (context().state.sdMounted) snprintf(sdInfo, sizeof(sdInfo), "%.1fGB / %.1fGB", context().state.sdUsedGB, context().state.sdTotalGB);
    else snprintf(sdInfo, sizeof(sdInfo), "nicht erkannt");
    text(result, "ESP32-S3                       OK\nFlash                       %lu MB\nPSRAM                         %lu MB\nLCD                    800 x 480 / initialisiert\nGT911                         Treiber initialisiert\nLVGL                       %d.%d.%d\nSD-Karte                       %s",
        static_cast<unsigned long>(context().state.flashBytes / 1048576),
        static_cast<unsigned long>(context().state.psramBytes / 1048576),
        LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH, sdInfo);
    auto progress = lv_bar_create(boot);
    lv_obj_set_pos(progress, 64, 306);
    lv_obj_set_size(progress, 670, 12);
    lv_bar_set_value(progress, 100, LV_ANIM_OFF);
    label(boot, 64, 340, 680, "Bild pruefen, dann tippen: Erst die Beruehrung bestaetigt Touch.", &lv_font_montserrat_16, amber);
    button(boot, 190, 393, 420, 62, "BILD OK - TOUCH BESTAETIGEN", bootConfirmed, 0, 0x256580);
}

void message(const char *title, const char *body)
{
    auto object = openModal(title);
    label(object, 34, 96, 732, body, &lv_font_montserrat_20);
}

void numeric(bool currentLimit)
{
    editingCurrent = currentLimit;
    auto object = openModal(currentLimit ? "STROMLIMIT (DEMO)" : "SOLLSPANNUNG (DEMO)");
    const auto &channel = context().state.channels[POWER_VARIABLE];
    auto info = label(object, 35, 88, 330, "", &lv_font_montserrat_20, muted);
    text(info, "Aktuell: %.3f %s\n\nNeuen Wert eingeben.\nUebernahme erst mit OK.",
        currentLimit ? channel.currentLimit : channel.setVoltage, currentLimit ? "A" : "V");
    entry = lv_textarea_create(object);
    lv_obj_set_pos(entry, 390, 70);
    lv_obj_set_size(entry, 365, 58);
    lv_textarea_set_one_line(entry, true);
    lv_textarea_set_max_length(entry, 8);
    lv_textarea_set_accepted_chars(entry, "0123456789.");
    lv_textarea_set_text(entry, "");
    lv_obj_set_style_text_font(entry, &lv_font_montserrat_28, 0);
    static const char *keys[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", ".", "0", LV_SYMBOL_BACKSPACE, ""};
    auto matrix = lv_btnmatrix_create(object);
    lv_obj_set_pos(matrix, 390, 138);
    lv_obj_set_size(matrix, 365, 250);
    lv_btnmatrix_set_map(matrix, keys);
    lv_btnmatrix_set_btn_ctrl_all(matrix, LV_BTNMATRIX_CTRL_NO_REPEAT | LV_BTNMATRIX_CTRL_CLICK_TRIG);
    lv_obj_add_event_cb(matrix, keypadEvent, LV_EVENT_VALUE_CHANGED, nullptr);
    entryError = label(object, 35, 300, 330, "", &lv_font_montserrat_20, red);
    button(object, 390, 405, 170, 56, "ABBRECHEN", closeModal);
    button(object, 580, 405, 175, 56, "OK", acceptNumber, 0, 0x256580);
}

void testMode()
{
    auto object = openModal("SYSTEM TEST / DEMO FAULTS");
    presses = 0;
    testColor = panel(object, 24, 70, 220, 138);
    lv_obj_set_style_bg_color(testColor, lv_color_hex(cyan), 0);
    button(object, 24, 220, 106, 48, "FARBE", testEvent, 1);
    button(object, 140, 220, 106, 48, "BUTTON", testEvent, 0);
    testInfo = label(object, 272, 80, 500, "", &lv_font_montserrat_20);
    label(object, 24, 284, 750, "Fehler einspeisen: schaltet ALLE Demo-Ausgaenge aus.", &lv_font_montserrat_16, amber);
    const char *names[] = {"OVP", "OCP", "OTP", "SHORT", "ADC", "SENSOR", "FAN", "COMM"};
    for (unsigned i = 0; i < 8; ++i) button(object, 24 + (i % 4) * 192, 318 + (i / 4) * 62,
        180, 52, names[i], faultEvent, i + 1, 0x643540);
    label(object, 24, 449, 750, "Chart-Test: ZURUECK > VERLAUF. Memory zeigt reale freie Bytes.", &lv_font_montserrat_14, muted);
}

void update(uint32_t now)
{
    if (!ctx) return;
    auto &state = context().state;
    if (state.fault != shownFault && state.fault != Fault::None) {
        closeModal(nullptr);
        if (faultOverlay) lv_obj_del_async(faultOverlay);
        faultOverlay = panel(lv_layer_top(), 0, 0, 800, 480);
        label(faultOverlay, 44, 32, 710, "WARNING", &lv_font_montserrat_40, red);
        auto faultText = label(faultOverlay, 44, 112, 710, "", &lv_font_montserrat_28);
        text(faultText, "%s\n\nALLE AUSGAENGE DEAKTIVIERT\n\nWert: %.3f   Limit: %.3f\n%s", faultName(state.fault), state.faultValue,
            state.faultLimit, state.simulation ? "SIMULIERTER FEHLER" : "Hardwaretreiber nicht implementiert");
        button(faultOverlay, 220, 382, 360, 64, state.simulation ? "QUITTIEREN" : "GESPERRT - RESET NOETIG", acknowledge, 0, 0x643540);
        shownFault = state.fault;
    }
    if (testInfo) {
        lv_point_t point = {};
        for (auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if (lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER) {
                lv_indev_get_point(input, &point);
                break;
            }
        }
        text(testInfo, "Touch: x=%d y=%d\nButton / Farbe: %u\nHeap frei: %lu Bytes\nPSRAM frei: %lu Bytes", point.x, point.y,
            presses, static_cast<unsigned long>(state.freeHeap), static_cast<unsigned long>(state.freePsram));
    }
    if (now - lastStatus >= Config::statusMs) {
        lastStatus = now;
        const unsigned minutes = state.uptimeMs / 60000;
        char sdInfo[24];
        if (state.sdMounted) snprintf(sdInfo, sizeof(sdInfo), "%.1fGB / %.1fGB", state.sdUsedGB, state.sdTotalGB);
        else snprintf(sdInfo, sizeof(sdInfo), "--");
        text(status, "UP %02u:%02u  WIFI --  USB ?  SD %s  FAN %u%% D  %.1f C D  %s",
            minutes / 60, minutes % 60, sdInfo, state.fanPercent, state.temperature,
            state.fault == Fault::None ? (state.simulation ? "DEMO" : "LOCK") : "FAULT");
        unsigned opacity = (100 - state.settings.contrast) * 180 / 100;
        if (state.settings.night && opacity < 80) opacity = 80;
        lv_obj_set_style_bg_opa(dimmer, opacity, 0);
    }
    if (boot || faultOverlay) return;
    switch (current) {
        case Screen::Dashboard: updateDashboard(); break;
        case Screen::Power: updatePower(); break;
        case Screen::Multimeter: updateMultimeter(); break;
        case Screen::Components: updateComponents(now); break;
        case Screen::Graph: updateGraph(now); break;
        case Screen::Settings: updateSettings(); break;
        default: break;
    }
}
}

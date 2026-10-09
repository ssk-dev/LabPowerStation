#include "ui_settings.h"
#include "ui.h"
#include "config.h"

namespace UI {
namespace {
lv_obj_t *menu, *detail, *heading, *description, *slider, *sliderValue, *controls[4];
unsigned category = 0;
const char *categories[] = {"GENERAL", "DISPLAY", "MEASUREMENT", "POWER SUPPLY", "CALIBRATION", "LIMITS", "FAN", "WIFI", "LOGGING", "SYSTEM"};
const char *details[] = {
    "Sprache: Deutsch / technische Begriffe Englisch\nEinheiten: SI, Temperatur Celsius, Zeit: Uptime\nStart: Hardwaretest, dann Dashboard. Kein Timeout.\nTon: kein Buzzer angebunden. Einstellungen in\n/settings.json auf SD/TF-Karte (ohne Karte: RAM).",
    "Backlight EXIO2: EIN/AUS, kein PWM-Dimmen.\nRegler: SOFTWARE-Dimmung, kein Stromspareffekt.\nTheme: Dark. Night Mode reduziert die Bildhelligkeit.\nGT911: Herstellerkoordinaten, keine Kalibrierung noetig.",
    "Quelle: SimulationSource, keine ADCs verbunden.\nUpdate Rate: 10 Hz oder 20 Hz. Keine ADC-Filterung.\nACS758 / ADS1256 erst nach Schaltungsplanung.\nUI kennt keine physischen ADC-Kanaele.",
    "Alle Ausgaenge starten OFF und bleiben nach Fehler OFF.\nDEMO VAR: 0-30 V / 0-5 A. Keine Hardware-Nennwerte!\nCV/CC wird fuer eine ohmsche Demo-Last simuliert.\nKeine Rampensteuerung oder echte Leistungsstufe.",
    "3.3 V / 5 V / 12 V / VAR: keine reale Kalibrierung.\nVoltage Offset/Gain und Current Offset/Gain erst mit\nvalidierter Referenz und ADC-Treiber freigeben.\nKeine wirkungslosen Kalibrierwerte speichern.",
    "OVP / OCP / OTP / SHORT: Fehleranzeige und Abschaltung\nder Simulation. Keine reale Schutzschaltung!\nUnabhaengige Hardwareabschaltung zwingend notwendig.\nFehlereinspeisung im System-Testmodus.",
    "Luefter ist NICHT angeschlossen. Anzeige nur Demo.\nAUTO: Prozentwert aus simulierter Temperatur.\nMANUAL: Prozentwert ueber Regler.\nSILENT: Demo-Handbetrieb mit 20 Prozent.",
    "WLAN ist in Phase 1 deaktiviert. SSID / IP: nicht vorhanden.\nKeine Zugangsdaten gespeichert, keine Verbindungen.\nConnect/Disconnect erst mit NetworkManager-Backend.\nTouch und Logging funktionieren offline.",
    "Logging: maximal 120 Demo-Samples im RAM, 1 Hz.\nExport CSV ueber Serial 115200, keine SD-Datei.\nExport pausiert Aufnahme; Ringdaten bleiben konsistent.\nEinstellungen werden automatisch in /settings.json\nauf der SD/TF-Karte gespeichert (EXIO4-CS, SPI).",
    ""
};

void back(lv_event_t *)
{
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(detail, LV_OBJ_FLAG_HIDDEN);
}
void testLongPress(lv_event_t *) { testMode(); }
void sliderChanged(lv_event_t *)
{
    unsigned value = lv_slider_get_value(slider);
    auto &state = context().state;
    if (category == 1) state.settings.contrast = value;
    if (category == 6) state.settings.fanManual = value;
    state.settingsDirty = true;
    updateSettings();
}
void configure(unsigned selected)
{
    category = selected;
    text(heading, "%s", categories[category]);
    text(description, "%s", details[category]);
    lv_obj_add_flag(slider, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(sliderValue, LV_OBJ_FLAG_HIDDEN);
    for (auto object : controls) lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    auto show = [](unsigned index, const char *caption) {
        buttonText(controls[index], caption);
        lv_obj_clear_flag(controls[index], LV_OBJ_FLAG_HIDDEN);
    };
    switch (category) {
        case 1:
            lv_obj_clear_flag(slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sliderValue, LV_OBJ_FLAG_HIDDEN);
            lv_slider_set_range(slider, 20, 100);
            lv_slider_set_value(slider, context().state.settings.contrast, LV_ANIM_OFF);
            show(0, "NIGHT MODE"); show(1, "LCD / TOUCH TEST"); break;
        case 2: show(0, "10 Hz"); show(1, "20 Hz"); break;
        case 3: show(0, "ALLE AUS"); show(1, "SET V"); show(2, "LIMIT A"); break;
        case 5: show(0, "FEHLERTEST"); break;
        case 6:
            lv_obj_clear_flag(slider, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sliderValue, LV_OBJ_FLAG_HIDDEN);
            lv_slider_set_range(slider, 0, 100);
            lv_slider_set_value(slider, context().state.settings.fanManual, LV_ANIM_OFF);
            show(0, "AUTO"); show(1, "MANUAL"); show(2, "SILENT"); break;
        case 8: show(0, "LOGGING ON/OFF"); show(1, "EXPORT CSV"); break;
        case 9: show(0, "TESTMODE"); show(1, "NEUSTART"); show(2, "DEFAULTS"); break;
        default: break;
    }
    updateSettings();
}
void select(lv_event_t *event)
{
    configure(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(detail, LV_OBJ_FLAG_HIDDEN);
}
void action(lv_event_t *event)
{
    unsigned index = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    auto &state = context().state;
    switch (category) {
        case 1:
            if (index == 0) { state.settings.night = !state.settings.night; state.settingsDirty = true; }
            else hardwareTest();
            break;
        case 2: state.settings.updateMs = index == 0 ? 100 : 50; state.settingsDirty = true; break;
        case 3:
            if (index == 0) context().power.allOff();
            else numeric(index == 2);
            break;
        case 5: testMode(); break;
        case 6:
            state.settings.fanAuto = index == 0;
            if (index == 2) {
                state.settings.fanManual = 20;
                lv_slider_set_value(slider, 20, LV_ANIM_OFF);
            }
            state.settingsDirty = true;
            break;
        case 8:
            if (index == 0) { state.settings.logging = !state.settings.logging; state.settingsDirty = true; }
            else {
                state.exportRequested = true;
                message("CSV EXPORT", "CSV-Ausgabe angefordert (Serial, 115200 Baud).\n\nVorher Logging aktivieren und Daten sammeln.\nBis zu 120 Samples, Aufnahme pausiert beim Export.\nOhne seriellen Empfaenger kann Export warten.\nKeine SD-Datei, kein Netzwerktransfer.");
            }
            break;
        case 9:
            if (index == 0) testMode();
            else if (index == 1) { context().power.allOff(); state.restartRequested = true; }
            else {
                context().power.allOff();
                state.settings = Settings{};
                state.settingsDirty = true;
                context().power.setVoltage(24);
                context().power.setCurrentLimit(5);
                message("DEFAULTS", "Einstellungen zurueckgesetzt und gespeichert. Ausgaenge OFF.\n\nKeine Zugangsdaten / NVS-Einstellungen vorhanden.\nMesshistorie bleibt bis zum Neustart erhalten.\nEin latched Fehler wird NICHT geloescht.");
            }
            break;
        default: break;
    }
    updateSettings();
}
}

void buildSettings(lv_obj_t *parent)
{
    menu = panel(parent, 0, 0, 800, 366);
    label(menu, 14, 4, 760, "EINSTELLUNGEN / PHASE 1", &lv_font_montserrat_20);
    for (unsigned i = 0; i < 10; ++i) {
        auto object = button(menu, 12 + (i % 2) * 394, 36 + (i / 2) * 64, 382, 56, categories[i], select, i);
        if (i == 9) lv_obj_add_event_cb(object, testLongPress, LV_EVENT_LONG_PRESSED, nullptr);
    }
    detail = panel(parent, 0, 0, 800, 366);
    heading = label(detail, 16, 8, 620, "", &lv_font_montserrat_20, cyan);
    button(detail, 650, 3, 134, 46, "ZURUECK", back);
    description = label(detail, 20, 64, 760, "", &lv_font_montserrat_20);
    slider = lv_slider_create(detail);
    lv_obj_set_pos(slider, 36, 233);
    lv_obj_set_size(slider, 535, 20);
    lv_obj_set_ext_click_area(slider, 18);
    lv_obj_add_event_cb(slider, sliderChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    sliderValue = label(detail, 602, 224, 180, "", &lv_font_montserrat_20);
    for (unsigned i = 0; i < 4; ++i) controls[i] = button(detail, 12 + i * 196, 296, 186, 56, "", action, i);
    back(nullptr);
}

void updateSettings()
{
    const auto &state = context().state;
    if (category == 9) text(description,
        "Firmware %s / Original Waveshare 4.3\nESP32-S3 / Flash %lu MB / PSRAM %lu MB\nLVGL %d.%d.%d / Freier Heap %lu Bytes\nSYSTEM lange druecken: LCD / Touch / Fehler / Memory",
        Config::firmware, static_cast<unsigned long>(state.flashBytes / 1048576),
        static_cast<unsigned long>(state.psramBytes / 1048576), LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH,
        static_cast<unsigned long>(state.freeHeap));
    if (category == 1) {
        text(sliderValue, "%u %% SW", state.settings.contrast);
        checked(controls[0], state.settings.night);
    }
    if (category == 2) {
        checked(controls[0], state.settings.updateMs == 100);
        checked(controls[1], state.settings.updateMs == 50);
    }
    if (category == 6) {
        text(sliderValue, "%u %% DEMO", state.settings.fanManual);
        checked(controls[0], state.settings.fanAuto);
        checked(controls[1], !state.settings.fanAuto);
    }
    if (category == 8) checked(controls[0], state.settings.logging);
}
}

#include "ui_multimeter.h"
#include "ui.h"

namespace UI {
namespace {
lv_obj_t *menu, *measurement, *title, *value, *range, *stats, *controls[4];
void back(lv_event_t *)
{
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(measurement, LV_OBJ_FLAG_HIDDEN);
}
void select(lv_event_t *event)
{
    context().measurements.selectMeter(static_cast<MeterMode>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event))));
    lv_obj_add_flag(menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(measurement, LV_OBJ_FLAG_HIDDEN);
    updateMultimeter();
}
void action(lv_event_t *event)
{
    auto &meter = context().state.meter;
    switch (reinterpret_cast<intptr_t>(lv_event_get_user_data(event))) {
        case 0: meter.autorange = !meter.autorange; break;
        case 1: meter.hold = !meter.hold; break;
        case 2: context().measurements.toggleRelative(); break;
        case 3: meter.statistics = !meter.statistics; context().measurements.resetMeterStatistics(); break;
        case 4: message("MULTIMETER / DEMO", "Alle Messarten zeigen synthetische Signale.\n\nAUTO: automatische Bereichsanzeige (Demo).\nHOLD: friert Anzeige und Statistik ein.\nREL: zieht den aktuellen Rohwert ab.\nMIN/MAX: Statistik ein/aus und Neustart.\n\nKein ADC, kein True-RMS-Messverfahren, keine\nSpannungsfestigkeit oder CAT-Einstufung vorhanden."); break;
    }
    updateMultimeter();
}
}

void buildMultimeter(lv_obj_t *parent)
{
    menu = panel(parent, 0, 0, 800, 366);
    label(menu, 14, 6, 760, "MULTIMETER / MESSART AUSWAEHLEN", &lv_font_montserrat_20);
    const char *names[] = {"DC V", "AC V", "DC A", "AC A", "Ohm", "Durchgang", "Diode", "Kapazitaet", "Frequenz"};
    for (unsigned i = 0; i < 9; ++i) button(menu, 14 + (i % 3) * 260, 46 + (i / 3) * 100, 250, 86, names[i], select, i);
    measurement = panel(parent, 0, 0, 800, 366);
    title = label(measurement, 16, 10, 600, "", &lv_font_montserrat_20, cyan);
    button(measurement, 652, 4, 132, 48, "MESSART", back);
    value = label(measurement, 42, 82, 720, "", &lv_font_montserrat_40, green);
    lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, 0);
    range = label(measurement, 42, 146, 720, "", &lv_font_montserrat_20, muted);
    lv_obj_set_style_text_align(range, LV_TEXT_ALIGN_CENTER, 0);
    stats = label(measurement, 35, 210, 730, "", &lv_font_montserrat_20);
    const char *actions[] = {"AUTO", "HOLD", "REL", "MIN/MAX", "SETTINGS"};
    for (unsigned i = 0; i < 5; ++i) {
        auto object = button(measurement, 14 + i * 156, 292, 148, 58, actions[i], action, i);
        if (i < 4) controls[i] = object;
    }
    back(nullptr);
}

void updateMultimeter()
{
    const auto &meter = context().state.meter;
    text(title, "MULTIMETER / %s", meterName(meter.mode));
    text(value, "%.3f %s", meter.value, meterUnit(meter.mode));
    text(range, "Range: %s  |  DEMO %s%s", meterRange(meter.mode, meter.autorange),
        meter.hold ? "HOLD " : "", meter.relative ? "REL" : "");
    if (meter.statistics) text(stats, "MIN  %.3f          MAX  %.3f          AVG  %.3f",
        meter.minimum, meter.maximum, meter.average);
    else text(stats, "Statistik ausgeblendet");
    checked(controls[0], meter.autorange);
    checked(controls[1], meter.hold);
    checked(controls[2], meter.relative);
    checked(controls[3], meter.statistics);
}
}

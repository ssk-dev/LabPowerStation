#include "ui_component_tester.h"
#include "ui.h"

namespace UI {
namespace {
lv_obj_t *choices[10], *result, *testButton;
unsigned selected = 9;
uint32_t started = 0;
bool running = false;
const char *names[] = {"R", "C", "L", "DIODE", "NPN", "PNP", "MOSFET", "JFET", "IGBT", "AUTO TEST"};
const char *results[] = {
    "RESISTOR\n1 -- 2\nR = 4.700 kOhm\nDEMO / kein Bauteil gemessen",
    "CAPACITOR\n1 (+) -- 2 (-)\nC = 100.2 uF   ESR = 0.18 Ohm\nDEMO / kein Bauteil gemessen",
    "INDUCTOR\n1 -- 2\nL = 10.4 mH   R = 2.1 Ohm\nDEMO / kein Bauteil gemessen",
    "DIODE\n1 ANODE   2 CATHODE\nVf = 0.681 V\nDEMO / kein Bauteil gemessen",
    "NPN TRANSISTOR\n1 BASE   2 COLLECTOR   3 EMITTER\nhFE = 214   Vbe = 0.67 V\nDEMO / kein Bauteil gemessen",
    "PNP TRANSISTOR\n1 BASE   2 COLLECTOR   3 EMITTER\nhFE = 186   Vbe = 0.65 V\nDEMO / kein Bauteil gemessen",
    "N-CHANNEL MOSFET\n1 GATE   2 DRAIN   3 SOURCE\nVTH = 2.40 V   RDS(on) = 0.038 Ohm\nDEMO / kein Bauteil gemessen",
    "N-CHANNEL JFET\n1 GATE   2 DRAIN   3 SOURCE\nIdss = 4.2 mA\nDEMO / kein Bauteil gemessen",
    "IGBT\n1 GATE   2 COLLECTOR   3 EMITTER\nVge(th) = 4.1 V\nDEMO / kein Bauteil gemessen"
};
void choose(lv_event_t *event)
{
    if (running) return;
    selected = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    for (unsigned i = 0; i < 10; ++i) checked(choices[i], i == selected);
    text(result, "TEST READY\n\nBauteil einsetzen (nur Visualisierung).\n%s / simuliertes Ergebnis", names[selected]);
}
void start(lv_event_t *)
{
    if (running || !context().state.simulation) return;
    running = true;
    started = context().state.uptimeMs;
    buttonText(testButton, "SIMULIERE...");
    lv_obj_add_state(testButton, LV_STATE_DISABLED);
    text(result, "DEMO TEST RUNNING\n\nKeine elektrischen Messungen.\nKein ZIF-Sockel angeschlossen.");
}
}

void buildComponents(lv_obj_t *parent)
{
    label(parent, 14, 4, 770, "BAUTEILTESTER / SIMULIERTE ERGEBNISSE", &lv_font_montserrat_20);
    for (unsigned i = 0; i < 10; ++i) choices[i] = button(parent, 12 + (i % 5) * 157, 38 + (i / 5) * 58,
        149, 50, names[i], choose, i);
    auto socket = panel(parent, 14, 165, 214, 182);
    label(socket, 16, 12, 180, "ZIF SOCKET", &lv_font_montserrat_20, green);
    for (unsigned i = 0; i < 6; ++i) {
        auto pin = panel(socket, 22 + (i % 3) * 61, 58 + (i / 3) * 50, 42, 36);
        auto caption = label(pin, 12, 7, 24, "");
        text(caption, "%u", i % 3 + 1);
    }
    label(socket, 16, 157, 190, "NUR DARSTELLUNG", &lv_font_montserrat_14, amber);
    result = label(parent, 250, 166, 528, "TEST READY\n\nBauteil einsetzen (Demo).\nAUTO TEST waehlt ein MOSFET-Beispiel.", &lv_font_montserrat_20);
    testButton = button(parent, 400, 297, 370, 54, "TEST STARTEN", start, 0, 0x256580);
    checked(choices[9], true);
}

void updateComponents(uint32_t now)
{
    if (running && now - started >= 800) {
        running = false;
        text(result, "%s", results[selected == 9 ? 6 : selected]);
        buttonText(testButton, "ERNEUT TESTEN");
        lv_obj_clear_state(testButton, LV_STATE_DISABLED);
    }
}
}

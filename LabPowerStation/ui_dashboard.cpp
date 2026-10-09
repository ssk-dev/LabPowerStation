#include "ui_dashboard.h"
#include "ui.h"

namespace UI {
namespace {
lv_obj_t *voltage[4], *details[4], *toggle[4], *totals;
void toggleOutput(lv_event_t *event)
{
    auto id = static_cast<PowerId>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    context().power.setEnabled(id, !context().state.channels[id].enabled);
    updateDashboard();
}
}

void buildDashboard(lv_obj_t *parent)
{
    label(parent, 14, 4, 600, "POWER OVERVIEW", &lv_font_montserrat_20);
    label(parent, 627, 8, 170, "SIMULATION", &lv_font_montserrat_16, amber);
    const uint32_t colors[] = {cyan, green, amber, 0xb59aff};
    for (unsigned i = 0; i < 4; ++i) {
        auto card = panel(parent, 12 + 197 * i, 38, 185, 233);
        label(card, 14, 14, 160, channelName(static_cast<PowerId>(i)), &lv_font_montserrat_20, colors[i]);
        voltage[i] = label(card, 12, 58, 170, "0.000 V", &lv_font_montserrat_28, colors[i]);
        details[i] = label(card, 14, 107, 160, "", &lv_font_montserrat_20);
        toggle[i] = button(card, 12, 174, 159, 46, "OFF", toggleOutput, i);
    }
    auto summary = panel(parent, 12, 282, 776, 74);
    totals = label(summary, 20, 12, 740, "", &lv_font_montserrat_20);
    updateDashboard();
}

void updateDashboard()
{
    const auto &state = context().state;
    for (unsigned i = 0; i < 4; ++i) {
        const auto &channel = state.channels[i];
        text(voltage[i], "%.3f V", channel.voltage);
        text(details[i], "%.3f A\n%.2f W", channel.current, channel.power);
        buttonText(toggle[i], channel.enabled ? "ON / AUS" : "OFF / EIN");
        checked(toggle[i], channel.enabled);
    }
    text(totals, "INPUT POWER          OUTPUT POWER          EFFICIENCY\n%.2f W                       %.2f W                       %.1f %%",
        state.inputPower, state.outputPower, state.efficiency);
}
}

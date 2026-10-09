#include "ui_power_supply.h"
#include "ui.h"

namespace UI {
namespace {
lv_obj_t *fixedValues[3], *fixedButtons[3];
lv_obj_t *variableValue, *variableDetails, *outputButton, *setButton, *limitButton;
void toggle(lv_event_t *event)
{
    auto id = static_cast<PowerId>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    context().power.setEnabled(id, !context().state.channels[id].enabled);
    updatePower();
}
void edit(lv_event_t *event)
{
    numeric(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)) != 0);
}
void step(lv_event_t *event)
{
    const int direction = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    context().power.setVoltage(context().state.channels[POWER_VARIABLE].setVoltage + direction * 0.1f);
    updatePower();
}
}

void buildPower(lv_obj_t *parent)
{
    label(parent, 14, 4, 760, "NETZTEIL / DEMO OUTPUT CONTROL", &lv_font_montserrat_20);
    for (unsigned i = 0; i < 3; ++i) {
        auto card = panel(parent, 12, 38 + i * 104, 257, 96);
        label(card, 12, 8, 84, channelName(static_cast<PowerId>(i)), &lv_font_montserrat_20, cyan);
        fixedValues[i] = label(card, 106, 8, 144, "", &lv_font_montserrat_16);
        fixedButtons[i] = button(card, 10, 40, 86, 46, "OFF", toggle, i);
    }
    auto card = panel(parent, 281, 38, 507, 312);
    label(card, 18, 8, 470, "VARIABLE OUTPUT", &lv_font_montserrat_20, 0xb59aff);
    variableValue = label(card, 18, 38, 450, "", &lv_font_montserrat_40, cyan);
    variableDetails = label(card, 18, 94, 470, "", &lv_font_montserrat_20);
    setButton = button(card, 18, 164, 220, 48, "SET V", edit, 0);
    limitButton = button(card, 252, 164, 235, 48, "LIMIT A", edit, 1);
    button(card, 18, 226, 64, 64, "-", step, -1);
    button(card, 94, 226, 64, 64, "+", step, 1);
    outputButton = button(card, 176, 226, 311, 64, "OUTPUT OFF", toggle, POWER_VARIABLE, 0x256580);
    updatePower();
}

void updatePower()
{
    for (unsigned i = 0; i < 3; ++i) {
        const auto &channel = context().state.channels[i];
        text(fixedValues[i], "%.3f V\n%.3f A\n%.2f W", channel.voltage, channel.current, channel.power);
        buttonText(fixedButtons[i], channel.enabled ? "ON" : "OFF");
        checked(fixedButtons[i], channel.enabled);
    }
    const auto &channel = context().state.channels[POWER_VARIABLE];
    text(variableValue, "%.3f V", channel.voltage);
    text(variableDetails, "CURRENT  %.3f A     POWER  %.2f W\nMODE  %s     ENERGY  %.4f Wh",
        channel.current, channel.power, modeName(channel.mode), channel.energy);
    text(lv_obj_get_child(setButton, 0), "SET: %.3f V", channel.setVoltage);
    text(lv_obj_get_child(limitButton, 0), "LIMIT: %.3f A", channel.currentLimit);
    buttonText(outputButton, channel.enabled ? "OUTPUT ON - AUS" : "OUTPUT OFF - EIN");
    checked(outputButton, channel.enabled);
}
}

#include "ui_graph.h"
#include "ui.h"
#include <math.h>

namespace UI {
namespace {
lv_obj_t *chart, *summary, *axis, *metrics[3], *windows[4];
lv_chart_series_t *series;
unsigned metric = 0;
unsigned windowIndex = 0;
uint32_t lastDraw = 0;
bool dirty = true;
constexpr unsigned points = 120;
const unsigned seconds[] = {30, 60, 300, 1800};
const char *units[] = {"V", "A", "W"};
float extract(const MeasurementData &data)
{
    return metric == 0 ? data.voltage : (metric == 1 ? data.current : data.power);
}
void selectMetric(lv_event_t *event)
{
    metric = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    dirty = true;
    for (unsigned i = 0; i < 3; ++i) checked(metrics[i], i == metric);
}
void selectWindow(lv_event_t *event)
{
    windowIndex = reinterpret_cast<intptr_t>(lv_event_get_user_data(event));
    dirty = true;
    for (unsigned i = 0; i < 4; ++i) checked(windows[i], i == windowIndex);
}
}

void buildGraph(lv_obj_t *parent)
{
    label(parent, 14, 4, 760, "VERLAUF / VARIABLE OUTPUT / DEMO", &lv_font_montserrat_20);
    const char *names[] = {"VOLTAGE", "CURRENT", "POWER"};
    for (unsigned i = 0; i < 3; ++i) metrics[i] = button(parent, 12 + i * 155, 34, 145, 44, names[i], selectMetric, i);
    chart = lv_chart_create(parent);
    lv_obj_set_pos(chart, 66, 90);
    lv_obj_set_size(chart, 710, 168);
    lv_obj_set_style_bg_color(chart, lv_color_hex(panelColor), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(borderColor), LV_PART_MAIN);
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, points);
    lv_chart_set_div_line_count(chart, 5, 7);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);
    series = lv_chart_add_series(chart, lv_color_hex(cyan), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, series, LV_CHART_POINT_NONE);
    axis = label(parent, 8, 106, 55, "", &lv_font_montserrat_14, muted);
    summary = label(parent, 14, 268, 775, "", &lv_font_montserrat_16);
    const char *ranges[] = {"LIVE 30 S", "1 MIN", "5 MIN", "30 MIN"};
    for (unsigned i = 0; i < 4; ++i) windows[i] = button(parent, 12 + i * 196, 304, 186, 48, ranges[i], selectWindow, i);
    checked(metrics[0], true);
    checked(windows[0], true);
}

void updateGraph(uint32_t now)
{
    if (!dirty && now - lastDraw < 1000) return;
    dirty = false;
    lastDraw = now;
    const unsigned span = seconds[windowIndex];
    float maximum = 0;
    float sum = 0;
    unsigned count = 0;
    MeasurementData sample;
    // Statistics use original samples, not resampled chart points.
    for (unsigned age = 0; age <= span; ++age) {
        if (!context().measurements.history(age, sample)) break;
        maximum = fmaxf(maximum, extract(sample));
        sum += extract(sample);
        ++count;
    }
    const float upper = fmaxf(1.0f, ceilf(maximum * 1.15f));
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, static_cast<lv_coord_t>(upper * 100));
    for (unsigned i = 0; i < points; ++i) {
        unsigned age = (points - 1 - i) * span / (points - 1);
        lv_coord_t value = LV_CHART_POINT_NONE;
        if (context().measurements.history(age, sample)) value = static_cast<lv_coord_t>(extract(sample) * 100);
        lv_chart_set_value_by_id(chart, series, i, value);
    }
    text(axis, "%.0f\n%s\n\n\n0", upper, units[metric]);
    text(summary, "Energy %.4f Wh   Peak %.3f %s   Avg %.3f %s   -%us ... jetzt",
        context().state.latest.energy, maximum, units[metric], count ? sum / count : 0, units[metric], span);
    lv_chart_refresh(chart);
}
}

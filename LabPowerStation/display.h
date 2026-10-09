#pragma once
#include <lvgl.h>
#include <esp_display_panel.hpp>

/** Single-task LVGL RGB565 port. Flush completes synchronously for the RGB framebuffer. */
class DisplayManager {
public:
    bool begin(esp_panel::drivers::LCD *lcd);
    /** Advance ticks using real elapsed millis and run LVGL in the Arduino task only. */
    void update(uint32_t now);
    bool healthy() const { return healthy_; }
private:
    static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels);
    esp_panel::drivers::LCD *lcd_ = nullptr;
    lv_disp_draw_buf_t buffer_ = {};
    lv_disp_drv_t driver_ = {};
    uint32_t lastTick_ = 0;
    bool healthy_ = true;
};

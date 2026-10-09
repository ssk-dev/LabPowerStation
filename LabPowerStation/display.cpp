#include "display.h"
#include "config.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

#if LVGL_VERSION_MAJOR != 8
#error "LabPowerStation requires LVGL 8.x (supplied version: 8.4.0)."
#endif
#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 0
#error "Use the supplied lv_conf.h: RGB565, no byte swap."
#endif

bool DisplayManager::begin(esp_panel::drivers::LCD *lcd)
{
    if (!lcd) return false;
    lcd_ = lcd;
    lv_init();
    const unsigned pixels = Config::width * Config::drawLines;
    auto memory = static_cast<lv_color_t *>(heap_caps_malloc(
        pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!memory) return false;
    lv_disp_draw_buf_init(&buffer_, memory, nullptr, pixels);
    lv_disp_drv_init(&driver_);
    driver_.hor_res = Config::width;
    driver_.ver_res = Config::height;
    driver_.draw_buf = &buffer_;
    driver_.flush_cb = flush;
    driver_.user_data = this;
    if (!lv_disp_drv_register(&driver_)) return false;
    lastTick_ = millis();
    return true;
}

void DisplayManager::flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels)
{
    auto self = static_cast<DisplayManager *>(driver->user_data);
    // ESP32_Display_Panel 1.0.0 esp_panel_lcd.hpp explicitly documents that RGB
    // drawBitmap copies into the framebuffer; source memory can be reused immediately.
    if (!self->lcd_->drawBitmap(area->x1, area->y1, area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1, reinterpret_cast<const uint8_t *>(pixels))) self->healthy_ = false;
    lv_disp_flush_ready(driver);
}

void DisplayManager::update(uint32_t now)
{
    lv_tick_inc(now - lastTick_);
    lastTick_ = now;
    lv_timer_handler();
}

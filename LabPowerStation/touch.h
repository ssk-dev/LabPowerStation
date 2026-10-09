#pragma once
#include <lvgl.h>
#include <esp_display_panel.hpp>

/** GT911 adapter. Board owns reset/address selection and the shared I2C host. */
class TouchManager {
public:
    bool begin(esp_panel::drivers::Touch *touch);
    bool healthy() const { return healthy_; }
private:
    static void read(lv_indev_drv_t *driver, lv_indev_data_t *data);
    esp_panel::drivers::Touch *touch_ = nullptr;
    lv_indev_drv_t driver_ = {};
    lv_point_t lastPoint_ = {};
    unsigned errors_ = 0;
    bool healthy_ = true;
};

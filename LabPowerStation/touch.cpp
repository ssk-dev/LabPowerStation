#include "touch.h"
#include "config.h"

bool TouchManager::begin(esp_panel::drivers::Touch *touch)
{
    if (!touch) return false;
    touch_ = touch;
    lv_indev_drv_init(&driver_);
    driver_.type = LV_INDEV_TYPE_POINTER;
    driver_.read_cb = read;
    driver_.user_data = this;
    driver_.long_press_time = 800;
    return lv_indev_drv_register(&driver_) != nullptr;
}

void TouchManager::read(lv_indev_drv_t *driver, lv_indev_data_t *data)
{
    auto self = static_cast<TouchManager *>(driver->user_data);
    esp_panel::drivers::TouchPoint point;
    const int count = self->touch_->readPoints(&point, 1, 0);
    if (count < 0) {
        if (++self->errors_ >= 10) self->healthy_ = false;
    } else {
        self->errors_ = 0;
    }
    const bool valid = count > 0 && point.x >= 0 && point.y >= 0 &&
        point.x < Config::width && point.y < Config::height;
    if (valid) self->lastPoint_ = {static_cast<lv_coord_t>(point.x), static_cast<lv_coord_t>(point.y)};
    data->point = self->lastPoint_;
    data->state = valid ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    data->continue_reading = false;
}

#pragma once

// Source: supplied ESP32_Display_Panel 1.0.0 Waveshare board definition.
// Original ESP32-S3-Touch-LCD-4.3, NOT the 4.3B board.
#define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM 1
#include <board/supported/waveshare/BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3.h>

// The supplied Arduino/examples/09_lvgl_Porting custom configuration uses
// RGB565 here, unlike the library's built-in RGB888 setting. Follow the demo.
#undef ESP_PANEL_BOARD_LCD_COLOR_BITS
#define ESP_PANEL_BOARD_LCD_COLOR_BITS ESP_PANEL_LCD_COLOR_BITS_RGB565

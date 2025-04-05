// src/lv_tick_custom.cpp
#include <Arduino.h>
#include <lvgl.h>

#if LV_TICK_CUSTOM
uint32_t lv_tick_get_ms() { return millis(); }
#endif
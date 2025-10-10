#pragma once
#include <lvgl.h>

// Call once after lv_init() and display driver registration
void touch_inject_init();

// Simulate a tap at x,y for ms_hold milliseconds
void touch_inject_press(int16_t x, int16_t y, uint16_t ms_hold = 50);

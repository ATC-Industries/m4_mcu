#ifndef LVGL_CALLBACKS_H
#define LVGL_CALLBACKS_H

#include <Arduino.h>
#include <lvgl/lvgl.h>

#include "display/display.h"
#include "touch/touch.h"

// LVGL buffers and drivers
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[800 * 10];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// Display flushing callback for LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);

// Touchpad reading callback for LVGL
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

void lvgl_task(void *parameter);

// Initialize LVGL
void init_lvgl();

#endif  // LVGL_CALLBACKS_H
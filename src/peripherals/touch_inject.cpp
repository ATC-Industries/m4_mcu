#include "peripherals/touch_inject.h"

#if LVGL_VERSION_MAJOR >= 9
// LVGL v9
static lv_indev_t* s_inj = nullptr;
#else
// LVGL v8
static lv_indev_drv_t s_drv;     // keep the driver storage alive
static lv_indev_t*     s_inj = nullptr;
#endif

static volatile bool s_pressed = false;
static lv_point_t s_pt = {0, 0};

#if LVGL_VERSION_MAJOR >= 9
static void inj_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
  LV_UNUSED(indev);
  data->point = s_pt;
  data->state = s_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
#else
static void inj_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  LV_UNUSED(drv);
  data->point = s_pt;
  data->state = s_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
#endif

static void release_timer_cb(lv_timer_t* t) {
  s_pressed = false;
  lv_timer_del(t);
}

void touch_inject_init() {
#if LVGL_VERSION_MAJOR >= 9
  static lv_indev_drv_t drv;
  lv_indev_drv_init(&drv);
  drv.type = LV_INDEV_TYPE_POINTER;
  drv.read_cb = inj_read_cb;
  s_inj = lv_indev_drv_register(&drv);
  LV_UNUSED(s_inj);
#else
  lv_indev_drv_init(&s_drv);
  s_drv.type = LV_INDEV_TYPE_POINTER;
  s_drv.read_cb = inj_read_cb;
  s_inj = lv_indev_drv_register(&s_drv);
  LV_UNUSED(s_inj);
#endif
}

void touch_inject_press(int16_t x, int16_t y, uint16_t ms_hold) {
  s_pt.x = x;
  s_pt.y = y;
  s_pressed = true;

  lv_timer_t* t = lv_timer_create(release_timer_cb, ms_hold, nullptr);
  lv_timer_set_repeat_count(t, 1);
}

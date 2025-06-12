#include "display/lvgl_callbacks.h"
#include "Config.h"

//========================================================================
// LVGL Callbacks
//========================================================================

// Display flushing callback for LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  lcd.startWrite();
  lcd.setAddrWindow(area->x1, area->y1, w, h);
  lcd.pushPixels((uint16_t *)color_p, w * h);
  lcd.endWrite();

  lv_disp_flush_ready(disp);
}

// Touchpad reading callback for LVGL
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  uint16_t touchX, touchY, touchZ;
  bool touched = touch.readTouchPoint(&touchX, &touchY, &touchZ);

  if (touched) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touchX;
    data->point.y = touchY;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

//========================================================================
// Core System Functions
//========================================================================

// LVGL task handler
void lvgl_task(void *parameter) {
  while (1) {
    lv_timer_handler();
    delay(5);
  }
}

// Initialize LVGL
void init_lvgl() {
  lv_init();

  // Initialize display buffer
  // lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 800 * 10);
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 40);

  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  lv_disp_drv_register(&disp_drv);

  // Initialize input device driver
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

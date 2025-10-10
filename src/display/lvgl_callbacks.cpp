#include "display/lvgl_callbacks.h"

#include <TFT_Touch.h>

#include "Config.h"
#include "touch/touch.h"
#include "StateManager.h"

extern TFT_Touch touch;

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

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  static uint16_t last_x = 0, last_y = 0;
  static bool was_pressed = false;
  static uint8_t no_touch_counter = 0;
  const uint8_t NO_TOUCH_THRESHOLD = 10;  // Require 10 consecutive no-touch samples

  bool currently_pressed = touch.Pressed();

  if (currently_pressed) {
    // Touch detected - reset counter
    no_touch_counter = 0;
    last_x = touch.X();
    last_y = touch.Y();
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PRESSED;

    if (!was_pressed) {
      Serial.print("Touch pressed at: ");
      Serial.print(last_x);
      Serial.print(", ");
      Serial.println(last_y);
    }
    was_pressed = true;
  } else {
    // No touch detected
    data->point.x = last_x;
    data->point.y = last_y;

    if (was_pressed) {
      no_touch_counter++;
      if (no_touch_counter >= NO_TOUCH_THRESHOLD) {
        // Confirmed release
        data->state = LV_INDEV_STATE_RELEASED;
        Serial.println("Touch released");
        was_pressed = false;
        no_touch_counter = 0;
      } else {
        // Still counting - maintain pressed state
        data->state = LV_INDEV_STATE_PRESSED;
      }
    } else {
      data->state = LV_INDEV_STATE_RELEASED;
    }
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
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);

  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;

  // Enable software rotation
  disp_drv.sw_rotate = 1;
  bool rotation180 = StateManager::getScreenRotation();
  disp_drv.rotated = rotation180 ? LV_DISP_ROT_180 : LV_DISP_ROT_NONE;;
  disp_drv.full_refresh = 1;

  lv_disp_drv_register(&disp_drv);

  // Initialize input device driver
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

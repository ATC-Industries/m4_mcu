#include "display/lvgl_callbacks.h"

#include <TFT_Touch.h>
#include "esp_heap_caps.h"

#include "Config.h"
#include "touch/touch.h"
#include "StateManager.h"

#define LOG_TAG "LVGLCallbacks"
#define LOG_DEBUG_DISABLE true
#include "Logging.h"

static lv_color_t *buf1 = nullptr;

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
  (void)indev_drv;

  static uint16_t last_x = 0;
  static uint16_t last_y = 0;
  static bool was_pressed = false;

  uint16_t x = 0;
  uint16_t y = 0;

  if (readTouchMapped(&x, &y)) {
    last_x = x;
    last_y = y;

    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_PRESSED;

    if (!was_pressed) {
      LOGD("Touch pressed at: %d, %d", last_x, last_y);
    }

    was_pressed = true;
  } else {
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_RELEASED;
    was_pressed = false;
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

  // Allocate draw buffer in internal DMA capable RAM
  const uint32_t buf_lines = 40;                       // tweak if you want
  const uint32_t buf_pixels = SCREEN_WIDTH * buf_lines;

  buf1 = (lv_color_t *) heap_caps_malloc(
      buf_pixels * sizeof(lv_color_t),
      MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (!buf1) {
    LOGE("Failed to allocate LVGL draw buffer");
    while (true) {
      delay(1000);  // hard fail, nothing good will happen without this
    }
  }

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buf_pixels);

  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;

  // Enable software rotation
  disp_drv.sw_rotate = 1;
  disp_drv.rotated = StateManager::getScreenRotation() ? LV_DISP_ROT_180 : LV_DISP_ROT_NONE;
  disp_drv.full_refresh = 1;

  lv_disp_drv_register(&disp_drv);

  // Initialize input device driver
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
}

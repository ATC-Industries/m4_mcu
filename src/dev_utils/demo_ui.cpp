#include "dev_utils/demo_ui.h"

#include "dev_utils/benchmark.h"
#include "display/backlight.h"

// Demo UI elements
static uint8_t brightness = 128;  // Start at 50%
static lv_obj_t *brightness_label;

//========================================================================
// Demo UI Functions
//========================================================================

// Slider event handler
static void slider_event_handler(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  brightness = (uint8_t)lv_slider_get_value(slider);

  // Update the backlight
  setBacklight(brightness);

  // Update label
  lv_label_set_text_fmt(brightness_label, "Brightness: %d%%", (brightness * 100) / 255);
}

// Color button event handler
static void color_btn_event_handler(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  int btn_id = (int)lv_obj_get_user_data(btn);

  lv_obj_t *cont = lv_obj_get_parent(btn);

  if (btn_id == 0) {
    // Dark Blue background
    lv_obj_set_style_bg_color(cont, lv_color_make(0, 0, 64), LV_PART_MAIN);
  } else {
    // White background
    lv_obj_set_style_bg_color(cont, lv_color_white(), LV_PART_MAIN);
  }
}

// Checkbox event handler
static void checkbox_event_handler(lv_event_t *e) {
  lv_obj_t *cb = lv_event_get_target(e);
  bool enabled = lv_obj_get_state(cb) & LV_STATE_CHECKED;
  benchmark_set_enabled(enabled);
}

void demo_ui_init() {
  // Initialize with default brightness
  brightness = 128;
}

// Create the demo UI
void create_demo_ui() {
  // Create the main container that will hold our UI elements
  lv_obj_t *cont = lv_obj_create(lv_scr_act());
  lv_obj_set_size(cont, 800, 480);
  lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_pad_all(cont, 10, LV_PART_MAIN);

  // Create title at the top
  lv_obj_t *title = lv_label_create(cont);
  lv_label_set_text(title, "ESP32-S3 LVGL Demo");
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

  // Create brightness label
  brightness_label = lv_label_create(cont);
  lv_obj_align(brightness_label, LV_ALIGN_TOP_MID, 0, 60);
  lv_label_set_text_fmt(brightness_label, "Brightness: %d%%", (brightness * 100) / 255);

  // Create brightness slider
  lv_obj_t *slider = lv_slider_create(cont);
  lv_obj_set_width(slider, 400);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 100);
  lv_slider_set_range(slider, 0, 255);
  lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
  lv_obj_add_event_cb(slider, slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

  // Create color change buttons
  lv_obj_t *btn_blue = lv_btn_create(cont);
  lv_obj_set_size(btn_blue, 150, 60);
  lv_obj_align(btn_blue, LV_ALIGN_LEFT_MID, 50, 50);
  lv_obj_add_event_cb(btn_blue, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn_blue, (void *)0);  // ID 0 for blue

  // Label for the blue button
  lv_obj_t *blue_label = lv_label_create(btn_blue);
  lv_label_set_text(blue_label, "Dark Blue");
  lv_obj_center(blue_label);

  // Create the "White" button
  lv_obj_t *btn_white = lv_btn_create(cont);
  lv_obj_set_size(btn_white, 150, 60);
  lv_obj_align(btn_white, LV_ALIGN_RIGHT_MID, -50, 50);
  lv_obj_add_event_cb(btn_white, color_btn_event_handler, LV_EVENT_CLICKED, NULL);
  lv_obj_set_user_data(btn_white, (void *)1);  // ID 1 for white

  // Label for the white button
  lv_obj_t *white_label = lv_label_create(btn_white);
  lv_label_set_text(white_label, "White");
  lv_obj_center(white_label);

  // Create benchmark checkbox
  lv_obj_t *cb = lv_checkbox_create(cont);
  lv_checkbox_set_text(cb, "Show Benchmark");
  lv_obj_align(cb, LV_ALIGN_BOTTOM_MID, 0, -20);
  lv_obj_add_event_cb(cb, checkbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
}
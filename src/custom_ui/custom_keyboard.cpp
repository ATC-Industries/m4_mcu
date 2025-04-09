#include "custom_ui/custom_keyboard.h"

// Static styles
static lv_style_t style_green_btn;
static lv_style_t style_red_btn;
static bool styles_initialized = false;

// Static keymap and ctrl_map
static const char *custom_keymap[] = {
    "1",  "2",          "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", ".", "0", LV_SYMBOL_BACKSPACE,
    "\n", LV_SYMBOL_OK, "",  "",   ""  // ENTER (3-wide) + fillers
};

static const lv_btnmatrix_ctrl_t custom_ctrl_map[] = {
    1,
    1,
    1,  // Row 1
    1,
    1,
    1,  // Row 2
    1,
    1,
    1,  // Row 3
    1,
    1,
    1 | LV_BTNMATRIX_CTRL_NO_REPEAT,  // Row 4
    3 | LV_BTNMATRIX_CTRL_NO_REPEAT,
    0,
    0  // Row 5
};

// Custom draw event for coloring buttons
static void keyboard_draw_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_DRAW_PART_BEGIN) return;

  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part != LV_PART_ITEMS) return;

  const char *txt = lv_btnmatrix_get_btn_text(e->current_target, dsc->id);
  if (!txt) return;

  if (strcmp(txt, LV_SYMBOL_OK) == 0) {
    dsc->rect_dsc->bg_color = lv_color_hex(0x1FA709);  // green
  } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    dsc->rect_dsc->bg_color = lv_color_hex(0xA70909);  // red
  }
}

// Public setup function
void setup_custom_number_keyboard(lv_obj_t *keyboard) {
  // Initialize styles once
  if (!styles_initialized) {
    styles_initialized = true;

    lv_style_init(&style_green_btn);
    lv_style_set_bg_color(&style_green_btn, lv_color_hex(0x1FA709));
    lv_style_set_bg_grad_color(&style_green_btn, lv_color_hex(0x1FA709));

    lv_style_init(&style_red_btn);
    lv_style_set_bg_color(&style_red_btn, lv_color_hex(0xA70909));
    lv_style_set_bg_grad_color(&style_red_btn, lv_color_hex(0xA70909));
  }

  // Set custom layout
  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_NUMBER, custom_keymap, custom_ctrl_map);

  // Apply event callback for coloring
  lv_obj_add_event_cb(keyboard, keyboard_draw_event_cb, LV_EVENT_ALL, NULL);
}

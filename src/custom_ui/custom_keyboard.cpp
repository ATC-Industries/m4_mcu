#include "custom_ui/custom_keyboard.h"
#include <string.h>

// Static styles
static lv_style_t style_green_btn;
static lv_style_t style_red_btn;
static bool styles_initialized = false;

// ========== Number pad (existing) ==========
static const char *custom_keymap[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", "\n",
    "7", "8", "9", "\n",
    ".", "0", LV_SYMBOL_BACKSPACE, "\n",
    LV_SYMBOL_OK, "", "", ""  // OK spans 3 columns (via ctrl map)
};

static const lv_btnmatrix_ctrl_t custom_ctrl_map[] = {
    // Row 1
    1, 1, 1,
    // Row 2
    1, 1, 1,
    // Row 3
    1, 1, 1,
    // Row 4
    1, 1, 1 | LV_BTNMATRIX_CTRL_NO_REPEAT,
    // Row 5
    3 | LV_BTNMATRIX_CTRL_NO_REPEAT, 0, 0, 0   // width 3, no repeat (your original style)
};

// ========== Hex pad (4 x 5) ==========
static const char *custom_hex_keymap[] = {
    "1", "2", "3", "4", "\n",
    "5", "6", "7", "8", "\n",
    "9", "0", "A", "B", "\n",
    "C", "D", "E", "F", "\n",
    LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK, "", "", ""  // last row: backspace + OK(3-wide)
};

static const lv_btnmatrix_ctrl_t custom_hex_ctrl_map[] = {
    // Row 1
    1, 1, 1, 1,
    // Row 2
    1, 1, 1, 1,
    // Row 3
    1, 1, 1, 1,
    // Row 4
    1, 1, 1, 1,
    // Row 5
    LV_BTNMATRIX_CTRL_NO_REPEAT,                 // backspace single width, no repeat
    3 | LV_BTNMATRIX_CTRL_NO_REPEAT,            // OK spans 3, no repeat (same literal style)
    0,                                          // filler
    0                                           // filler
};

// ========== Shared draw callback for coloring special keys ==========
static void keyboard_draw_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_DRAW_PART_BEGIN) return;

  lv_obj_draw_part_dsc_t *dsc = (lv_obj_draw_part_dsc_t *)lv_event_get_param(e);
  if (dsc->part != LV_PART_ITEMS) return;

  const char *txt = lv_btnmatrix_get_btn_text((lv_obj_t *)e->current_target, dsc->id);
  if (!txt) return;

  if (strcmp(txt, LV_SYMBOL_OK) == 0) {
    dsc->rect_dsc->bg_color = lv_color_hex(0x1FA709);  // green
  } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
    dsc->rect_dsc->bg_color = lv_color_hex(0xA70909);  // red
  }
}

// ========== Public setup functions ==========
void setup_custom_number_keyboard(lv_obj_t *keyboard) {
  if (!styles_initialized) {
    styles_initialized = true;

    lv_style_init(&style_green_btn);
    lv_style_set_bg_color(&style_green_btn, lv_color_hex(0x1FA709));
    lv_style_set_bg_grad_color(&style_green_btn, lv_color_hex(0x1FA709));

    lv_style_init(&style_red_btn);
    lv_style_set_bg_color(&style_red_btn, lv_color_hex(0xA70909));
    lv_style_set_bg_grad_color(&style_red_btn, lv_color_hex(0xA70909));
  }

  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_NUMBER, custom_keymap, custom_ctrl_map);
  lv_obj_add_event_cb(keyboard, keyboard_draw_event_cb, LV_EVENT_ALL, NULL);

  // Optional: force number mode immediately
  // lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
}

void setup_custom_hex_keyboard(lv_obj_t *keyboard) {
  if (!styles_initialized) {
    styles_initialized = true;

    lv_style_init(&style_green_btn);
    lv_style_set_bg_color(&style_green_btn, lv_color_hex(0x1FA709));
    lv_style_set_bg_grad_color(&style_green_btn, lv_color_hex(0x1FA709));

    lv_style_init(&style_red_btn);
    lv_style_set_bg_color(&style_red_btn, lv_color_hex(0xA70909));
    lv_style_set_bg_grad_color(&style_red_btn, lv_color_hex(0xA70909));
  }

  // Use a user mode so we do not overwrite built-in layouts
  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_1, custom_hex_keymap, custom_hex_ctrl_map);
  lv_obj_add_event_cb(keyboard, keyboard_draw_event_cb, LV_EVENT_ALL, NULL);

  // Activate the hex layout
  lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_1);
}

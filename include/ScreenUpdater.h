#ifndef SCREEN_UPDATER_H
#define SCREEN_UPDATER_H

#define COLOR_INDIC_GREEN lv_color_make(0x00, 0xC8, 0x53)
#define COLOR_INDIC_RED lv_color_make(0xF9, 0x2A, 0x1C)
#define COLOR_INDIC_DISABLED lv_color_make(0x80, 0x80, 0x80)
#define COLOR_INDIC_YELLOW lv_color_make(0xFF, 0xE0, 0x00)

void updateMainScreen();  // Call this periodically to update UI elements based on state

#endif  // SCREEN_UPDATER_H

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <Arduino.h>

// Backlight control definitions
#define LCD_BL_PIN 2
#define LCD_BL_FREQ 1000
#define LCD_BL_CHANNEL 0
#define LCD_BL_RESOLUTION 8

/**
 * @brief Sets the backlight brightness of the display.
 *
 * This function adjusts the brightness level of the display's backlight.
 *
 * @param brightness The desired brightness level, ranging from 0 (off) to 255 (maximum brightness).
 */
void setBacklight(uint8_t brightness);


/**
 * @brief Retrieves the current backlight level.
 * 
 * @return uint8_t The current backlight level as an 8-bit unsigned integer.
 */
uint8_t getBacklight();

/**
 * @brief Fades the backlight brightness of the display.
 *
 * This function fades the brightness level of the display's backlight.
 *
 * @param fadeIn True to fade in, false to fade out.
 * @param duration The duration of the fade effect in milliseconds.
 */
void fadeBacklight(bool fadeIn, int duration = 1000);

#endif  // BACKLIGHT_H

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <Arduino.h>

// Backlight control definitions
#define LCD_BL_PIN 2
#define LCD_BL_FREQ 10000  // Increased to 10kHz for smoother PWM
#define LCD_BL_CHANNEL 0
#define LCD_BL_RESOLUTION 8

void setupBacklight(uint8_t pin = LCD_BL_PIN, uint8_t channel = LCD_BL_CHANNEL, uint8_t initial_brightness = 200);
void setBacklight(uint8_t brightness);
void setBacklightSmooth(uint8_t brightness);
void setBacklightFast(uint8_t brightness);
void updateBacklight();
uint8_t getBacklight();
void fadeBacklight(bool fadeIn, int duration = 1000);

#endif  // BACKLIGHT_H
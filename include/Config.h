#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "0.0.2-alpha"
#define SCREEN_UPDATE_INTERVAL_MS 200
#define DEFAULT_BACKLIGHT_BRIGHTNESS 192
#define SCREEN_UPDATE_INTERVAL_MS 200
#define DEFAULT_BACKLIGHT_BRIGHTNESS 192
#define SCREEN_ROTATION 0U  // Display rotation (0 = no rotation, 1 = 90 degrees, etc.)
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Pin definitions for XPT2046 Touch Chip
#define TOUCH_CS_PIN 38
#define TOUCH_MOSI_PIN 11
#define TOUCH_MISO_PIN 13
#define TOUCH_SCK_PIN 12
#define TOUCH_SPI_BUS 2  // or HSPI or VSPI if already defined

// Enable this for development-only features like fake pulls
// #define DEVELOPMENT_MODE

// Calibration number boundaries
#define CALIBRATION_MIN 250
#define CALIBRATION_MAX 25000

// Factory-default calibration values (pulses per 300 ft)
#define RADAR_CALIBRATION_PULSES 3542
#define GPS_CALIBRATION_PULSES 3780

// Speed input pin
#define SPEED_SENSOR_PIN 44

#endif  // CONFIG_H

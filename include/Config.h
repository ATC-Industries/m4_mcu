#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "0.0.1-alpha"
#define SCREEN_UPDATE_INTERVAL_MS 200
#define DEFAULT_BACKLIGHT_BRIGHTNESS 192

// Enable this for development-only features like fake pulls
// #define DEVELOPMENT_MODE

// Calibration number boundaries
#define CALIBRATION_MIN 250
#define CALIBRATION_MAX 25000

// Factory-default calibration values (pulses per 300 ft)
#define RADAR_CALIBRATION_PULSES 3542
#define GPS_CALIBRATION_PULSES 3780

// Speed input pin
#define SPEED_SENSOR_PIN 20

#endif  // CONFIG_H

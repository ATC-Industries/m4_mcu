#ifndef CONFIG_H
#define CONFIG_H

// SYSTEM SETTINGS
#define SERIAL_BAUD_RATE 115200
#define VERSION "0.0.4-alpha"

// DISPLAY SETTINGS
#define SCREEN_UPDATE_INTERVAL_MS 200  // Interval for screen updates in milliseconds
#define SCREEN_ROTATION 2              // Display rotation (0 = no rotation, 1 = 90 degrees, etc.)
#define SCREEN_WIDTH 800               // Display width in pixels
#define SCREEN_HEIGHT 480              // Display height in pixels
// For Display Pin Settings see include/display/display.h

// BACKLIGHT SETTINGS
#define LCD_BL_FREQ 10000                 // Increased to 10kHz for smoother PWM
#define LCD_BL_CHANNEL 0                  // PWM channel for backlight control
#define LCD_BL_RESOLUTION 8               // 8-bit resolution (0-255)
#define DEFAULT_BACKLIGHT_BRIGHTNESS 192  // Default backlight brightness (0-255)
#define LCD_BL_PIN GPIO_NUM_2             // Backlight control pin

// TOUCH SETTINGS
#define TOUCH_CS_PIN GPIO_NUM_38    // Touch CS pin
#define TOUCH_MOSI_PIN GPIO_NUM_11  // Touch MOSI pin
#define TOUCH_MISO_PIN GPIO_NUM_13  // Touch MISO pin
#define TOUCH_SCK_PIN GPIO_NUM_12   // Touch SCK pin

// Enable this for development-only features like fake pulls
// #define DEVELOPMENT_MODE

// Speed Calibration number boundaries
#define CALIBRATION_MIN 250
#define CALIBRATION_MAX 25000

// Factory-default calibration values (pulses per 300 ft)
#define RADAR_CALIBRATION_PULSES 3542
#define GPS_CALIBRATION_PULSES 3780

// Speed input pin
#define SPEED_SENSOR_PIN GPIO_NUM_44  // TODO: Where is 44 coming from? I don't see it on the schematic

// OTHER PIN DEFINITIONS
#define NO_CONNECTION_1 GPIO_NUM_35
#define NO_CONNECTION_2 GPIO_NUM_36
#define NO_CONNECTION_3 GPIO_NUM_37
#define IO42 GPIO_NUM_42
#define IO17 GPIO_NUM_17
#define IO18 GPIO_NUM_18
#define TF_CS GPIO_NUM_10
#define I2C_SDA GPIO_NUM_19
#define I2C_SCL GPIO_NUM_20

#endif  // CONFIG_H

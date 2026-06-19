#ifndef CONFIG_H
#define CONFIG_H

// Forcefully undefine the library's defaults
#undef DEVICE_TYPE
#undef DEFAULT_DEVICE_NAME

#define DEVICE_TYPE M4_MAIN_CONTROLLER
#define DEFAULT_DEVICE_NAME "M4 Sled Monitor"

// Include Dependencies
#include "M4Device.h"  // Include the M4Device header for the device_type enum
#include "M4Callbacks.h"
#include "M4CommProtocol.h"
#include "M4OTAServer.h"
#include "M4PairingControl.h"

// SYSTEM SETTINGS
#define SERIAL_BAUD_RATE 115200
#define VERSION_MAJOR "0"
#define VERSION_MINOR "0"
#define VERSION_PATCH "5"
#define VERSION_PRERELEASE "alpha"  // Comment out or undefine for stable releases

#ifdef VERSION_PRERELEASE
    #define DEVICE_VERSION VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH "-" VERSION_PRERELEASE
#else
    #define DEVICE_VERSION VERSION_MAJOR "." VERSION_MINOR "." VERSION_PATCH
#endif

// DISPLAY SETTINGS
#define SCREEN_UPDATE_INTERVAL_MS 200        // Interval for screen updates in milliseconds
#define SCREEN_ROTATION 0U                   // Display rotation (0 = no rotation, 1 = 90 degrees, etc.)
#define SCREEN_WIDTH 800                     // Display width in pixels
#define SCREEN_HEIGHT 480                    // Display height in pixels
// For Display Pin Settings see include/display/display.h

// BACKLIGHT SETTINGS
#define LCD_BL_FREQ 10000                 // Increased to 10kHz for smoother PWM
#define LCD_BL_CHANNEL 0                  // PWM channel for backlight control
#define LCD_BL_RESOLUTION 8               // 8-bit resolution (0-255)
#define DEFAULT_BACKLIGHT_BRIGHTNESS 192  // Default backlight brightness (0-255)
#define LCD_BL_PIN IO_LCD_BL_CTR             // Backlight control pin

// TOUCH SETTINGS
#define TOUCH_CS_PIN IO_TP_CS    // Touch CS pin
#define TOUCH_MOSI_PIN IO_TP_IN  // Touch MOSI pin
#define TOUCH_MISO_PIN IO_TP_OUT  // Touch MISO pin
#define TOUCH_SCK_PIN IO_TP_CLK   // Touch SCK pin

// Enable this for development-only features like fake pulls
// #define DEVELOPMENT_MODE

// Speed Calibration number boundaries
#define CALIBRATION_MIN 250
#define CALIBRATION_MAX 25000

// Factory-default calibration values (pulses per 300 ft)
#define RADAR_CALIBRATION_PULSES 3542
#define GPS_CALIBRATION_PULSES 3780

// Speed input pin
#define SPEED_SENSOR_PIN IO_SENSOR  // This is one of the RX TX pins and the speed sensor needs to be disconeected before burning.

// PULL HISTORY SETTINGS
#define MAX_PULL_HISTORY 50  // Maximum number of pull results to store

// Pin configuration for 800x480 RGB display
#define IO_LCD_BL_CTR  GPIO_NUM_4
#define IO_INTA        GPIO_NUM_5
#define IO_INTB        GPIO_NUM_6
#define IO_SENSOR      GPIO_NUM_7
#define IO_BUTTON      GPIO_NUM_15
#define IO_TP_OUT      GPIO_NUM_16
#define IO_TP_IN       GPIO_NUM_17
#define IO_TP_CS       GPIO_NUM_18
#define IO_TP_CLK      GPIO_NUM_8
#define IO_I2C_SDA     GPIO_NUM_19
#define IO_I2C_SCL     GPIO_NUM_20
#define IO_VSYNC       GPIO_NUM_3
#define IO_HSYNC       GPIO_NUM_46
#define IO_PCLK        GPIO_NUM_9
#define IO_B7          GPIO_NUM_10
#define IO_B6          GPIO_NUM_11
#define IO_B5          GPIO_NUM_12
#define IO_B4          GPIO_NUM_13
#define IO_B3          GPIO_NUM_14
#define IO_G7          GPIO_NUM_21
#define IO_G6          GPIO_NUM_47
#define IO_G5          GPIO_NUM_48
#define IO_G4          GPIO_NUM_45
#define IO_DE_BOOT     GPIO_NUM_0
#define IO_G3          GPIO_NUM_38
#define IO_G2          GPIO_NUM_39
#define IO_R7          GPIO_NUM_40
#define IO_R6          GPIO_NUM_41
#define IO_R5          GPIO_NUM_42
#define IO_R4          GPIO_NUM_2
#define IO_R3          GPIO_NUM_1


// OTHER PIN DEFINITIONS
// #define NO_CONNECTION_1 GPIO_NUM_35
// #define NO_CONNECTION_2 GPIO_NUM_36
// #define NO_CONNECTION_3 GPIO_NUM_37
// #define IO42 GPIO_NUM_42
// #define IO17 GPIO_NUM_17
// #define IO18 GPIO_NUM_18
// #define TF_CS IO_TP_CS
// #define I2C_SDA IO_I2C_SDA
// #define I2C_SCL IO_I2C_SCL

// I2C
#define I2C_SDA_PIN IO_I2C_SDA   // from your new pin map
#define I2C_SCL_PIN IO_I2C_SCL

// MCP23017 address from A2 A1 A0 straps (base 0x20)
#define MCP23017_ADDR 0x20       // update if A0..A2 not 000

// Horn on MCP23017 GPA7
#define MCP_HORN_PORTA_PIN 7     // 0..7 map to GPA0..GPA7
#define MCP_HORN_ACTIVE_HIGH 1   

// Main Button
#define BUTTON_ACTIVE_LOW 1


#endif  // CONFIG_H

# __UI_PROJECT_NAME__

An ESP32-S3 based HMI display controller board for 7" 800x480 RGB display with LVGL 8.3.11 support and resistive touch. This project provides drivers and examples for using the M4 display with Squareline Studio.

## Proprietary Notice

This software is proprietary and confidential. All rights reserved.
Copyright © 2024 ATC Industries.

__NO LICENSE IS GRANTED__, this code is private property of ATC Industries.
Any unauthorized copying, modification, distribution, or use is strictly prohibited.

## Contact Information

### Author

* Name: Adam Clarkson
* Email: [adam@agri-tronix.com](mailto:adam@agri-tronix.com)

### Company

* Name: ATC Industries
* Address: 2001 N Morton St, Franklin, IN 46131, United States
* Phone: [+1 317-738-4474](tel:+13177384474)

## Hardware Specifications

* Display: 7.0 inch TFT-LCD (800x480)
* Processor: ESP32-S3-WROOM-1U
* Touch: Resistive (XPT2046)
* Display Driver: EK9716BD3 & EK73002ACGB
* Power: DC 5V-2A
* Interfaces: UART, GPIO, I2C, Battery

## Features

* RGB parallel display interface for fast refresh rates
* XPT2046 resistive touch controller with calibration
* LVGL UI library support
* Squareline Studio compatibility
* PlatformIO project setup

## Getting Started

### Development Requirements

* VSCode with PlatformIO
* LVGL 8.3.11
* ESP32 Arduino Framework
* ESP32_Display_Panel library

### Squareline Studio Integration

This repository includes an Open Board Package (OBP) for Squareline Studio, allowing you to easily design UIs for the M4 display.

To use the M4 display in Squareline Studio:

1. Install the OBP package by copying the provided folder to your Squareline boards directory
2. Create a new project selecting the "M4 7-inch RGB Display" board
3. Design your UI in Squareline Studio
4. Export the project to use with this board's drivers

## Project Structure

```bash
m4-7inch-rgb-display/
├── .gitignore
├── platformio.ini          # PlatformIO configuration
├── README.md
├── include/                # Header files
│   ├── display/            # Display driver headers
│   ├── touch/              # Touch driver headers
│   └── lv_conf.h           # LVGL configuration
├── src/                    # Source files
│   ├── main.cpp            # Main application
│   ├── display/            # Display implementations
│   ├── touch/              # Touch implementations
│   └── ui/                 # UI-related code (for Squareline output)
└── squareline/             # Squareline Studio OBP package
    └── M4_7INCH_OBP/       # Board package files
```

## Built Ins

Changing the brightness of the display can be done using a set of built in functions. The display brightness can be set to a value between 0 and 255.

### Example of ui_events for brightness slider

1. Create a slider with value 0-254 (or 1-254 if you don't ever want to turn off the display)
2. Add and event to the slider on value change.
3. In the event, call the built in function `set_display_brightness` with the value of the slider.

```cpp
void changeBrightness(lv_event_t *e) {
  lv_obj_t *slider = lv_event_get_target(e);
  uint8_t brightness = (uint8_t)lv_slider_get_value(slider);

  // Update the backlight
  setBacklight(brightness);

  // Update label
  lv_label_set_text_fmt(ui_brightnesslabel, "Brightness: %d%%", (brightness * 100) / 255);
}
```

When loading the setting screen you can set the initial value of the slider to the current brightness.

```cpp
  if (ui_brightness_slider != NULL) {
    lv_slider_set_value(ui_brightness_slider, display_get_brightness(), LV_ANIM_OFF);
  }
```

## Style Guide

Following Google C++ Style Guide:

* Header files use `.h` extension
* Include guards use `#ifndef PROJECT_PATH_FILE_H_`
* Forward declarations preferred over includes
* Class members initialization in constructors
* Public before private in declarations
* Proper naming conventions (e.g., kConstant for constants)

## Support

For any inquiries about this project, please contact:

* Name: Adam Clarkson
* Email: [adam@agri-tronix.com](mailto:adam@agri-tronix.com)
* Phone: [+1 317-738-4474](tel:+13177384474)

---

Copyright © 2024 ATC Industries. All rights reserved.

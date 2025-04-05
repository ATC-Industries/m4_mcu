#ifndef DEMO_UI_H
#define DEMO_UI_H

#include <Arduino.h>
#include <lvgl.h>

/**
 * @brief Demo UI module
 *
 * This module provides a simple demonstration UI for testing the display
 * and LVGL functionality. It includes brightness control, color change buttons,
 * and benchmark functionality.
 *
 * When creating your own application:
 * 1. You can use this as a reference for how to create LVGL UI elements
 * 2. You can modify this code to suit your needs
 * 3. Or you can create your own UI module and replace the call to create_demo_ui()
 *    in the main.cpp file
 */

/**
 * @brief Initialize the demo UI module
 * Call this once during setup
 */
void demo_ui_init();

/**
 * @brief Create the demo UI elements
 * Creates sliders, buttons, and other demo components
 */
void create_demo_ui();

#endif  // DEMO_UI_H
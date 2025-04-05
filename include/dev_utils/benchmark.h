#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <Arduino.h>
#include <lvgl.h>

/**
 * @brief Initialize the benchmark module
 * Call this once during setup
 */
void benchmark_init();

/**
 * @brief Toggle the benchmark display on or off
 * @param enable True to enable, false to disable
 */
void benchmark_set_enabled(bool enable);

/**
 * @brief Check if benchmark display is currently enabled
 * @return True if enabled, false if disabled
 */
bool benchmark_is_enabled();

/**
 * @brief Update the benchmark display
 * Call this in your main loop or LVGL task
 */
void benchmark_update();

#endif  // BENCHMARK_H
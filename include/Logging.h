#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/*
 * Lightweight project logging for ESP32 / Arduino builds.
 *
 * What this gives you:
 * - A consistent log format across the project.
 * - Log levels so release builds can hide noisy debug messages.
 * - Optional per-file tags so each message says what module it came from.
 * - A single place to change output formatting later if needed.
 *
 * Basic usage:
 *   LOGE("Something failed: %d", code);
 *   LOGW("Using fallback value");
 *   LOGI("System started");
 *   LOGD("Raw sensor=%d", value);
 *
 * Important:
 * - Do NOT include a trailing newline in the format string.
 *   The logger adds the newline for you.
 * - `LOGD` is for developer-only detail.
 * - `LOGI` is for normal high-level status.
 * - `LOGW` is for unusual but non-fatal conditions.
 * - `LOGE` is for real failures or clearly bad states.
 *
 * Optional per-file tag:
 *   Put this near the top of a .cpp file before including Logging.h:
 *
 *     #define LOG_TAG "AlarmManager"
 *     #include "Logging.h"
 *
 *   Then `LOGI("Loaded")` will print something like:
 *     [1234][I][AlarmManager] Loaded
 *
 * Log level control:
 *   LOG_LEVEL controls what gets compiled in.
 *   0 = none
 *   1 = error
 *   2 = warn
 *   3 = info
 *   4 = debug
 *
 * Example:
 *   #define LOG_LEVEL 4
 *   #include "Logging.h"
 *
 * If LOG_LEVEL is lower than a given macro's level, that macro compiles out.
 * This is better than a runtime `if` because unused log calls disappear from
 * the build entirely.
 *
 * Optional per-file debug disable:
 *   If one file is too noisy, you can disable only that file's `LOGD(...)`
 *   output without changing the global log level.
 *
 *     #define LOG_TAG "SomeModule"
 *     #define LOG_DEBUG_DISABLE true
 *     #include "Logging.h"
 *
 *   Behavior:
 *   - If `LOG_DEBUG_DISABLE` is missing, debug logs stay enabled.
 *   - If `LOG_DEBUG_DISABLE` is `false`, debug logs stay enabled.
 *   - If `LOG_DEBUG_DISABLE` is `true`, `LOGD(...)` compiles out for that file.
 */

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

/*
 * LOG_TAG is optional. If a file does not define it before including this
 * header, logs from that file simply omit the module tag.
 */
#ifndef LOG_TAG
#define LOG_TAG nullptr
#endif

/*
 * Per-file override for debug logs.
 *
 * Default behavior is "debug logs allowed". A file can opt out by defining:
 *   #define LOG_DEBUG_DISABLE true
 * before including this header.
 */
#ifndef LOG_DEBUG_DISABLE
#define LOG_DEBUG_DISABLE false
#endif

/*
 * Internal helper used by the public LOG* macros.
 *
 * Why this exists:
 * - It keeps the formatting rules in one place.
 * - It makes it easy to later redirect logs somewhere other than Serial.
 * - It lets the macros stay short and readable.
 *
 * Format today:
 *   [millis][level][tag] message
 *
 * Example:
 *   [1523][I][Touch] Calibration loaded
 *
 * Why this does not call `Serial.vprintf(...)`:
 * - Some Arduino cores expose `Serial.printf(...)` but not `Serial.vprintf(...)`.
 * - Using `vsnprintf(...)` here is more portable and avoids depending on a
 *   method that may not exist on `HardwareSerial`.
 */
static inline void log_printf_impl(const char *level,
                                   const char *tag,
                                   const char *fmt, ...) {
  Serial.printf("[%lu][%s]", millis(), level);
  if (tag != nullptr && tag[0] != '\0') {
    Serial.printf("[%s] ", tag);
  } else {
    Serial.print(" ");
  }

  va_list args;
  va_start(args, fmt);
  va_list args_copy;
  va_copy(args_copy, args);
  int needed = vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);

  if (needed >= 0) {
    size_t buf_size = static_cast<size_t>(needed) + 1;
    char stack_buf[160];
    char *buf = stack_buf;

    if (buf_size > sizeof(stack_buf)) {
      buf = static_cast<char *>(malloc(buf_size));
    }

    if (buf != nullptr) {
      vsnprintf(buf, buf_size, fmt, args);
      Serial.print(buf);
      if (buf != stack_buf) {
        free(buf);
      }
    } else {
      Serial.print("[log formatting failed: out of memory]");
    }
  } else {
    Serial.print("[log formatting failed]");
  }
  va_end(args);

  Serial.print('\n');
}

/*
 * Public logging macros.
 *
 * These are compile-time gated. For example, if LOG_LEVEL is 2, then LOGI and
 * LOGD turn into empty statements and do not generate output code.
 */
#if LOG_LEVEL >= 1
#define LOGE(fmt, ...) do { log_printf_impl("E", LOG_TAG, fmt, ##__VA_ARGS__); } while (0)
#else
#define LOGE(...) do {} while (0)
#endif

#if LOG_LEVEL >= 2
#define LOGW(fmt, ...) do { log_printf_impl("W", LOG_TAG, fmt, ##__VA_ARGS__); } while (0)
#else
#define LOGW(...) do {} while (0)
#endif

#if LOG_LEVEL >= 3
#define LOGI(fmt, ...) do { log_printf_impl("I", LOG_TAG, fmt, ##__VA_ARGS__); } while (0)
#else
#define LOGI(...) do {} while (0)
#endif

/*
 * Debug logging uses a file-level gate at the call site instead of a `#if`
 * gate at header-parse time.
 *
 * Why this is more robust:
 * - Each `.cpp` file can say `#define LOG_DEBUG_DISABLE 1` or `true`.
 * - The decision to print happens where `LOGD(...)` is used, using that
 *   file's current setting.
 * - This avoids the "header was included earlier so the wrong version of LOGD
 *   got baked in" problem.
 */
#define LOGD(fmt, ...) \
  do { \
    if ((LOG_LEVEL >= 4) && !(LOG_DEBUG_DISABLE)) { \
      log_printf_impl("D", LOG_TAG, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

#endif

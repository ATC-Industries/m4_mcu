#ifndef LOGGING_H
#define LOGGING_H
#include <Arduino.h>

#ifndef LOG_LEVEL
#define LOG_LEVEL 3  // 0=none 1=error 2=warn 3=info 4=debug
#endif

#define LOGE(fmt, ...) do { if (LOG_LEVEL >= 1) Serial.printf("[E] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOGW(fmt, ...) do { if (LOG_LEVEL >= 2) Serial.printf("[W] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOGI(fmt, ...) do { if (LOG_LEVEL >= 3) Serial.printf("[I] " fmt "\n", ##__VA_ARGS__); } while(0)
#define LOGD(fmt, ...) do { if (LOG_LEVEL >= 4) Serial.printf("[D] " fmt "\n", ##__VA_ARGS__); } while(0)

// Example usage:
// LOGE("This is an error: %d", errorCode);
// LOGW("This is a warning");
// LOGI("This is some info");
// LOGD("This is a debug message: var=%s", varName);

#endif

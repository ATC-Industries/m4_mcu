// lvgl_mem.cpp
#include "lvgl.h"
#include "esp_heap_caps.h"

extern "C" void * lv_mem_custom_alloc(size_t size) {
  // Prefer PSRAM
  void * p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) {
    // Fallback to internal
    p = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  return p;
}

extern "C" void lv_mem_custom_free(void * p) {
  if (p) free(p);
}

#include "dev_utils/benchmark.h"

// Benchmark containers and widgets
static lv_obj_t *benchmark_cont = NULL;
static lv_obj_t *perf_label = NULL;
static lv_obj_t *mem_label = NULL;
static bool benchmark_enabled = false;

void benchmark_init() {
  // Initialize module but don't create display yet
  benchmark_enabled = false;
}

void create_benchmark_display() {
  if (benchmark_cont != NULL) return;  // Already created

  // Create a container for benchmark display
  // benchmark_cont = lv_obj_create(lv_scr_act());
  benchmark_cont = lv_obj_create(lv_layer_top());
  lv_obj_set_size(benchmark_cont, 180, 80);
  lv_obj_align(benchmark_cont, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

  // Set style for semi-transparent black background
  lv_obj_set_style_bg_color(benchmark_cont, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(benchmark_cont, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_radius(benchmark_cont, 5, LV_PART_MAIN);
  lv_obj_set_style_pad_all(benchmark_cont, 10, LV_PART_MAIN);

  // Create performance monitor label
  perf_label = lv_label_create(benchmark_cont);
  lv_obj_align(perf_label, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_label_set_text(perf_label, "FPS: calculating...");

  // Create memory usage label
  mem_label = lv_label_create(benchmark_cont);
  lv_obj_align(mem_label, LV_ALIGN_TOP_LEFT, 0, 25);

  // Get initial memory info
  lv_mem_monitor_t mem_mon;
  lv_mem_monitor(&mem_mon);
  lv_label_set_text_fmt(mem_label, "Memory: %d%% used", (int)(mem_mon.used_pct));
}

void remove_benchmark_display() {
  if (benchmark_cont != NULL) {
    lv_obj_del(benchmark_cont);
    benchmark_cont = NULL;
    perf_label = NULL;
    mem_label = NULL;
  }
}

void benchmark_set_enabled(bool enable) {
  benchmark_enabled = enable;

  if (benchmark_enabled) {
    create_benchmark_display();
  } else {
    remove_benchmark_display();
  }
}

bool benchmark_is_enabled() { return benchmark_enabled; }

void benchmark_update() {
  if (!benchmark_enabled || benchmark_cont == NULL) return;

  // Get performance data
  static uint32_t lastUpdate = 0;
  static uint32_t frameCnt = 0;
  static float fps = 0;

  frameCnt++;
  uint32_t now = millis();

  // Initialize lastUpdate if this is the first call
  if (lastUpdate == 0) {
    lastUpdate = now;
  }

  // Update every second
  if (now - lastUpdate >= 1000) {
    fps = 1000.0f * frameCnt / (now - lastUpdate);
    frameCnt = 0;
    lastUpdate = now;

    // Update FPS display
    lv_label_set_text_fmt(perf_label, "FPS: %d", (int)fps);

// Get memory info - ESP32 specific
#ifdef ESP32
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    int usedPercent = 100 - (freeHeap * 100 / totalHeap);
    lv_label_set_text_fmt(mem_label, "Memory: %d%% used", usedPercent);
#else
    // Fallback to LVGL's memory monitor
    lv_mem_monitor_t mem_mon;
    lv_mem_monitor(&mem_mon);
    lv_label_set_text_fmt(mem_label, "Memory: %d%% used", (int)(mem_mon.used_pct));
#endif
  }
}
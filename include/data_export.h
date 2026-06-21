#ifndef DATA_EXPORT_H
#define DATA_EXPORT_H

#include <lvgl/lvgl.h>

enum class DataExportState {
  IDLE,
  STARTING_AP,
  READY,
  SHUTDOWN,
};

void data_export_start();
void data_export_stop();
void data_export_loop();
void data_export_build_screen(lv_obj_t *panel);
DataExportState data_export_get_state();

#endif  // DATA_EXPORT_H

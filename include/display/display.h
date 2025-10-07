#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#include <Arduino.h>
#include "Config.h"

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;

  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      // // Pin configuration for 800x480 RGB display (OLD BOARD)
      // cfg.pin_d0 = GPIO_NUM_15;       // B3
      // cfg.pin_d1 = GPIO_NUM_7;        // B4
      // cfg.pin_d2 = GPIO_NUM_6;        // B5
      // cfg.pin_d3 = GPIO_NUM_5;        // B6
      // cfg.pin_d4 = GPIO_NUM_4;        // B7

      // cfg.pin_d5 = GPIO_NUM_9;        // G2
      // cfg.pin_d6 = GPIO_NUM_46;       // G3 
      // cfg.pin_d7 = GPIO_NUM_3;        // G4
      // cfg.pin_d8 = GPIO_NUM_8;        // G5
      // cfg.pin_d9 = GPIO_NUM_16;       // G6
      // cfg.pin_d10 = GPIO_NUM_1;       // G7

      // cfg.pin_d11 = GPIO_NUM_14;      // R3
      // cfg.pin_d12 = GPIO_NUM_21;      // R4
      // cfg.pin_d13 = GPIO_NUM_47;      // R5
      // cfg.pin_d14 = GPIO_NUM_48;      // R6
      // cfg.pin_d15 = GPIO_NUM_45;      // R7

      // cfg.pin_henable = GPIO_NUM_41;  // DE
      // cfg.pin_vsync = GPIO_NUM_40;    // VSYNC
      // cfg.pin_hsync = GPIO_NUM_39;    // HSYNC
      // cfg.pin_pclk = GPIO_NUM_0;      // BOOT LCD PCLK

      // Pin configuration for 800x480 RGB display
      cfg.pin_d0  = IO_B3;       // B3
      cfg.pin_d1  = IO_B4;       // B4
      cfg.pin_d2  = IO_B5;       // B5
      cfg.pin_d3  = IO_B6;       // B6
      cfg.pin_d4  = IO_B7;       // B7

      cfg.pin_d5  = IO_G2;       // G2
      cfg.pin_d6  = IO_G3;       // G3
      cfg.pin_d7  = IO_G4;       // G4
      cfg.pin_d8  = IO_G5;       // G5
      cfg.pin_d9  = IO_G6;       // G6
      cfg.pin_d10 = IO_G7;       // G7

      cfg.pin_d11 = IO_R3;       // R3
      cfg.pin_d12 = IO_R4;       // R4
      cfg.pin_d13 = IO_R5;       // R5
      cfg.pin_d14 = IO_R6;       // R6
      cfg.pin_d15 = IO_R7;       // R7

      cfg.pin_henable = IO_DE_BOOT; // DE
      cfg.pin_vsync   = IO_VSYNC;   // VSYNC
      cfg.pin_hsync   = IO_HSYNC;   // HSYNC
      cfg.pin_pclk    = IO_PCLK;    // PCLK


      cfg.freq_write = 15000000;

      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch = 40;

      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch = 13;

      cfg.pclk_active_neg = 1;
      cfg.de_idle_high = 0;
      cfg.pclk_idle_high = 0;

      _bus_instance.config(cfg);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width = 800;
      cfg.memory_height = 480;
      cfg.panel_width = 800;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;

      // cfg.rgb_order = 1;

      _panel_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

extern LGFX lcd;

// Initialize the display and touch
void init_display();

#endif  // INCLUDE_DISPLAY_H_
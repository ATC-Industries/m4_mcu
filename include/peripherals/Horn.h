#ifndef PERIPHERALS_HORN_H_
#define PERIPHERALS_HORN_H_

#include <Arduino.h>
#include "peripherals/mcp23017.h"

class Horn {
 public:
  Horn(Mcp23017* io, uint8_t porta_pin, bool active_high)
      : io_(io), pin_(porta_pin), active_high_(active_high) {}

  void begin() {
    io_->pinModeA(pin_, OUTPUT);
    off();
  }
  void on()  { io_->writeA(pin_,  active_high_); }
  void off() { io_->writeA(pin_, !active_high_); }
  void pulse(uint16_t ms) { on(); delay(ms); off(); }

 private:
  Mcp23017* io_;
  uint8_t pin_;
  bool active_high_;
};

#endif  // PERIPHERALS_HORN_H_

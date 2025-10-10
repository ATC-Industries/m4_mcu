// src/peripherals/Button.cpp
#include "peripherals/Button.h"

Button::Button(uint8_t pin, bool active_low, uint16_t debounce_ms, uint16_t long_ms)
    : pin_(pin),
      active_low_(active_low),
      debounce_ms_(debounce_ms),
      long_ms_(long_ms) {}

void Button::begin() {
  pinMode(pin_, active_low_ ? INPUT_PULLUP : INPUT_PULLDOWN);
  last_raw_ = readRaw();
  stable_state_pressed_ = last_raw_;
  last_change_ms_ = millis();
}

bool Button::readRaw() const {
  bool level = digitalRead(pin_);
  return active_low_ ? !level : level;
}

void Button::update() {
  const unsigned long now = millis();
  bool raw = readRaw();

  if (raw != last_raw_) {
    last_raw_ = raw;
    last_change_ms_ = now;
    // do not change stable state yet
  }

  // Debounce window
  if ((now - last_change_ms_) >= debounce_ms_) {
    if (raw != stable_state_pressed_) {
      stable_state_pressed_ = raw;
      long_fired_ = false;
      if (stable_state_pressed_) {
        if (on_press_) on_press_();
      } else {
        if (on_release_) on_release_();
      }
    }
  }

  // Long press
  if (stable_state_pressed_ && !long_fired_) {
    if ((now - last_change_ms_) >= long_ms_) {
      long_fired_ = true;
      if (on_long_) on_long_();
    }
  }
}

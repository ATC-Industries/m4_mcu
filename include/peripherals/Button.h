// include/peripherals/Button.h
#ifndef PERIPHERALS_BUTTON_H_
#define PERIPHERALS_BUTTON_H_

#include <Arduino.h>

typedef void (*BtnCallback)();

class Button {
 public:
  Button(uint8_t pin, bool active_low, uint16_t debounce_ms, uint16_t long_ms);

  void begin();
  // Call in loop
  void update();

  // Optional callbacks
  void onPress(BtnCallback cb)   { on_press_ = cb; }
  void onRelease(BtnCallback cb) { on_release_ = cb; }
  void onLong(BtnCallback cb)    { on_long_ = cb; }

  // Query
  bool isPressed() const { return stable_state_pressed_; }

 private:
  bool readRaw() const;

  uint8_t pin_;
  bool active_low_;
  uint16_t debounce_ms_;
  uint16_t long_ms_;

  bool last_raw_{false};
  bool stable_state_pressed_{false};
  unsigned long last_change_ms_{0};
  bool long_fired_{false};

  BtnCallback on_press_{nullptr};
  BtnCallback on_release_{nullptr};
  BtnCallback on_long_{nullptr};
};

#endif  // PERIPHERALS_BUTTON_H_

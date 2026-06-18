#include "display/backlight.h"

// Constants
static uint8_t current_brightness = 200;  // Default brightness (0-255)
static uint8_t target_brightness = 200;   // Target brightness for smooth transitions
static bool transition_active = false;    // Flag to indicate transition in progress
static unsigned long last_update_time = 0;
static const unsigned long UPDATE_INTERVAL = 5;  // 5ms between updates (faster)
static const uint8_t STEP_SIZE = 5;              // Larger steps for faster transitions

// Setup function - call this in your main setup()
void setupBacklight(uint8_t pin, uint8_t channel, uint8_t initial_brightness) {
  // Configure PWM with higher frequency for smoother operation
  ledcSetup(channel, LCD_BL_FREQ, 8);  // 10kHz frequency, 8-bit resolution
  ledcAttachPin(pin, channel);

  // Set initial brightness
  current_brightness = initial_brightness;
  target_brightness = initial_brightness;
  ledcWrite(channel, initial_brightness);
}

// Set backlight immediately
void setBacklight(uint8_t brightness) {
  current_brightness = brightness;  // Store the current brightness
  target_brightness = brightness;   // Also set as target
  ledcWrite(LCD_BL_CHANNEL, brightness);
  transition_active = false;
}

// Start smooth transition to new brightness level
void setBacklightSmooth(uint8_t brightness) {
  target_brightness = brightness;
  transition_active = true;
}

// Call this function regularly in your main loop
void updateBacklight() {
  if (!transition_active) return;

  unsigned long current_time = millis();
  if (current_time - last_update_time < UPDATE_INTERVAL) return;
  last_update_time = current_time;

  // Use larger step size for faster transitions
  if (current_brightness < target_brightness) {
    // Calculate step size based on difference (for very responsive UI)
    uint8_t step = min((uint8_t)STEP_SIZE, (uint8_t)(target_brightness - current_brightness));
    current_brightness += step;
    ledcWrite(LCD_BL_CHANNEL, current_brightness);
  } else if (current_brightness > target_brightness) {
    uint8_t step = min((uint8_t)STEP_SIZE, (uint8_t)(current_brightness - target_brightness));
    current_brightness -= step;
    ledcWrite(LCD_BL_CHANNEL, current_brightness);
  } else {
    transition_active = false;  // Reached target, stop transition
  }
}

// Fast mode for UI interactions like sliders
void setBacklightFast(uint8_t brightness) {
  // Slider drags should update immediately. Falling back to the smooth-ramp
  // path here creates a stream of intermediate PWM duty changes while the user
  // is still moving the slider, which can make panel jitter worse.
  current_brightness = brightness;
  target_brightness = brightness;
  transition_active = false;
  ledcWrite(LCD_BL_CHANNEL, brightness);
}

// Fade backlight without blocking (just initiates the fade)
void fadeBacklight(bool fadeIn, int duration) {
  target_brightness = fadeIn ? 255 : 0;
  transition_active = true;
}

uint8_t getBacklight() { return current_brightness; }

// PullStateManager.h
#ifndef PULLSTATEMANAGER_H
#define PULLSTATEMANAGER_H

#include "StateManager.h"
#include <lvgl/lvgl.h>

class PullStateManager {
public:
  static void init();    // optional setup if needed
  static void update();  // Call this every loop iteration

  // Button handlers (called from UI)
  static void handleStagePressed(lv_event_t *e);
  static void handleCancelPressed();
  static void handleStopPressed();
  static void handleDiscardPressed();
  static void handleSavePressed();
  static void handleResetPressed();

  // External triggers
  static void triggerEmergencyStop();
  static void detectPullStart(float currentSpeed);  // Can be called periodically to detect transition to PULLING

private:
  static void enterState(PullState newState);
  static void updateUIForState(PullState state);
  static void triggerRelaysForState(PullState state);
  static void resetMaxValues();
  static void resetCurrentValues();
  static const char* stateToString(PullState state);
};

#endif

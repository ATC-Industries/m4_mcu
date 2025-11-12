#include "PullStateManager.h"

#include "../ui/ui.h"
#include "SpeedModule.h"
#include "StateManager.h"
#include "AlarmManager.h"
#include "TachClient.h"
#include "Logging.h"

static unsigned long lastDebugPrint = 0;
static const unsigned long debugInterval = 5000;  // Every 5 seconds
static PullState s_lastState = PullState::READY;
static PullState s_lastNotifiedState = PullState::READY;

namespace {
  constexpr float  kStartSpeedMph = 0.5f;   // already using this
  constexpr float  kEndSpeedMph   = 0.2f;   // lower than start to avoid chatter
  constexpr uint32_t kEndHoldMs   = 300;    // must be low for 300 ms
  uint32_t s_belowSinceMs = 0;
}

void PullStateManager::enterState(PullState newState) {
  const char* oldState = PullStateManager::stateToString(StateManager::getPullState());
  const char* newStateStr = PullStateManager::stateToString(newState);
  LOGI("[PSM] Transitioning from %s to %s", oldState, newStateStr);

  StateManager::setPullState(newState);
  updateUIForState(newState);
  triggerRelaysForState(newState);
  SpeedModule::notifyPullStateChanged(newState);

  if (newState == PullState::READY) resetMaxValues();
}

void PullStateManager::init() {
  LOGI("[PSM] init -> READY");
  enterState(PullState::READY);  // Always start in READY
}
// TODO: Somewhere here I need to Call AlarmManager::evaluateTick() while PullState is STAGED or PULLING.


void PullStateManager::update() {
  PullState current = StateManager::getPullState();

  if (current == PullState::READY) {
    s_belowSinceMs = 0;
    AlarmManager::resetForStateEntry();
  } else if (current == PullState::STAGED) {
    float s = StateManager::getSpeed();
    detectPullStart(s); // starts at > 0.5 mph
  } else if (current == PullState::PULLING) {
    float s = StateManager::getSpeed();

    // End-of-pull hysteresis with hold time
    uint32_t now = millis();
    if (s <= kEndSpeedMph) {
      if (s_belowSinceMs == 0) s_belowSinceMs = now;
      if (now - s_belowSinceMs >= kEndHoldMs) {
        LOGI("[PSM] PULLING -> speed low for %ums -> PULLEND", (unsigned)kEndHoldMs);
        s_belowSinceMs = 0;
        enterState(PullState::PULLEND);
      }
    } else {
      // back above threshold, clear timer
      s_belowSinceMs = 0;
    }
  }

  unsigned long now = millis();
  if (now - lastDebugPrint >= debugInterval) {
    lastDebugPrint = now;

    PullState current = StateManager::getPullState();
    const char* stateStr = PullStateManager::stateToString(current);
    //LOGD("[PSM] Heartbeat: current state=%s", stateStr);

    //LOGI("[PSM] heartbeat state=%d", (int)current);
    updateUIForState(current);
  }
}

void PullStateManager::handleStagePressed(lv_event_t *e) {
  LOGI("[PSM] handleStagePressed");
  // notify TachClient once on transition
  LOGI("[PSM] notifying TachClient due to state change -> STAGED");
  Tach::pairTSS(e);
  enterState(PullState::STAGED);   // this will now notify TachClient once
}

void PullStateManager::handleCancelPressed() { enterState(PullState::READY); }

void PullStateManager::handleStopPressed() { enterState(PullState::PULLEND); }

void PullStateManager::handleDiscardPressed() {
  resetMaxValues();
  enterState(PullState::READY);
}

void PullStateManager::handleSavePressed() {
  StateManager::savePullResult();
  
  // Increment to next driver
  StateManager::prefs().driverNumber += 1;
  StateManager::savePreferences();
  
  enterState(PullState::READY);
}

void PullStateManager::handleResetPressed() { enterState(PullState::READY); }

void PullStateManager::triggerEmergencyStop() { enterState(PullState::EMERGENCYSTOP); }

void PullStateManager::detectPullStart(float currentSpeed) {
  static unsigned long lastStageCheckMs = 0;
  unsigned long now = millis();
  if (now - lastStageCheckMs >= debugInterval) {
    lastStageCheckMs = now;
    LOGD("[PSM] detectPullStart speed=%.2f state=%d", currentSpeed, (int)StateManager::getPullState());
  }
  if (StateManager::getPullState() == PullState::STAGED && currentSpeed > 0.5f) {
    LOGI("[PSM] STAGED + speed>0.5 -> PULLING");
    enterState(PullState::PULLING);
  }
}

void PullStateManager::resetMaxValues() { 
  StateManager::resetAllMaxValues(); 
  AlarmManager::resetForStateEntry(); // Reset all alarms
}

void PullStateManager::resetCurrentValues() {
  StateManager::setRPM(0);
  StateManager::setSpeed(0);
  StateManager::setDistance(0);
}

void PullStateManager::updateUIForState(PullState state) {
  lv_obj_add_flag(ui_MainContainerStateREADY, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_MainContainerStateSTAGED, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_MainContainerStatePULLING, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_MainContainerStatePULLEND, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui_MainContainerStateEMERGENCYSTOP, LV_OBJ_FLAG_HIDDEN);

  switch (state) {
    case PullState::READY:
      lv_obj_clear_flag(ui_MainContainerStateREADY, LV_OBJ_FLAG_HIDDEN);
      break;
    case PullState::STAGED:
      lv_obj_clear_flag(ui_MainContainerStateSTAGED, LV_OBJ_FLAG_HIDDEN);
      break;
    case PullState::PULLING:
      lv_obj_clear_flag(ui_MainContainerStatePULLING, LV_OBJ_FLAG_HIDDEN);
      break;
    case PullState::PULLEND:
      lv_obj_clear_flag(ui_MainContainerStatePULLEND, LV_OBJ_FLAG_HIDDEN);
      break;
    case PullState::EMERGENCYSTOP:
      lv_obj_clear_flag(ui_MainContainerStateEMERGENCYSTOP, LV_OBJ_FLAG_HIDDEN);
      break;
  }
}

void PullStateManager::triggerRelaysForState(PullState state) {
  // You can customize which relays are activated depending on the state
  // Example:
  for (int i = 0; i < 4; ++i) {
    if (StateManager::prefs().relayEnabled[i]) {
      StateManager::setRelayState(i, (state == PullState::PULLING ? RelayState::ENGAGED : RelayState::DISENGAGED));
    }
  }
}

const char* PullStateManager::stateToString(PullState state) {
  switch (state) {
    case PullState::READY:
      return "READY";
    case PullState::STAGED:
      return "STAGED";
    case PullState::PULLING:
      return "PULLING";
    case PullState::PULLEND:
      return "PULLEND";
    case PullState::EMERGENCYSTOP:
      return "EMERGENCYSTOP";
    default:
      return "UNKNOWN";
  }
}

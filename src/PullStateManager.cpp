#include "PullStateManager.h"

#include "../ui/ui.h"
#include "SpeedModule.h"
#include "StateManager.h"
#include "AlarmManager.h"

static unsigned long lastDebugPrint = 0;
static const unsigned long debugInterval = 2000;  // Every 2 seconds

void PullStateManager::enterState(PullState newState) {
  StateManager::setPullState(newState);
  updateUIForState(newState);
  triggerRelaysForState(newState);
  SpeedModule::notifyPullStateChanged(newState);

  if (newState == PullState::READY) {
    resetMaxValues();
  }

  if (newState == PullState::PULLEND) {
    // optionally set labels to MAX
  }
}

void PullStateManager::init() {
  enterState(PullState::READY);  // Always start in READY
}
// TODO: Somewhere here I need to Call AlarmManager::evaluateTick() while PullState is STAGED or PULLING.


void PullStateManager::update() {
  PullState current = StateManager::getPullState();

  switch (current) {
    case PullState::STAGED:
      detectPullStart(StateManager::getSpeed());
      break;

    case PullState::PULLING:
      if (StateManager::getSpeed() <= 0.0f) {
        enterState(PullState::PULLEND);
      }
      break;

      // Future: Handle e-stop debounce / other state checks here

    default:
      break;
  }

  unsigned long now = millis();
  if (now - lastDebugPrint >= debugInterval) {
    lastDebugPrint = now;

    PullState current = StateManager::getPullState();
    const char* stateStr = "";

    switch (current) {
      case PullState::READY:
        stateStr = "READY";
        break;
      case PullState::STAGED:
        stateStr = "STAGED";
        break;
      case PullState::PULLING:
        stateStr = "PULLING";
        break;
      case PullState::PULLEND:
        stateStr = "PULLEND";
        break;
      case PullState::EMERGENCYSTOP:
        stateStr = "EMERGENCYSTOP";
        break;
      default:
        stateStr = "UNKNOWN";
        break;
    }

    //Serial.printf("[PullState] Current state: %s\n", stateStr);
    updateUIForState(current);
  }
}

void PullStateManager::handleStagePressed() { 
  enterState(PullState::STAGED); 
  // // TODO: all this is copid from the D35. I need figur it all out. commenting for now.
  // resetBtnCB(e); //TODO change to something that will reset the tach connection indicator
  // Serial.println("Starting pairing...");  // Log start of pairing process

  // // Reset the RSSI (Received Signal Strength Indicator) value to the lowest
  // // possible to ensure the next value received is higher.
  // pairedTractorRSSI = INT_MIN;

  // // Clear the previously paired tractor's address to start fresh.
  // memset(pairedTractorAddress, 0, sizeof(pairedTractorAddress));

  // // Flag to indicate that the pairing process has started.
  // isPairing = true;
  // pairingStartedMillis = millis();
  // isWaitingForPairingDelay = true;

  // // Create a JSON document for configuring the pairing message.
  // // This document is used to specify the action and the type of device we want
  // // to pair with.
  // JsonDocument doc;
  // doc["action"] =
  //     SEND_PROXIMITY;  // Action to request proximity information from devices.
  // doc["reqDevice"] = M4_TACH_SENSOR;  // Specify that we're interested in
  //                                     // pairing with a tach sensor.

  // // Serialize the JSON document to a string payload to be sent over the
  // // network.
  // String payload;

  // doc.shrinkToFit();  // optional

  // serializeJson(doc, payload);  // Convert the JSON document into a string.

  // // Define a broadcast address to send the pairing message to all devices.
  // uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  // // Send the pairing message as a broadcast with high priority.
  // sendMessage(broadcastAddress, BROADCAST, SEND_PROXIMITY, payload,
  //             HIGH_PRIORITY);

  // // Log that the pairing button was pressed and the message has been sent.
  // Serial.println("Pair Tach button pressed");
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
  if (StateManager::getPullState() == PullState::STAGED && currentSpeed > 0.5f) {
    enterState(PullState::PULLING);
  }
}

void PullStateManager::resetMaxValues() { StateManager::resetAllMaxValues(); }

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

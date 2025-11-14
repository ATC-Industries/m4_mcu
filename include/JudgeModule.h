// include/JudgeModule.h
#ifndef INCLUDE_JUDGE_JUDGEMODULE_H_
#define INCLUDE_JUDGE_JUDGEMODULE_H_

#include <Arduino.h>

#include "PullStateManager.h"  // PullState enum
#include "StateManager.h"      // PullResult, UnitSystem, etc

// Forward declare LVGL type so we do not leak LVGL everywhere
struct _lv_obj_t;
typedef _lv_obj_t lv_obj_t;

namespace JudgeModule {

// Snapshot of a single broadcast from the sled host
struct HostSnapshot {
  uint8_t   host_id;
  PullState pull_state;

  float     distanceFeet;       // raw host feet
  float     speedMph;           // raw host mph
  float     rpm;

  float     maxDistanceFeet;
  float     maxSpeedMph;
  float     maxRpm;
};

// Lifecycle
void begin();
void tick();

// Judge mode control
void onJudgeModeChanged(bool enabled);
bool isJudgeModeActive();

// Configure which host we are listening to
void setTrackedHostId(uint8_t host_id);
uint8_t getTrackedHostId();

// Called by comms when the sled host broadcasts a status packet
void onHostStatusBroadcast(const HostSnapshot& snap);

// Pull history sync - from host to judge
void applyRemotePullHistory(const PullResult* pulls, int count);

// UI helpers - call from screen onload handlers
void applyJudgeModeToMainScreen();
void applyJudgeModeToSettingsScreen();
void applyJudgeModeToPullHistoryScreen();

// Debug access
const HostSnapshot& getLastSnapshot();

}  // namespace JudgeModule

#endif  // INCLUDE_JUDGE_JUDGEMODULE_H_

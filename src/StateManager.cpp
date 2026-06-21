#include "StateManager.h"
#include "peripherals/PeripheralsInit.h"
#include <cstring>
#include <SpeedModule.h>

#define LOG_TAG "StateManager"
#include "Logging.h"

#include <Preferences.h>
#include <nvs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cstdlib>

static ::Preferences storage;
static bool s_preferencesDirty = false;
static uint8_t s_dirtySections = 0;
static unsigned long s_saveRequestedAtMs = 0;
static constexpr unsigned long kPreferencesSaveDebounceMs = 400;
static constexpr unsigned long kSaveTriggeredPersistenceDelayMs = 10000;
static constexpr unsigned long kScreenTransitionQuietMs = 500;
// TODO: For production-quality no-jitter persistence, move pull history and
// driver number to I2C FRAM. Keep NVS for low-frequency settings only.
static SemaphoreHandle_t s_prefsMutex = nullptr;
static SystemPreferences s_writeSnapshot;
static uint8_t s_snapshotSections = 0;
static int s_dirtyHistoryIndex = -1;
static int s_snapshotHistoryIndex = -1;
static unsigned long s_dispatchNotBeforeMs = 0;
static unsigned long s_lastScreenTransitionAtMs = 0;
static uint8_t s_deferredSavePersistenceDepth = 0;
static volatile bool s_writerBusy = false;
static TaskHandle_t s_writerTask = nullptr;
static bool s_hasJudgeMirrorUnits = false;
static UnitSystem s_judgeMirrorUnitSystem = UnitSystem::IMPERIAL;
static float s_judgeMirrorTrackLengthFeet = 300.0f;
static bool s_hasJudgeMirrorDisplayPrefs = false;
static bool s_judgeMirrorTachEnabled = true;
static bool s_judgeMirrorLimitSwitchesEnabled = true;
static bool s_judgeMirrorRelaysEnabled = true;
static bool s_judgeMirrorLimitSwitchEnabled[2] = {true, true};
static bool s_judgeMirrorRelayEnabled[4] = {true, true, true, true};

enum DirtySection : uint8_t {
  kDirtyGeneral = 1 << 0,
  kDirtyPairing = 1 << 1,
  kDirtyHistory = 1 << 2,
  kDirtyDriverNumber = 1 << 3,
};

struct PackedPullHistoryRowHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t reserved;
  uint32_t driverNameLen;
  int32_t driverNumber;
  uint32_t classNameLen;
  int32_t classWeight;
  float maxSpeedMPH;
  float maxDistanceFeet;
  float maxRPM;
  uint32_t timestamp;
};

static constexpr uint32_t kPackedPullHistoryRowMagic = 0x31524C50;  // "PLR1"
static constexpr uint16_t kPackedPullHistoryRowVersion = 1;

static void preferencesWriterTask(void *parameter);

static String makePullHistoryBlobKey(int index) {
  return "pull" + String(index);
}

static String decodePackedPullHistoryString(const uint8_t* data, size_t len) {
  if (len == 0) return String();

  char* buffer = static_cast<char*>(malloc(len + 1));
  if (buffer == nullptr) {
    return String();
  }
  memcpy(buffer, data, len);
  buffer[len] = '\0';
  String value(buffer);
  free(buffer);
  return value;
}

static bool loadPackedPullHistoryRow(const char* key, PullResult& result) {
  size_t blobLen = storage.getBytesLength(key);
  if (blobLen < sizeof(PackedPullHistoryRowHeader)) {
    return false;
  }

  uint8_t* blob = static_cast<uint8_t*>(malloc(blobLen));
  if (blob == nullptr) {
    LOGE("Failed to allocate pull history blob buffer");
    return false;
  }

  size_t bytesRead = storage.getBytes(key, blob, blobLen);
  if (bytesRead != blobLen) {
    free(blob);
    return false;
  }

  PackedPullHistoryRowHeader header;
  memcpy(&header, blob, sizeof(header));
  size_t expectedLen = sizeof(header) + header.driverNameLen + header.classNameLen;
  if (header.magic != kPackedPullHistoryRowMagic || header.version != kPackedPullHistoryRowVersion ||
      expectedLen != blobLen) {
    free(blob);
    return false;
  }

  size_t offset = sizeof(header);
  result.driverName = decodePackedPullHistoryString(blob + offset, header.driverNameLen);
  offset += header.driverNameLen;
  result.className = decodePackedPullHistoryString(blob + offset, header.classNameLen);
  result.driverNumber = header.driverNumber;
  result.classWeight = header.classWeight;
  result.maxSpeedMPH = header.maxSpeedMPH;
  result.maxDistanceFeet = header.maxDistanceFeet;
  result.maxRPM = header.maxRPM;
  result.timestamp = header.timestamp;

  free(blob);
  return true;
}

static void ensurePreferencesWriterStarted() {
  if (s_prefsMutex == nullptr) {
    s_prefsMutex = xSemaphoreCreateMutex();
    if (s_prefsMutex == nullptr) {
      LOGE("Failed to create preferences mutex");
      return;
    }
  }

  if (s_writerTask == nullptr) {
    constexpr BaseType_t kWriterPriority = 1;
    constexpr uint32_t kWriterStackWords = 6144 / sizeof(StackType_t);
    constexpr BaseType_t kWriterCore = (ARDUINO_RUNNING_CORE == 0) ? 1 : 0;
    BaseType_t result = xTaskCreatePinnedToCore(preferencesWriterTask, "PrefsWriter", kWriterStackWords,
                                                nullptr, kWriterPriority, &s_writerTask, kWriterCore);
    if (result != pdPASS) {
      s_writerTask = nullptr;
      LOGE("Failed to create preferences writer task");
    }
  }
}

static void requestPreferencesSave(uint8_t sections) {
  ensurePreferencesWriterStarted();
  if (s_prefsMutex != nullptr) {
    xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
    bool hadHistoryDirty = (s_dirtySections & kDirtyHistory) != 0;
    s_preferencesDirty = true;
    s_dirtySections |= sections;
    if ((sections & kDirtyHistory) && !hadHistoryDirty) {
      s_dirtyHistoryIndex = -1;
    }
    s_saveRequestedAtMs = millis();
    if (s_deferredSavePersistenceDepth > 0) {
      unsigned long delayedUntil = s_saveRequestedAtMs + kSaveTriggeredPersistenceDelayMs;
      if (delayedUntil > s_dispatchNotBeforeMs) {
        s_dispatchNotBeforeMs = delayedUntil;
      }
    }
    xSemaphoreGive(s_prefsMutex);
    return;
  }

  s_preferencesDirty = true;
  bool hadHistoryDirty = (s_dirtySections & kDirtyHistory) != 0;
  s_dirtySections |= sections;
  if ((sections & kDirtyHistory) && !hadHistoryDirty) {
    s_dirtyHistoryIndex = -1;
  }
  s_saveRequestedAtMs = millis();
  if (s_deferredSavePersistenceDepth > 0) {
    unsigned long delayedUntil = s_saveRequestedAtMs + kSaveTriggeredPersistenceDelayMs;
    if (delayedUntil > s_dispatchNotBeforeMs) {
      s_dispatchNotBeforeMs = delayedUntil;
    }
  }
}

static void requestPullHistorySave(int dirtyIndex) {
  ensurePreferencesWriterStarted();
  if (s_prefsMutex != nullptr) {
    xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
    s_preferencesDirty = true;
    if (s_dirtySections & kDirtyHistory) {
      if (s_dirtyHistoryIndex != dirtyIndex) {
        s_dirtyHistoryIndex = -1;
      }
    } else {
      s_dirtyHistoryIndex = dirtyIndex;
    }
    s_dirtySections |= kDirtyHistory;
    s_saveRequestedAtMs = millis();
    if (s_deferredSavePersistenceDepth > 0) {
      unsigned long delayedUntil = s_saveRequestedAtMs + kSaveTriggeredPersistenceDelayMs;
      if (delayedUntil > s_dispatchNotBeforeMs) {
        s_dispatchNotBeforeMs = delayedUntil;
      }
    }
    xSemaphoreGive(s_prefsMutex);
    return;
  }

  s_preferencesDirty = true;
  if (s_dirtySections & kDirtyHistory) {
    if (s_dirtyHistoryIndex != dirtyIndex) {
      s_dirtyHistoryIndex = -1;
    }
  } else {
    s_dirtyHistoryIndex = dirtyIndex;
  }
  s_dirtySections |= kDirtyHistory;
  s_saveRequestedAtMs = millis();
  if (s_deferredSavePersistenceDepth > 0) {
    unsigned long delayedUntil = s_saveRequestedAtMs + kSaveTriggeredPersistenceDelayMs;
    if (delayedUntil > s_dispatchNotBeforeMs) {
      s_dispatchNotBeforeMs = delayedUntil;
    }
  }
}

static esp_err_t writeGeneralPreferencesToStorage(nvs_handle_t h, const SystemPreferences& prefs) {
  esp_err_t err = nvs_set_u8(h, "unitSystem", static_cast<uint8_t>(prefs.unitSystem));
  if (err != ESP_OK) return err;
  err = nvs_set_str(h, "className", prefs.pullingClassName.c_str());
  if (err != ESP_OK) return err;
  err = nvs_set_i32(h, "classWeight", prefs.pullingClassWeight);
  if (err != ESP_OK) return err;
  err = nvs_set_str(h, "driverName", prefs.driverName.c_str());
  if (err != ESP_OK) return err;
  err = nvs_set_i32(h, "driverNumber", prefs.driverNumber);
  if (err != ESP_OK) return err;
  err = nvs_set_i32(h, "M4ID", prefs.M4IDNumber);
  if (err != ESP_OK) return err;
  err = nvs_set_i32(h, "HostM4ID", prefs.HostM4IDNumber);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "isJudgeMode", prefs.isJudgeMode ? 1 : 0);
  if (err != ESP_OK) return err;

  err = nvs_set_u8(h, "ls1_enabled", prefs.limitSwitchEnabled[0] ? 1 : 0);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "ls2_enabled", prefs.limitSwitchEnabled[1] ? 1 : 0);
  if (err != ESP_OK) return err;

  for (int i = 0; i < 4; ++i) {
    err = nvs_set_u8(h, ("relayEn" + String(i)).c_str(), prefs.relayEnabled[i] ? 1 : 0);
    if (err != ESP_OK) return err;
  }

  err = nvs_set_blob(h, "distAlarm1", &prefs.distanceAlarm1, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "distAlarm2", &prefs.distanceAlarm2, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "tachAlarm1", &prefs.tachAlarm1, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "tachAlarm2", &prefs.tachAlarm2, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "mphAlarm1", &prefs.mphAlarm1, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "mphAlarm2", &prefs.mphAlarm2, sizeof(float));
  if (err != ESP_OK) return err;

  err = nvs_set_blob(h, "trackLength", &prefs.trackLengthFeet, sizeof(float));
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "brightness", prefs.screenBrightness);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "screenRot180", prefs.screenRotation180 ? 1 : 0);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "tachEnabled", prefs.tachEnabled ? 1 : 0);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "limitsEnabled", prefs.limitSwitchesEnabled ? 1 : 0);
  if (err != ESP_OK) return err;
  err = nvs_set_u8(h, "relaysEnabled", prefs.relaysEnabled ? 1 : 0);
  if (err != ESP_OK) return err;
  return nvs_set_i32(h, "speedCal", prefs.speedCalibrationPulses);
}

static esp_err_t writePairingPreferencesToStorage(nvs_handle_t h, const SystemPreferences& prefs) {
  esp_err_t err = nvs_set_u8(h, "autoConnTrac", prefs.isAutoConnectTractor ? 1 : 0);
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "tractorAddr", prefs.pairedTractorAddress, 6);
  if (err != ESP_OK) return err;
  err = nvs_set_blob(h, "remoteAddr", prefs.pairedRemoteDisplayAddress, 6);
  if (err != ESP_OK) return err;
  return nvs_set_u32(h, "pairingDelay", prefs.pairingDelay);
}

static esp_err_t writeDriverNumberToStorage(nvs_handle_t h, const SystemPreferences& prefs) {
  return nvs_set_i32(h, "driverNumber", prefs.driverNumber);
}

static esp_err_t writePullHistoryRowToStorage(nvs_handle_t h, const SystemPreferences& prefs, int index) {
  const PullResult& pull = prefs.pullHistory[index];
  PackedPullHistoryRowHeader header = {
      .magic = kPackedPullHistoryRowMagic,
      .version = kPackedPullHistoryRowVersion,
      .reserved = 0,
      .driverNameLen = static_cast<uint32_t>(pull.driverName.length()),
      .driverNumber = pull.driverNumber,
      .classNameLen = static_cast<uint32_t>(pull.className.length()),
      .classWeight = pull.classWeight,
      .maxSpeedMPH = pull.maxSpeedMPH,
      .maxDistanceFeet = pull.maxDistanceFeet,
      .maxRPM = pull.maxRPM,
      .timestamp = static_cast<uint32_t>(pull.timestamp),
  };

  const size_t blobLen = sizeof(header) + header.driverNameLen + header.classNameLen;
  uint8_t* blob = static_cast<uint8_t*>(malloc(blobLen));
  if (blob == nullptr) {
    return ESP_ERR_NO_MEM;
  }

  memcpy(blob, &header, sizeof(header));
  size_t offset = sizeof(header);
  if (header.driverNameLen > 0) {
    memcpy(blob + offset, pull.driverName.c_str(), header.driverNameLen);
    offset += header.driverNameLen;
  }
  if (header.classNameLen > 0) {
    memcpy(blob + offset, pull.className.c_str(), header.classNameLen);
  }

  String key = makePullHistoryBlobKey(index);
  esp_err_t err = nvs_set_blob(h, key.c_str(), blob, blobLen);
  free(blob);
  return err;
}

static esp_err_t writePullHistoryToStorage(nvs_handle_t h, const SystemPreferences& prefs, int dirtyIndex) {
  bool singleRow = dirtyIndex >= 0 && dirtyIndex < prefs.pullHistoryCount;

  esp_err_t err;
  if (singleRow) {
    err = nvs_set_i32(h, "pullCount", prefs.pullHistoryCount);
    if (err != ESP_OK) return err;

    err = writePullHistoryRowToStorage(h, prefs, dirtyIndex);
    if (err != ESP_OK) return err;
    return ESP_OK;
  }

  err = nvs_set_i32(h, "pullCount", prefs.pullHistoryCount);
  if (err != ESP_OK) return err;

  for (int i = 0; i < prefs.pullHistoryCount; i++) {
    err = writePullHistoryRowToStorage(h, prefs, i);
    if (err != ESP_OK) return err;
  }
  return ESP_OK;
}

static void mergeFailedSnapshot(uint8_t snapshotSections, int snapshotHistoryIndex) {
  bool hadPendingHistory = (s_dirtySections & kDirtyHistory) != 0;
  int pendingHistoryIndex = s_dirtyHistoryIndex;

  s_preferencesDirty = true;
  s_dirtySections |= snapshotSections;
  if (snapshotSections & kDirtyHistory) {
    if (!hadPendingHistory) {
      s_dirtyHistoryIndex = snapshotHistoryIndex;
    } else if (pendingHistoryIndex != snapshotHistoryIndex) {
      s_dirtyHistoryIndex = -1;
    }
  }
  s_saveRequestedAtMs = millis();
}

static void preferencesWriterTask(void *parameter) {
  (void)parameter;

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    const uint8_t snapshotSections = s_snapshotSections;
    esp_err_t err = ESP_OK;
    nvs_handle_t h = 0;
    err = nvs_open("m4prefs", NVS_READWRITE, &h);
    if (err == ESP_OK && (snapshotSections & kDirtyGeneral)) {
      err = writeGeneralPreferencesToStorage(h, s_writeSnapshot);
    }
    if (err == ESP_OK && (snapshotSections & kDirtyDriverNumber)) {
      err = writeDriverNumberToStorage(h, s_writeSnapshot);
    }
    if (err == ESP_OK && (snapshotSections & kDirtyPairing)) {
      err = writePairingPreferencesToStorage(h, s_writeSnapshot);
    }
    if (err == ESP_OK && (snapshotSections & kDirtyHistory)) {
      err = writePullHistoryToStorage(h, s_writeSnapshot, s_snapshotHistoryIndex);
    }
    if (err == ESP_OK) {
      err = nvs_commit(h);
    }
    if (h != 0) {
      nvs_close(h);
    }

    xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
    if (err != ESP_OK) {
      mergeFailedSnapshot(snapshotSections, s_snapshotHistoryIndex);
    }
    s_writerBusy = false;
    xSemaphoreGive(s_prefsMutex);

    if (err != ESP_OK) {
      LOGE("nvs flush failed: %d", static_cast<int>(err));
      LOGE("If you recently burned new firmware. Advise doing an Erase Flash");
    }
  }
}

static void dispatchPendingSaveIfIdle() {
  ensurePreferencesWriterStarted();
  if (s_prefsMutex == nullptr || s_writerTask == nullptr || s_writerBusy) {
    return;
  }

  bool shouldNotify = false;
  xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
  if (!s_writerBusy && s_preferencesDirty && s_dirtySections != 0) {
    s_writeSnapshot = StateManager::prefs();
    s_snapshotSections = s_dirtySections;
    s_snapshotHistoryIndex = (s_dirtySections & kDirtyHistory) ? s_dirtyHistoryIndex : -1;
    s_dirtySections = 0;
    s_dirtyHistoryIndex = -1;
    s_dispatchNotBeforeMs = 0;
    s_preferencesDirty = false;
    s_writerBusy = true;
    shouldNotify = true;
  }
  xSemaphoreGive(s_prefsMutex);

  if (shouldNotify) {
    xTaskNotifyGive(s_writerTask);
  }
}

SystemState StateManager::systemState;
SystemPreferences StateManager::preferences;

SystemState& StateManager::state() { return systemState; }
SystemPreferences& StateManager::prefs() { return preferences; }

// ----- Unit-aware getters -----

namespace {
UnitSystem getEffectiveUnitSystem() {
  if (StateManager::getJudgeMode() && s_hasJudgeMirrorUnits) {
    return s_judgeMirrorUnitSystem;
  }
  return StateManager::prefs().unitSystem;
}

float getEffectiveTrackLengthFeet() {
  if (StateManager::getJudgeMode() && s_hasJudgeMirrorUnits) {
    return s_judgeMirrorTrackLengthFeet;
  }
  return StateManager::prefs().trackLengthFeet;
}

bool shouldUseJudgeMirrorDisplayPrefs() {
  return StateManager::getJudgeMode() && s_hasJudgeMirrorDisplayPrefs;
}
}  // namespace

UnitSystem StateManager::getUnitSystem() { return getEffectiveUnitSystem(); }

float StateManager::getDistance() {
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.distanceInFeet
                                                            : systemState.distanceInFeet * 0.3048f;
}

float StateManager::getSpeed() {
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.speedInMPH
                                                            : systemState.speedInMPH * 1.60934f;
}

float StateManager::getTrackLength() {
  float trackLengthFeet = getEffectiveTrackLengthFeet();
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? trackLengthFeet
                                                            : trackLengthFeet * 0.3048f;
}

float StateManager::getRPM() { return systemState.rpm; }

float StateManager::getMaxRPM() { return systemState.maxRpm; }

float StateManager::getMaxSpeed() { 
  return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.maxSpeedInMPH
                                                            : systemState.maxSpeedInMPH * 1.60934f;
}

float StateManager::getMaxDistance() { 
      return (getEffectiveUnitSystem() == UnitSystem::IMPERIAL) ? systemState.maxDistanceInFeet
                                                                : systemState.maxDistanceInFeet * 0.3048f;
}

bool StateManager::getTachEnabled() {
  if (shouldUseJudgeMirrorDisplayPrefs()) {
    return s_judgeMirrorTachEnabled;
  }
  return preferences.tachEnabled;
}

bool StateManager::getLimitSwitchesEnabled() {
  if (shouldUseJudgeMirrorDisplayPrefs()) {
    return s_judgeMirrorLimitSwitchesEnabled;
  }
  return preferences.limitSwitchesEnabled;
}

bool StateManager::getRelaysEnabled() {
  if (shouldUseJudgeMirrorDisplayPrefs()) {
    return s_judgeMirrorRelaysEnabled;
  }
  return preferences.relaysEnabled;
}

bool StateManager::isRelayEnabled(int index) {
  if (index < 0 || index >= 4) return false;
  if (shouldUseJudgeMirrorDisplayPrefs()) {
    return s_judgeMirrorRelayEnabled[index];
  }
  return preferences.relayEnabled[index];
}

int StateManager::getM4ID() {
  return preferences.M4IDNumber;
} 

int StateManager::getHostM4ID() {
  return preferences.HostM4IDNumber;
} 

// ----- Setters -----

void StateManager::setUnitSystem(UnitSystem s) {
  LOGI("setUnitSystem -> %u", (unsigned)s);
  if (preferences.unitSystem == s) return;
  preferences.unitSystem = s;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setRPM(float rpm) {
  systemState.rpm = rpm;
  if (systemState.currentPullState == PullState::PULLING && rpm > systemState.maxRpm) {
    systemState.maxRpm = rpm;
  }
}

void StateManager::setSpeed(float mph) {
  systemState.speedInMPH = mph;
  if (systemState.currentPullState == PullState::PULLING && mph > systemState.maxSpeedInMPH) {
    systemState.maxSpeedInMPH = mph;
  }
}

void StateManager::setDistance(float ft) {
  systemState.distanceInFeet = ft;
  if (systemState.currentPullState == PullState::PULLING && ft > systemState.maxDistanceInFeet) {
    systemState.maxDistanceInFeet = ft;
  }
}

void StateManager::resetMaxRPM() { systemState.maxRpm = 0.0f; }

void StateManager::resetMaxSpeed() { systemState.maxSpeedInMPH = 0.0f; }

void StateManager::resetMaxDistance() { systemState.maxDistanceInFeet = 0.0f; }

void StateManager::resetAllMaxValues() {
  resetMaxRPM();
  resetMaxSpeed();
  resetMaxDistance();
}
void StateManager::setPullState(PullState newState) { systemState.currentPullState = newState; }

PullState StateManager::getPullState() { return systemState.currentPullState; }

int StateManager::getSpeedCalibrationNumber() { return preferences.speedCalibrationPulses; }

void StateManager::setSpeedCalibrationNumber(int pulses) {
  if (preferences.speedCalibrationPulses == pulses) return;
  preferences.speedCalibrationPulses = pulses;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setScreenRotation(bool rotation180) {
  if (preferences.screenRotation180 == rotation180) return;
  preferences.screenRotation180 = rotation180;
  
  requestPreferencesSave(kDirtyGeneral);
  LOGI("Screen rotation set to %s", rotation180 ? "180°" : "0°");
}

void StateManager::setM4ID(int unitId, bool persist) {
  int clamped = unitId;
  if (clamped < 0) {
    clamped = 0;
  } else if (clamped > 9999) {
    clamped = 9999;
  }

  bool changed = preferences.M4IDNumber != clamped;
  if (changed) {
    preferences.M4IDNumber = clamped;
  }

  if (persist && changed) {
    requestPreferencesSave(kDirtyGeneral);
  }
}

void StateManager::setHostM4ID(int unitId, bool persist) {
  int clamped = unitId;
  if (clamped < 0) {
    clamped = 0;
  } else if (clamped > 9999) {
    clamped = 9999;
  }

  bool changed = preferences.HostM4IDNumber != clamped;
  if (changed) {
    preferences.HostM4IDNumber = clamped;
  }

  if (persist && changed) {
    requestPreferencesSave(kDirtyGeneral);
  }
}

void StateManager::setJudgeMode(bool isJudgeMode, bool persist) {
  bool changed = preferences.isJudgeMode != isJudgeMode;
  if (changed) {
    preferences.isJudgeMode = isJudgeMode;
    systemState.judgeMode = isJudgeMode;  // sync runtime state
  }

  if (persist && changed) {
    requestPreferencesSave(kDirtyGeneral);
  }
}

void StateManager::setJudgeMirrorUnits(UnitSystem system, float trackLengthFeet) {
  s_judgeMirrorUnitSystem = system;
  if (trackLengthFeet > 0.0f) {
    s_judgeMirrorTrackLengthFeet = trackLengthFeet;
  }
  s_hasJudgeMirrorUnits = true;
}

void StateManager::setJudgeMirrorDisplayPrefs(bool tachEnabled,
                                              bool limitSwitchesEnabled,
                                              bool relaysEnabled,
                                              const bool limitSwitchEnabled[2],
                                              const bool relayEnabled[4]) {
  s_judgeMirrorTachEnabled = tachEnabled;
  s_judgeMirrorLimitSwitchesEnabled = limitSwitchesEnabled;
  s_judgeMirrorRelaysEnabled = relaysEnabled;
  for (int i = 0; i < 2; ++i) {
    s_judgeMirrorLimitSwitchEnabled[i] = limitSwitchEnabled ? limitSwitchEnabled[i] : true;
  }
  for (int i = 0; i < 4; ++i) {
    s_judgeMirrorRelayEnabled[i] = relayEnabled ? relayEnabled[i] : true;
  }
  s_hasJudgeMirrorDisplayPrefs = true;
}

void StateManager::clearJudgeMirrorUnits() {
  s_hasJudgeMirrorUnits = false;
  s_hasJudgeMirrorDisplayPrefs = false;
}

bool StateManager::getJudgeMode() {
  return systemState.judgeMode;
}

bool StateManager::getJudgeModePreference() {
  return preferences.isJudgeMode;
}

void StateManager::setScreenBrightness(uint8_t level) {
  if (preferences.screenBrightness == level) return;
  preferences.screenBrightness = level;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setTrackLengthFeet(float feet) {
  if (feet <= 0.0f) return;                     // ignore bad values
  if (fabs(preferences.trackLengthFeet - feet) < 0.001f) return;
  preferences.trackLengthFeet = feet;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setDriverNumber(int number, bool persist) {
  if (number < 1) {
    number = 1;
  }
  if (preferences.driverNumber == number) return;
  preferences.driverNumber = number;
  if (persist) {
    requestPreferencesSave(kDirtyDriverNumber);
  }
}

void StateManager::setTachEnabled(bool on) {
  if (preferences.tachEnabled == on) return;
  preferences.tachEnabled = on;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setLimitSwitchesEnabled(bool on) {
  if (preferences.limitSwitchesEnabled == on) return;
  preferences.limitSwitchesEnabled = on;
  requestPreferencesSave(kDirtyGeneral);
}

void StateManager::setRelaysEnabled(bool on) {
  if (preferences.relaysEnabled == on) return;
  preferences.relaysEnabled = on;
  requestPreferencesSave(kDirtyGeneral);
}


bool StateManager::getScreenRotation() {
  bool rotation180 = preferences.screenRotation180;
  LOGD("Current screen rotation is %s", rotation180 ? "180°" : "0°");
  return rotation180;
}



// This is a setting. this determines if the M4 will auto-connect to the Tach Tractor Should remain here in StateManager
void StateManager::setIsAutoConnectTractor(bool autoConnect) {
  if (preferences.isAutoConnectTractor == autoConnect) return;
  preferences.isAutoConnectTractor = autoConnect;
  requestPreferencesSave(kDirtyPairing);

  LOGI("Auto-connect to Tach Tractor set to %s", autoConnect ? "ENABLED" : "DISABLED");
}

bool StateManager::getIsAutoConnectTractor() {
  return preferences.isAutoConnectTractor;
}

void StateManager::setPairedTractorAddress(const uint8_t* addr) {
  if (!addr) return;
  if (memcmp(preferences.pairedTractorAddress, addr, 6) == 0) return;
  memcpy(preferences.pairedTractorAddress, addr, 6);
  requestPreferencesSave(kDirtyPairing);
}

const uint8_t* StateManager::getPairedTractorAddress() {
  return preferences.pairedTractorAddress;
}

void StateManager::savePullResult() {
  PullResult newPull;
  const int previousCount = preferences.pullHistoryCount;
  newPull.driverName = preferences.driverName;
  newPull.driverNumber = preferences.driverNumber;
  newPull.className = preferences.pullingClassName;
  newPull.classWeight = preferences.pullingClassWeight;
  newPull.maxSpeedMPH = systemState.maxSpeedInMPH;
  newPull.maxDistanceFeet = systemState.maxDistanceInFeet;
  newPull.maxRPM = systemState.maxRpm;
  newPull.timestamp = millis() / 1000;  // Convert to seconds

  // Shift existing pulls down if at capacity
  if (preferences.pullHistoryCount >= MAX_PULL_HISTORY) {
    for (int i = 1; i < MAX_PULL_HISTORY; i++) {
      preferences.pullHistory[i - 1] = preferences.pullHistory[i];
    }
    preferences.pullHistory[MAX_PULL_HISTORY - 1] = newPull;
    requestPreferencesSave(kDirtyHistory);
  } else {
    // Add to end
    preferences.pullHistory[preferences.pullHistoryCount] = newPull;
    preferences.pullHistoryCount++;
    requestPullHistorySave(previousCount);
  }
}

int StateManager::getPullHistoryCount() {
  return preferences.pullHistoryCount;
}

const PullResult* StateManager::getPullHistory() {
  return preferences.pullHistory;
}

const PullResult* StateManager::getPullResult(int index) {
  if (index < 0 || index >= preferences.pullHistoryCount) {
    return nullptr;
  }
  return &preferences.pullHistory[index];
}

void StateManager::clearPullHistory() {
  // Reset pull history count to 0
  preferences.pullHistoryCount = 0;
  
  // Reset driver number to 1
  preferences.driverNumber = 1;
  
  // Clear all pull history entries (optional, but good practice)
  for (int i = 0; i < MAX_PULL_HISTORY; i++) {
    preferences.pullHistory[i] = PullResult{};
  }
  
  // Save preferences to persist changes
  requestPreferencesSave(kDirtyGeneral | kDirtyHistory);
  
  LOGI("Pull history cleared and driver number reset to 1");
}

RelayState StateManager::getRelayState(int index) {
  if (index < 0 || index >= 4) return RelayState::DISENGAGED;
  return systemState.relayStates[index];
}

void StateManager::setRelayState(int index, RelayState state) {
  if (index < 0 || index >= 4) return;
  systemState.relayStates[index] = state;
}

bool StateManager::getLimitSwitchTriggered(int index) {
  if (index < 0 || index >= 2) return false;
  return systemState.limitSwitchTriggered[index];
}

void StateManager::setLimitSwitchTriggered(int index, bool triggered) {
  if (index < 0 || index >= 2) return;
  systemState.limitSwitchTriggered[index] = triggered;
}

bool StateManager::isLimitSwitchEnabled(int index) {
  if (index < 0 || index >= 2) return false;
  if (shouldUseJudgeMirrorDisplayPrefs()) {
    return s_judgeMirrorLimitSwitchEnabled[index];
  }
  return preferences.limitSwitchEnabled[index];
}

void StateManager::setLimitSwitchEnabled(int index, bool enabled) {
  if (index < 0 || index >= 2) return;
  if (preferences.limitSwitchEnabled[index] == enabled) return;
  preferences.limitSwitchEnabled[index] = enabled;
  requestPreferencesSave(kDirtyGeneral);
}

// ----- Preferences -----

void StateManager::loadPreferences() {
  ensurePreferencesWriterStarted();
  PullState ps = getPullState();
  if (ps == PullState::PULLING || SpeedModule::isDriveOffModeActive()) return;
  storage.begin("m4prefs", false);

  preferences.unitSystem =
      static_cast<UnitSystem>(storage.getUChar("unitSystem", static_cast<uint8_t>(UnitSystem::IMPERIAL)));

  preferences.pullingClassName = storage.getString("className", "M4 Sled Monitor - " + String(DEVICE_VERSION));
  preferences.pullingClassWeight = storage.getInt("classWeight", 0);
  preferences.driverName = storage.getString("driverName", "Driver");
  preferences.driverNumber = storage.getInt("driverNumber", 1);

  preferences.M4IDNumber = storage.getInt("M4ID", 0);
  preferences.HostM4IDNumber = storage.getInt("HostM4ID", 0);
  preferences.isJudgeMode = storage.getBool("isJudgeMode", false);
  systemState.judgeMode = preferences.isJudgeMode;


  preferences.limitSwitchEnabled[0] = storage.getBool("ls1_enabled", true);
  preferences.limitSwitchEnabled[1] = storage.getBool("ls2_enabled", true);

  for (int i = 0; i < 4; ++i) {
    preferences.relayEnabled[i] = storage.getBool(("relayEn" + String(i)).c_str(), true);
  }

  preferences.distanceAlarm1 = storage.getFloat("distAlarm1", 0.0f);
  preferences.distanceAlarm2 = storage.getFloat("distAlarm2", 0.0f);
  preferences.tachAlarm1 = storage.getFloat("tachAlarm1", 0.0f);
  preferences.tachAlarm2 = storage.getFloat("tachAlarm2", 0.0f);
  preferences.mphAlarm1 = storage.getFloat("mphAlarm1", 0.0f);
  preferences.mphAlarm2 = storage.getFloat("mphAlarm2", 0.0f);

  preferences.trackLengthFeet = storage.getFloat("trackLength", 300.0f);

  preferences.screenBrightness = storage.getUChar("brightness", 100);
  preferences.screenRotation180 = storage.getBool("screenRot180", false);
  preferences.tachEnabled = storage.getBool("tachEnabled", true);
  preferences.limitSwitchesEnabled = storage.getBool("limitsEnabled", true);
  preferences.relaysEnabled = storage.getBool("relaysEnabled", true);

  preferences.isAutoConnectTractor = storage.getBool("autoConnTrac", true);

  preferences.speedCalibrationPulses = storage.getInt("speedCal", 1000);

  // Load M4 communication settings
  storage.getBytes("tractorAddr", preferences.pairedTractorAddress, 6);
  storage.getBytes("remoteAddr", preferences.pairedRemoteDisplayAddress, 6);
  preferences.pairingDelay = storage.getULong("pairingDelay", 10000);

  // Load pull history
  preferences.pullHistoryCount = storage.getInt("pullCount", 0);
  if (preferences.pullHistoryCount > MAX_PULL_HISTORY) {
    preferences.pullHistoryCount = MAX_PULL_HISTORY;
  }
  
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    String rowKey = makePullHistoryBlobKey(i);
    if (!loadPackedPullHistoryRow(rowKey.c_str(), preferences.pullHistory[i])) {
      String keyPrefix = "pull" + String(i) + "_";
      preferences.pullHistory[i].driverName = storage.getString((keyPrefix + "driver").c_str(), "");
      preferences.pullHistory[i].driverNumber = storage.getInt((keyPrefix + "drivNum").c_str(), 0);
      preferences.pullHistory[i].className = storage.getString((keyPrefix + "class").c_str(), "");
      preferences.pullHistory[i].classWeight = storage.getInt((keyPrefix + "weight").c_str(), 0);
      preferences.pullHistory[i].maxSpeedMPH = storage.getFloat((keyPrefix + "speed").c_str(), 0.0f);
      preferences.pullHistory[i].maxDistanceFeet = storage.getFloat((keyPrefix + "distance").c_str(), 0.0f);
      preferences.pullHistory[i].maxRPM = storage.getFloat((keyPrefix + "rpm").c_str(), 0.0f);
      preferences.pullHistory[i].timestamp = storage.getULong((keyPrefix + "time").c_str(), 0);
    }
  }

  storage.end();
  LOGD("LimitSwitchEnabled: LS1 = %d, LS2 = %d", preferences.limitSwitchEnabled[0],
       preferences.limitSwitchEnabled[1]);

  // Print pull history
  LOGD("Pull History (%d pulls):", preferences.pullHistoryCount);
  for (int i = 0; i < preferences.pullHistoryCount; i++) {
    const PullResult& pull = preferences.pullHistory[i];
    LOGD("%d: Driver %s (#%d), Speed: %.2f MPH, Distance: %.2f ft, RPM: %.1f, Time: %lu",
         i, pull.driverName.c_str(), pull.driverNumber, pull.maxSpeedMPH,
         pull.maxDistanceFeet, pull.maxRPM, pull.timestamp);
  }

  LOGI("Preferences loaded successfully.");
}

void StateManager::savePreferences() {
  requestPreferencesSave(kDirtyGeneral | kDirtyPairing | kDirtyHistory);
}

void StateManager::flushPreferencesNow() {
  dispatchPendingSaveIfIdle();
}

void StateManager::flushPreferencesNowBlocking() {
  ensurePreferencesWriterStarted();
  if (s_prefsMutex == nullptr) return;

  PullState ps = getPullState();
  if (ps == PullState::PULLING || ps == PullState::STAGED) return;
  if (SpeedModule::isDriveOffModeActive()) return;

  while (s_writerBusy) {
    vTaskDelay(1);
  }

  SystemPreferences snapshot;
  uint8_t snapshotSections = 0;
  int snapshotHistoryIndex = -1;

  xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
  if (!s_preferencesDirty || s_dirtySections == 0) {
    xSemaphoreGive(s_prefsMutex);
    return;
  }

  snapshot = preferences;
  snapshotSections = s_dirtySections;
  snapshotHistoryIndex = (s_dirtySections & kDirtyHistory) ? s_dirtyHistoryIndex : -1;
  s_dirtySections = 0;
  s_dirtyHistoryIndex = -1;
  s_dispatchNotBeforeMs = 0;
  s_preferencesDirty = false;
  xSemaphoreGive(s_prefsMutex);

  esp_err_t err = ESP_OK;
  nvs_handle_t h = 0;
  err = nvs_open("m4prefs", NVS_READWRITE, &h);
  if (err == ESP_OK && (snapshotSections & kDirtyGeneral)) {
    err = writeGeneralPreferencesToStorage(h, snapshot);
  }
  if (err == ESP_OK && (snapshotSections & kDirtyDriverNumber)) {
    err = writeDriverNumberToStorage(h, snapshot);
  }
  if (err == ESP_OK && (snapshotSections & kDirtyPairing)) {
    err = writePairingPreferencesToStorage(h, snapshot);
  }
  if (err == ESP_OK && (snapshotSections & kDirtyHistory)) {
    err = writePullHistoryToStorage(h, snapshot, snapshotHistoryIndex);
  }
  if (err == ESP_OK) {
    err = nvs_commit(h);
  }
  if (h != 0) {
    nvs_close(h);
  }

  if (err != ESP_OK) {
    xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
    mergeFailedSnapshot(snapshotSections, snapshotHistoryIndex);
    xSemaphoreGive(s_prefsMutex);
    LOGE("blocking nvs flush failed: %d", static_cast<int>(err));
  }
}

void StateManager::processPendingSave() {
  if (s_prefsMutex == nullptr || s_writerTask == nullptr) return;
  if (!s_preferencesDirty || s_writerBusy) return;
  PullState ps = getPullState();
  if (ps == PullState::PULLING || ps == PullState::STAGED) return;
  if (SpeedModule::isDriveOffModeActive()) return;

  unsigned long now = millis();
  if (now - s_lastScreenTransitionAtMs < kScreenTransitionQuietMs) return;
  if (now - s_saveRequestedAtMs < kPreferencesSaveDebounceMs) return;
  if (s_dispatchNotBeforeMs != 0 && now < s_dispatchNotBeforeMs) return;

  bool shouldNotify = false;
  xSemaphoreTake(s_prefsMutex, portMAX_DELAY);
  if (!s_writerBusy && s_preferencesDirty && s_dirtySections != 0) {
    s_writeSnapshot = preferences;
    s_snapshotSections = s_dirtySections;
    s_snapshotHistoryIndex = (s_dirtySections & kDirtyHistory) ? s_dirtyHistoryIndex : -1;
    s_dirtySections = 0;
    s_dirtyHistoryIndex = -1;
    s_preferencesDirty = false;
    s_writerBusy = true;
    shouldNotify = true;
  }
  xSemaphoreGive(s_prefsMutex);

  if (shouldNotify) {
    xTaskNotifyGive(s_writerTask);
  }
}

void StateManager::noteScreenTransition() {
  s_lastScreenTransitionAtMs = millis();
}

void StateManager::beginDeferredSavePersistence() {
  s_deferredSavePersistenceDepth++;
}

void StateManager::endDeferredSavePersistence() {
  if (s_deferredSavePersistenceDepth > 0) {
    s_deferredSavePersistenceDepth--;
  }
}

#include "M4CommsHelpers.h"
#include "AlarmManager.h"
#include "TachClient.h"

#include "../ui/ui.h"
#include "remotes/RemoteManager.h"
#include "JudgeModule.h"

#define LOG_TAG "M4CommsHelpers"
#include "Logging.h"

const uint8_t kBroadcastAddress[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

// Define the global variables declared in M4CommsHelpers.h
int CHECK_REMOTE_CONNECTION_RATE = 5000;

bool isPairing = false;
uint8_t pairedTractorAddress[6];
int pairedTractorRSSI = -100;
bool isTractorConnected = false;
unsigned long isTractorConnectedMillis = 0;

bool isRemoteDisplayConnected = false;
unsigned long isRemoteDisplayConnectedMillis = 0;
uint8_t pairedRemoteDisplayAddress[6];
int pairedRemoteRSSI = -100;

bool SendTestPatternToSignFlag = false;
bool SendSignalStrengthToSignFlag = false;

unsigned long pairingStartedMillis = 0;
unsigned long pairingDelay;
bool isWaitingForPairingDelay = false;
bool isInPull = false;

VERSION pairedTractorVersion = {0, 0, 0};
VERSION pairedRemoteVersion = {0, 0, 0};

#include "TachClient.h"
#include <ArduinoJson.h>

// If your M4Message is larger than payload+header, adjust these checks
static_assert(sizeof(M4Message) <= 256, "M4Message unexpectedly large");

void onMessageReceived(uint8_t *senderAddress,
                       uint8_t *incomingData,
                       uint8_t len,
                       int rssi,
                       bool broadcast) {
  //LOGI("[M4CommsHelpers] onMessageReceived called with len=%d", len);
  // 1) Basic frame guard
  if (!incomingData || len < sizeof(M4Message)) {
    LOGE("[M4CommsHelpers] Invalid incoming data: Bad frame size");
    return;
  }

  // 2) Copy the raw struct safely
  M4Message message;
  memcpy(&message, incomingData, sizeof(M4Message));

  // 3) Parse JSON payload
  // Tune capacity to your payload size. 256 is safe for small messages.
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message.payload);
  if (error) {
    LOGE("[M4CommsHelpers] deserializeJson() failed: %s", error.c_str());
    return;
  }

  // 4) Extract fields defensively
  action_type action = doc["action"].is<int>()
                         ? static_cast<action_type>(doc["action"].as<int>())
                         : NO_ACTION;

  //LOGD("[Action Extraction] Action: %d", action);

  // value/unit are optional in most frames
  const bool hasValue = doc["value"].is<float>();
  const float value   = hasValue ? doc["value"].as<float>() : 0.0f;

  // LOGI("[M4CommsHelpers] Received message: action=%d, value=%.2f, from %02X:%02X:%02X:%02X:%02X:%02X, rssi=%d, broadcast=%d",
  //      static_cast<int>(action),
  //      value,
  //      senderAddress[0], senderAddress[1], senderAddress[2],
  //      senderAddress[3], senderAddress[4], senderAddress[5],
  //      rssi,
  //      broadcast ? 1 : 0);

  // 6) Handle the rest of your protocol
  switch (action) {
    case PAIRING_RESPONSE: {
      //LOGI("[M4CommsHelpers] PAIRING_RESPONSE case triggered");
      Tach::onPairingResponse(senderAddress, rssi);
      break;
    }

    case PAIRING_REMOTE_RESPONSE: {
      if (RemoteManager::IsPairing()) {
        RemoteManager::OnPairingRemoteResponse(senderAddress, rssi, doc);
      }
      break;
    }

    case SEND_RPM: {
      processSendRPM(value, rssi);
      if (doc["version"].is<JsonObject>()) {
        JsonObjectConst v = doc["version"].as<JsonObjectConst>();
        updateVersion(
          pairedTractorVersion,
          v["major"].is<int>() ? v["major"].as<int>() : 0,
          v["minor"].is<int>() ? v["minor"].as<int>() : 0,
          v["patch"].is<int>() ? v["patch"].as<int>() : 0
        );
      }
      break;
    }

    case REMOTE_ACK: {
      processRemoteAck(rssi);
      if (doc["version"].is<JsonObject>()) {
        JsonObjectConst v = doc["version"].as<JsonObjectConst>();
        updateVersion(
          pairedRemoteVersion,
          v["major"].is<int>() ? v["major"].as<int>() : 0,
          v["minor"].is<int>() ? v["minor"].as<int>() : 0,
          v["patch"].is<int>() ? v["patch"].as<int>() : 0
        );
      }
      break;
    }

    case SEND_REMOTE_VALUE: {
      // REMED out for now TODO probably need to delete at somepoint. or actually use it
      // // Judge Stand fan-out
      // if (StateManager::getJudgeMode()) {
      //   int m4id = doc["M4ID"].is<int>() ? doc["M4ID"].as<int>() : -1;
      //   if (m4id == StateManager::getHostM4ID()) {
      //     bool pulling = doc["isPulling"].is<bool>() ? doc["isPulling"].as<bool>() : false;
      //     // StateManager::setIsInPull(pulling);   // uncomment when you add it

      //     float distance = doc["distance"].is<float>() ? doc["distance"].as<float>() : 0.0f;
      //     StateManager::setDistance(distance);

      //     float speed = doc["speed"].is<float>() ? doc["speed"].as<float>() : 0.0f;
      //     StateManager::setSpeed(speed);

      //     // RPM here is broadcast from the host. TachClient already updates RPM from TSS.
      //     // If Judge Mode should override, keep this. Otherwise, remove it.
      //     float rpm = doc["rpm"].is<float>() ? doc["rpm"].as<float>() : 0.0f;
      //     StateManager::setRPM(rpm);
      //   }
      // }
      break;
    }
    case SEND_JUDGE_DATA: {
      LOGD("[RX] SEND_JUDGE_DATA received");

      // Only care when we are in Judge Mode
      if (!StateManager::getJudgeMode()) {
        break;
      }

      int hostPref  = StateManager::getHostM4ID();
      int hostFromMsg = doc["h"].is<int>() ? doc["h"].as<int>()
                                           : (doc["host"].is<int>() ? doc["host"].as<int>() : -1);

      LOGD("[RX] Judge hostPref=%d msgHost=%d", hostPref, hostFromMsg);

      if (hostPref <= 0) {
        LOGD("[RX] Ignoring judge data, host MCU ID not configured");
        break;
      }

      if (hostFromMsg != hostPref) {
        LOGD("[RX] Ignoring judge data, not our host");
        break;
      }

      const char* kind = doc["k"].is<const char*>() ? doc["k"].as<const char*>() : "t";

      if (strcmp(kind, "c") == 0) {
        AlarmConfig configs[kChannels][kSlots];
        JsonArrayConst alarms = doc["a"].as<JsonArrayConst>();
        int alarmIndex = 0;

        for (int c = 0; c < kChannels; ++c) {
          for (int s = 0; s < kSlots; ++s) {
            AlarmConfig cfg{};
            if (alarmIndex < alarms.size()) {
              JsonArrayConst row = alarms[alarmIndex].as<JsonArrayConst>();
              cfg.enabled = row[0].as<int>() != 0;
              cfg.tripPoint = row[1].as<float>();
              cfg.style = static_cast<AlarmStyle>(row[2].as<int>());
              cfg.color = static_cast<AlarmColor>(row[3].as<int>());
            }
            configs[c][s] = cfg;
            ++alarmIndex;
          }
        }

        UnitSystem unitSystem = static_cast<UnitSystem>(doc["u"].is<int>() ? doc["u"].as<int>() : 0);
        float trackLengthFeet = doc["tl"].is<float>() ? doc["tl"].as<float>() : 300.0f;
        uint8_t activePreset = doc["ap"].is<int>() ? static_cast<uint8_t>(doc["ap"].as<int>()) : 0;
        JudgeModule::applyRemoteConfig(unitSystem, trackLengthFeet, activePreset, configs);
        break;
      }

      if (strcmp(kind, "r") == 0) {
        int totalCount = doc["c"].is<int>() ? doc["c"].as<int>() : 0;
        if (totalCount <= 0) {
          JudgeModule::applyRemotePullHistoryRow(0, 0, PullResult{});
          break;
        }

        PullResult pull{};
        pull.driverNumber = doc["dn"].is<int>() ? doc["dn"].as<int>() : 0;
        pull.driverName = doc["d"].is<const char*>() ? doc["d"].as<const char*>() : "";
        pull.className = doc["cn"].is<const char*>() ? doc["cn"].as<const char*>() : "";
        pull.classWeight = doc["cw"].is<int>() ? doc["cw"].as<int>() : 0;
        pull.maxSpeedMPH = doc["s"].is<float>() ? doc["s"].as<float>() : 0.0f;
        pull.maxDistanceFeet = doc["df"].is<float>() ? doc["df"].as<float>() : 0.0f;
        pull.maxRPM = doc["r"].is<float>() ? doc["r"].as<float>() : 0.0f;
        pull.timestamp = doc["t"].is<unsigned long>() ? doc["t"].as<unsigned long>() : 0;

        int index = doc["i"].is<int>() ? doc["i"].as<int>() : 0;
        JudgeModule::applyRemotePullHistoryRow(index, totalCount, pull);
        break;
      }

      if (strcmp(kind, "m") == 0) {
        int hostUnitId = doc["mid"].is<int>() ? doc["mid"].as<int>() : hostFromMsg;
        String driverName = doc["d"].is<const char*>() ? String(doc["d"].as<const char*>()) : String("");
        int driverNumber = doc["dn"].is<int>() ? doc["dn"].as<int>() : 0;
        String className = doc["cn"].is<const char*>() ? String(doc["cn"].as<const char*>()) : String("");
        JudgeModule::applyRemoteMeta(hostUnitId, driverName, driverNumber, className);
        break;
      }

      JudgeModule::HostSnapshot snap{};
      snap.host_id = static_cast<uint8_t>(hostFromMsg);

      int stateInt = doc["ps"].is<int>() ? doc["ps"].as<int>()
                    : (doc["pullState"].is<int>() ? doc["pullState"].as<int>()
                                                  : static_cast<int>(PullState::READY));
      snap.pull_state = static_cast<PullState>(stateInt);

      snap.distanceFeet = doc["d"].is<float>() ? doc["d"].as<float>()
                        : (doc["distance"].is<float>() ? doc["distance"].as<float>() : 0.0f);
      snap.speedMph     = doc["s"].is<float>() ? doc["s"].as<float>()
                        : (doc["speed"].is<float>() ? doc["speed"].as<float>() : 0.0f);
      snap.rpm          = doc["r"].is<float>() ? doc["r"].as<float>()
                        : (doc["rpm"].is<float>() ? doc["rpm"].as<float>() : 0.0f);

      snap.maxDistanceFeet = doc["md"].is<float>() ? doc["md"].as<float>()
                              : (doc["maxDistance"].is<float>() ? doc["maxDistance"].as<float>() : snap.distanceFeet);
      snap.maxSpeedMph     = doc["ms"].is<float>() ? doc["ms"].as<float>()
                              : (doc["maxSpeed"].is<float>() ? doc["maxSpeed"].as<float>() : snap.speedMph);
      snap.maxRpm          = doc["mr"].is<float>() ? doc["mr"].as<float>()
                              : (doc["maxRpm"].is<float>() ? doc["maxRpm"].as<float>() : snap.rpm);

      LOGD("[RX] Judge data host=%d state=%d dist=%.2f speed=%.2f rpm=%.1f",
          snap.host_id, stateInt, snap.distanceFeet, snap.speedMph, snap.rpm);

      uint8_t activePreset = doc["ap"].is<int>() ? static_cast<uint8_t>(doc["ap"].as<int>())
                                                 : AlarmManager::getActivePreset();
      uint8_t tripMask = doc["tm"].is<int>() ? static_cast<uint8_t>(doc["tm"].as<int>()) : 0;
      AlarmManager::applyMirrorTripMask(activePreset, tripMask);
      JudgeModule::onHostStatusBroadcast(snap);
      break;
    }

    default:
      Serial.println("Unknown action received.");
      break;
  }
}

void formatMACAddress(char *macStr, const uint8_t *macAddress) {
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", macAddress[0], macAddress[1],
          macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
}

void updateDisplayWithMAC(uint8_t *macAddress, lv_obj_t *btn, lv_obj_t *label) {
  char btnLabel[5];  // Buffer for last 4 digits + null terminator
  // Format the last 4 digits of the MAC address
  snprintf(btnLabel, sizeof(btnLabel), "%02X%02X", macAddress[4],
           macAddress[5]);

  if (btn && label) {
    showUIObject(btn, true);  // Show the button if it's not already visible
    lv_label_set_text(
        label,
        btnLabel);  // Set the label to the last 4 digits of the MAC address
  }
}

void processSendRPM(const float value, int rssi) {
  StateManager::setRPM(value);
  Tach::state.pairedTSSRSSI = rssi;
  Tach::state.isTSSConnected = true;
  LOGI("[Tach] RPM updated to %.2f from TSS with RSSI %d", value, rssi);
}

void processRemoteAck(int rssi) {
  isRemoteDisplayConnected = true;
  isRemoteDisplayConnectedMillis = millis();
  pairedRemoteRSSI = rssi;
}

int convertRssiToSignalStrength(int rssi) {
  const int MIN_RSSI = -90;
  const int MAX_RSSI = 0;
  const int MIN_STRENGTH = 0;
  const int MAX_STRENGTH = 1000;

  if (rssi <= MIN_RSSI) {
    return MIN_STRENGTH;
  }
  if (rssi >= MAX_RSSI) {
    return MAX_STRENGTH;
  }

  return (rssi - MIN_RSSI) * (MAX_STRENGTH - MIN_STRENGTH) /
         (MAX_RSSI - MIN_RSSI);
}

String convertRssiToSignalQuality(int rssi) {
  if (rssi > -30) return "Amazing";
  if (rssi > -55) return "Good";
  if (rssi > -67) return "Fair";
  if (rssi > -70) return "Okay";
  if (rssi > -80) return "Poor";
  if (rssi > -90) return "Weak";
  return "Unusable";
}

void changeRSSISymbol(signed int rssi, wirelessSymbols symbol) {
  // TODO Carry over from Tach Monitor UI
  // switch (symbol) {
  //   case REMOTE_SETTINGS: {
  //     // Hide all wireless symbol images first ex: ui_Wireless0 through
  //     // ui_Wireless5
  //     showUIObject(ui_Wireless0, false);
  //     showUIObject(ui_Wireless1, false);
  //     showUIObject(ui_Wireless2, false);
  //     showUIObject(ui_Wireless3, false);
  //     showUIObject(ui_Wireless4, false);
  //     showUIObject(ui_Wireless5, false);

  //     // Show the correct wireless symbol image based on the rssi value
  //     if (rssi > -30)
  //       showUIObject(ui_Wireless0, true);
  //     else if (rssi > -55)
  //       showUIObject(ui_Wireless1, true);
  //     else if (rssi > -67)
  //       showUIObject(ui_Wireless2, true);
  //     else if (rssi > -70)
  //       showUIObject(ui_Wireless3, true);
  //     else if (rssi > -80)
  //       showUIObject(ui_Wireless4, true);
  //     else if (rssi > -90)
  //       showUIObject(ui_Wireless5, true);
  //   }
  //   case REMOTE_RUN: {
  //     // Hide all wireless symbol images first ex: ui_RunRemoteWireless0
  //     // through ui_RunRemoteWireless5
  //     showUIObject(ui_RunRemoteWireless0, false);
  //     showUIObject(ui_RunRemoteWireless1, false);
  //     showUIObject(ui_RunRemoteWireless2, false);
  //     showUIObject(ui_RunRemoteWireless3, false);
  //     showUIObject(ui_RunRemoteWireless4, false);
  //     showUIObject(ui_RunRemoteWireless5, false);

  //     // Show the correct wireless symbol image based on the rssi value
  //     if (rssi > -30)
  //       showUIObject(ui_RunRemoteWireless5, true);
  //     else if (rssi > -55)
  //       showUIObject(ui_RunRemoteWireless4, true);
  //     else if (rssi > -67)
  //       showUIObject(ui_RunRemoteWireless3, true);
  //     else if (rssi > -70)
  //       showUIObject(ui_RunRemoteWireless2, true);
  //     else if (rssi > -80)
  //       showUIObject(ui_RunRemoteWireless1, true);
  //     else if (rssi > -90)
  //       showUIObject(ui_RunRemoteWireless0, true);
  //   }
  //   case TRACTOR_RUN: {
  //     // Hide all wireless symbol images first ex: ui_RunTractorWireless0
  //     // through ui_RunTractorWireless5
  //     showUIObject(ui_RunTractorWireless0, false);
  //     showUIObject(ui_RunTractorWireless1, false);
  //     showUIObject(ui_RunTractorWireless2, false);
  //     showUIObject(ui_RunTractorWireless3, false);
  //     showUIObject(ui_RunTractorWireless4, false);
  //     showUIObject(ui_RunTractorWireless5, false);

  //     // Show the correct wireless symbol image based on the rssi value
  //     if (rssi > -30)
  //       showUIObject(ui_RunTractorWireless5, true);
  //     else if (rssi > -55)
  //       showUIObject(ui_RunTractorWireless4, true);
  //     else if (rssi > -67)
  //       showUIObject(ui_RunTractorWireless3, true);
  //     else if (rssi > -70)
  //       showUIObject(ui_RunTractorWireless2, true);
  //     else if (rssi > -80)
  //       showUIObject(ui_RunTractorWireless1, true);
  //     else if (rssi > -90)
  //       showUIObject(ui_RunTractorWireless0, true);
  //   }
  // }
}

void signalStrengthHelper() {
  // if tractor is connected then show ui_RunTractorContainer and hide
  // ui_NoTractorConnectedLabelContainer
  if (isTractorConnected) {
        // TODO update Display stuff from tach

    // showUIObject(ui_RunTractorContainer, true);
    // showUIObject(ui_NoTractorConnectedLabelContainer, false);
    // // update D35 with the signal strength
    // // change ui_WirelessSymbol to a different icon
    // changeRSSISymbol(pairedTractorRSSI, TRACTOR_RUN);

    // // update label ui_RemoteRSSIScaleLabel with the signal strength scale
    // // cast to a string
    // lv_label_set_text(
    //     ui_RunTractorRSSIScaleLabel,
    //     String(convertRssiToSignalStrength(pairedTractorRSSI)).c_str());
    // // update label ui_RemoteRSSITextLabel with the signal strength text cast
    // // to a string
    // lv_label_set_text(ui_RunTractorRSSITextLabel,
    //                   convertRssiToSignalQuality(pairedTractorRSSI).c_str());
    // // update label ui_RemoteRSSILabel with signal strenght as "RSSI: -30dBm"
    char buff[25];
    snprintf(buff, sizeof(buff), "RSSI: %d dBm", pairedTractorRSSI);
    // lv_label_set_text(ui_RunTractorRSSILabel, buff);
  } else {
    // showUIObject(ui_RunTractorContainer, false);
    // showUIObject(ui_NoTractorConnectedLabelContainer, true);
  }
  // if isRemoteDisplayConnected then show ui_RemoteContainer and hide
  // ui_NoRemoteConnectedLabelContainer
  
}

void broadcastJudgeStandState() {
  static constexpr uint8_t kBroadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                   0xFF};
  static constexpr const char *kChannelNames[kChannels] = {"distance", "speed",
                                                           "rpm"};

  const auto &systemState = StateManager::state();
  const auto &preferences = StateManager::prefs();

  JsonDocument doc;
  String payload;
  payload.reserve(256);

  char macStr[18] = {0};
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           deviceSelf.getMacAddress()[0], deviceSelf.getMacAddress()[1],
           deviceSelf.getMacAddress()[2], deviceSelf.getMacAddress()[3],
           deviceSelf.getMacAddress()[4], deviceSelf.getMacAddress()[5]);

  doc["macAddress"] = macStr;
  doc["name"] = deviceSelf.getName();
  doc["type"] = deviceSelf.getType();
  doc["action"] = SEND_REMOTE_VALUE;
  doc["currentMph"] = systemState.speedInMPH;
  doc["currentRpm"] = systemState.rpm;
  doc["maxRpm"] = systemState.maxRpm;
  doc["m4Id"] = preferences.M4IDNumber;

  JsonArray alarmsArray = doc["alarms"].to<JsonArray>();
  for (uint8_t channel = 0; channel < kChannels; ++channel) {
    for (uint8_t slot = 0; slot < kSlots; ++slot) {
      const AlarmConfig config = AlarmManager::getConfigActive(
          static_cast<AlarmChannel>(channel),
          static_cast<AlarmSlot>(slot));
      if (!config.enabled || !config.tripped) {
        continue;
      }
    JsonObject alarm = alarmsArray.add<JsonObject>();
      alarm["channel"] = kChannelNames[channel];
      alarm["slot"] = slot + 1;
      alarm["tripPoint"] = config.tripPoint;
      alarm["style"] = static_cast<int>(config.style);
      alarm["color"] = static_cast<int>(config.color);
    }
  }

  serializeJson(doc, payload);

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, kBroadcastAddress, sizeof(kBroadcastAddress));
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_del_peer(kBroadcastAddress);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add judge broadcast peer");
    return;
  }

  sendMessage(kBroadcastAddress, BROADCAST, SEND_REMOTE_VALUE, payload,
              HIGH_PRIORITY);
}

void updateRemoteDisplay(float value) {
  // If in remote pairing mode exit immedietly
  if (RemoteManager::IsPairing()) {
    return;
  }
  // Create a JSON object and populate it
  JsonDocument doc;
  String payload;

  // doc["macAddress"] = deviceSelf.getMacAddress();
  // Convert MAC address to a human-readable string
  char macStr[18] = {0};  // 17 for MAC address in format XX:XX:XX:XX:XX:XX +
                          // 1 for null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           deviceSelf.getMacAddress()[0], deviceSelf.getMacAddress()[1],
           deviceSelf.getMacAddress()[2], deviceSelf.getMacAddress()[3],
           deviceSelf.getMacAddress()[4], deviceSelf.getMacAddress()[5]);

  doc["macAddress"] = macStr;

  doc["name"] = deviceSelf.getName();
  doc["type"] = deviceSelf.getType();
  doc["action"] = SEND_REMOTE_VALUE;
  // doc["value"] = value;
  doc["value"] = static_cast<int>(value);
  doc["units"] = "RPM";

  // Convert JSON object to String
  serializeJson(doc, payload);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, pairedRemoteDisplayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Delete the peer in case it is already added
  esp_now_del_peer(pairedRemoteDisplayAddress);

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  sendMessage(pairedRemoteDisplayAddress, RESPONSE, REMOTE_VALUE_RESPONSE,
              payload, HIGH_PRIORITY);
}

void updateRemoteDisplay(String value) {
  // Create a JSON object and populate it
  JsonDocument doc;
  String payload;

  // doc["macAddress"] = deviceSelf.getMacAddress();
  // Convert MAC address to a human-readable string
  char macStr[18] = {0};  // 17 for MAC address in format XX:XX:XX:XX:XX:XX +
                          // 1 for null terminator
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           deviceSelf.getMacAddress()[0], deviceSelf.getMacAddress()[1],
           deviceSelf.getMacAddress()[2], deviceSelf.getMacAddress()[3],
           deviceSelf.getMacAddress()[4], deviceSelf.getMacAddress()[5]);

  doc["macAddress"] = macStr;

  doc["name"] = deviceSelf.getName();
  doc["type"] = deviceSelf.getType();
  doc["action"] = SEND_REMOTE_VALUE;
  // doc["value"] = value;
  doc["value"] = value;
  doc["units"] = "RPM";

  // Convert JSON object to String
  serializeJson(doc, payload);

  // Register peer
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, pairedRemoteDisplayAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Delete the peer in case it is already added
  esp_now_del_peer(pairedRemoteDisplayAddress);

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  sendMessage(pairedRemoteDisplayAddress, RESPONSE, REMOTE_VALUE_RESPONSE,
              payload, HIGH_PRIORITY);
}

// Function to handle the test pattern update
void handleTestPattern(bool &SendTestPatternToSignFlag) {
  static unsigned long lastUpdateTime = 0;
  static int currentNumber = 1234;
  const unsigned long updateInterval =
      1000;  // Update every 1000 milliseconds (1 second)

  unsigned long currentTime = millis();
  if (SendTestPatternToSignFlag &&
      currentTime - lastUpdateTime >= updateInterval) {
    updateRemoteDisplay(currentNumber);
    lastUpdateTime = currentTime;

    // Logic to increment each digit, wrapping around as described
    int thousands = (currentNumber / 1000) % 10;
    int hundreds = (currentNumber / 100) % 10;
    int tens = (currentNumber / 10) % 10;
    int ones = currentNumber % 10;

    // Increment digits
    thousands = (thousands + 1) % 10;
    hundreds = (hundreds + 1) % 10;
    tens = (tens + 1) % 10;
    ones = (ones + 1) % 10;

    // Construct the new number
    currentNumber = thousands * 1000 + hundreds * 100 + tens * 10 + ones;

    // Reset to 1234 if the currentNumber exceeds 9012
    if (currentNumber > 9012) {
      currentNumber = 1234;
    }
  } else if (SendSignalStrengthToSignFlag &&
             currentTime - lastUpdateTime >= updateInterval) {
    updateRemoteDisplay(convertRssiToSignalStrength(pairedRemoteRSSI));
    lastUpdateTime = currentTime;
  }
}

bool isAddressEmpty(const uint8_t *address, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (address[i] != 0) {
      return false;  // Found a non-zero byte, address is not empty
    }
  }
  return true;  // All bytes were 0, address is empty
}

void pairingStateSwitcher() {
  if (isPairing) {
    float speedupPairingFactor = 1.0f;
    if (!isAddressEmpty(pairedTractorAddress, 6)) {
      speedupPairingFactor = 0.5f;
    }

    if (millis() - pairingStartedMillis >=
        (pairingDelay * speedupPairingFactor)) {
      isWaitingForPairingDelay = false;  // End the waiting period
      isPairing = false;
      // Check if the pairing was successful
      if (!isAddressEmpty(pairedTractorAddress, 6)) {
        // Pairing was successful
        isInPull = true;
        // Set run screen ui buttons to show end pull button
        // showUIObject(ui_EndPullButton, true);
        // showUIObject(ui_PairButton, false);
        // showUIObject(ui_PairingIndicator, false);

        // Hide and show the test Remote Buttons
        showRemoteTestButtons(false);
      } else {
        // Pairing was unsuccessful
        isInPull = false;
        // Set run screen ui buttons to show Pair button
        // showUIObject(ui_PairButton, true);
        // showUIObject(ui_PairingIndicator, false);
        // showUIObject(ui_EndPullButton, false);
        // showRemoteTestButtons(true);
      }
    }
  }
}

void showUIObject(lv_obj_t *obj, bool show) {
  _ui_flag_modify(obj, LV_OBJ_FLAG_HIDDEN,
                  show ? _UI_MODIFY_FLAG_REMOVE : _UI_MODIFY_FLAG_ADD);
}

void showRemoteTestButtons(bool show) {
  // This code was brought over from the Tach Monitor UI. TODO: Update this
  // showUIObject(ui_testPatternBtn, show);
  // showUIObject(ui_signalTestBtn, show);
  // showUIObject(ui_noTestLabel, !show);
}

void updateVersion(struct VERSION &version, int major, int minor, int patch) {
  version.major = major;
  version.minor = minor;
  version.patch = patch;
}

void initializeM4Communications() {
  // Load persistent data from StateManager
  const uint8_t* tractorAddr = StateManager::getPairedTractorAddress();
  const uint8_t* remoteAddr = StateManager::getPairedRemoteDisplayAddress();
  
  memcpy(pairedTractorAddress, tractorAddr, 6);
  memcpy(pairedRemoteDisplayAddress, remoteAddr, 6);
  pairingDelay = StateManager::getPairingDelay();
}

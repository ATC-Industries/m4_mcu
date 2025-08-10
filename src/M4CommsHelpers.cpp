#include "M4CommsHelpers.h"

#include "../ui/ui.h"

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

void onMessageReceived(uint8_t *senderAddress, uint8_t *incomingData,
                       uint8_t len, int rssi, bool broadcast) {
  M4Message message;
  memcpy(&message, incomingData, sizeof(message));

  JsonDocument doc;

  auto error = deserializeJson(doc, message.payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  Serial.print("Received message: ");
  Serial.println(message.payload);

  action_type action = doc["action"];
  const float value = doc["value"];
  const char *unit = doc["unit"];
  switch (action) {
    case PAIRING_RESPONSE:
      processPairingResponse(senderAddress, rssi);
      break;
    case PAIRING_REMOTE_RESPONSE:
      processPairingRemoteResponse(senderAddress);
      break;
    case SEND_RPM:
      processSendRPM(value, rssi);
      if (doc.containsKey("version")) {
        JsonObject versionObj = doc["version"].as<JsonObject>();
        updateVersion(pairedTractorVersion, versionObj["major"].as<int>(),
                      versionObj["minor"].as<int>(),
                      versionObj["patch"].as<int>());
      }
      break;
    case REMOTE_ACK:
      processRemoteAck(rssi);
      if (doc.containsKey("version")) {
        JsonObject versionObj = doc["version"].as<JsonObject>();
        updateVersion(pairedRemoteVersion, versionObj["major"].as<int>(),
                      versionObj["minor"].as<int>(),
                      versionObj["patch"].as<int>());
      }
      break;
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

void processPairingResponse(uint8_t *senderAddress, int rssi) {
  if (isPairing && rssi > pairedTractorRSSI) {
    char macStr[18];
    formatMACAddress(macStr, senderAddress);
    Serial.println("Action: PAIRING_RESPONSE");
    Serial.print("Found a closer tractor (Sender Address): ");
    Serial.println(macStr);

    memcpy(pairedTractorAddress, senderAddress, 6);
    StateManager::setPairedTractorAddress(senderAddress);
    pairedTractorRSSI = rssi;
    //peakRPM = 0.00;
    StateManager::resetMaxRPM();

    formatMACAddress(macStr, pairedTractorAddress);
    Serial.print("New paired tractor (pairedTractorAddress): ");
    Serial.println(macStr);

    Serial.print("New paired tractor with RSSI: ");
    Serial.println(pairedTractorRSSI);
  }
}

void processPairingRemoteResponse(uint8_t *senderAddress) {
  char macStr[18];
  formatMACAddress(macStr, senderAddress);
  Serial.print("Found a Remote Display (Sender Address): ");
  Serial.println(macStr);
  
  // This code was brought over from the Tach Monitor UI. TODO: Update this
  // for (int i = 0; i < MAX_PAIRABLE_REMOTE_DISPLAYS; i++) {
  //   if (pairableDevicesMAC[i][0] == 0) {  // Assuming 0 indicates an empty slot
  //     memcpy(pairableDevicesMAC[i], senderAddress, 6);

  //     lv_obj_t *btn = NULL;
  //     lv_obj_t *label = NULL;
  //     switch (i) {
  //       case 0:
  //         btn = ui_AvailableSignBtn1;
  //         label = ui_AvailableSignBtnLabel1;
  //         break;
  //       case 1:
  //         btn = ui_AvailableSignBtn2;
  //         label = ui_AvailableSignBtnLabel2;
  //         break;
  //       case 2:
  //         btn = ui_AvailableSignBtn3;
  //         label = ui_AvailableSignBtnLabel3;
  //         break;
  //       case 3:
  //         btn = ui_AvailableSignBtn4;
  //         label = ui_AvailableSignBtnLabel4;
  //         break;
  //       default:
  //         Serial.println("Error: No available slot for new remote display.");
  //         return;
  //     }

  //     updateDisplayWithMAC(senderAddress, btn, label);
  //     break;  // Exit loop after adding the MAC address and updating the display
  //   }
  // }
}

void processSendRPM(const float value, int rssi) {
  StateManager::setRPM(value);
  pairedTractorRSSI = rssi;
  isTractorConnected = true;
  isTractorConnectedMillis = millis();
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

void isRemoteDisplayConnectedHelper() {
  if (millis() - isRemoteDisplayConnectedMillis >=
      CHECK_REMOTE_CONNECTION_RATE) {
    isRemoteDisplayConnected = false;
  }
  if (isRemoteDisplayConnected) {
    // TODO update Display stuff from tach
    // showUIObject(ui_isRemoteDisplayConnectedStatusIcon, true);
    // // Change text of ui_pairedStatusLabel to "PAIRED"
    // lv_label_set_text(ui_pairedStatusLabel, LV_SYMBOL_OK " PAIRED");
    // // clear object state for ui_testPatternBtn to undisable button
    // _ui_state_modify(ui_testPatternBtn, LV_STATE_DISABLED,
    //                  _UI_MODIFY_FLAG_REMOVE);
    // // clear object state for ui_signalTestBtn to undisable button
    // _ui_state_modify(ui_signalTestBtn, LV_STATE_DISABLED,
    //                  _UI_MODIFY_FLAG_REMOVE);
    // // Show Wireless signal strength
    // showUIObject(ui_RemoteRSSIStrengthContainer, true);
    // // update D35 with the signal strength
    // // change ui_WirelessSymbol to a different icon
    // changeRSSISymbol(pairedRemoteRSSI, REMOTE_SETTINGS);

    // // update label ui_RemoteRSSIScaleLabel with the signal strength scale
    // // cast to a string
    // lv_label_set_text(
    //     ui_RemoteRSSIScaleLabel,
    //     String(convertRssiToSignalStrength(pairedRemoteRSSI)).c_str());
    // // update label ui_RemoteRSSITextLabel with the signal strength text cast
    // // to a string
    // lv_label_set_text(ui_RemoteRSSITextLabel,
    //                   convertRssiToSignalQuality(pairedRemoteRSSI).c_str());
    // update label ui_RemoteRSSILabel with signal strenght as "RSSI: -30dBm"
    char buff[25];
    snprintf(buff, sizeof(buff), "RSSI: %d dBm", pairedRemoteRSSI);
    // lv_label_set_text(ui_RemoteRSSILabel, buff);
  } else {
        // TODO update Display stuff from tach

    // showUIObject(ui_isRemoteDisplayConnectedStatusIcon, false);
    // // Change text of ui_pairedStatusLabel to "NOT PAIRED"
    // lv_label_set_text(ui_pairedStatusLabel, LV_SYMBOL_CLOSE " NOT PAIRED");
    // // add object state for ui_testPatternBtn to disable button
    // lv_obj_add_state(ui_testPatternBtn, LV_STATE_DISABLED);
    // // add object state for ui_signalTestBtn to disable button
    // lv_obj_add_state(ui_signalTestBtn, LV_STATE_DISABLED);
    // // Hide Wireless signal strength
    // showUIObject(ui_RemoteRSSIStrengthContainer, false);
  }
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
  if (isRemoteDisplayConnected) {
    // showUIObject(ui_RunRemoteContainer, true);
    // showUIObject(ui_NoRemoteConnectedLabelContainer, false);
    // // update D35 with the signal strength
    // // change ui_WirelessSymbol to a different icon
    // changeRSSISymbol(pairedRemoteRSSI, REMOTE_RUN);

    // // update label ui_RemoteRSSIScaleLabel with the signal strength scale
    // // cast to a string
    // lv_label_set_text(
    //     ui_RunRemoteRSSIScaleLabel,
    //     String(convertRssiToSignalStrength(pairedRemoteRSSI)).c_str());
    // // update label ui_RemoteRSSITextLabel with the signal strength text cast
    // // to a string
    // lv_label_set_text(ui_RunRemoteRSSITextLabel,
    //                   convertRssiToSignalQuality(pairedRemoteRSSI).c_str());
    // // update label ui_RemoteRSSILabel with signal strenght as "RSSI: -30dBm"
    char buff[25];
    snprintf(buff, sizeof(buff), "RSSI: %d dBm", pairedRemoteRSSI);
    // lv_label_set_text(ui_RunRemoteRSSILabel, buff);
  } else {
    // showUIObject(ui_RunRemoteContainer, false);
    // showUIObject(ui_NoRemoteConnectedLabelContainer, true);
  }
}

void updateRemoteDisplay(float value) {
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
    int speedupPairingFactor = 1;
    if (!isAddressEmpty(pairedTractorAddress, 6)) {
      speedupPairingFactor = .5;
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


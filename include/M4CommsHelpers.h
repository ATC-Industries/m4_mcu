#ifndef M4COMMSHELPERS_H
#define M4COMMSHELPERS_H

#include <lvgl.h>
#include <Arduino.h>

#include "Config.h"
#include "StateManager.h"

extern const uint8_t kBroadcastAddress[6];

extern int CHECK_REMOTE_CONNECTION_RATE;

extern bool isPairing;
extern uint8_t pairedTractorAddress[6];
extern int pairedTractorRSSI;
extern bool isTractorConnected;
extern unsigned long isTractorConnectedMillis;

extern bool isRemoteDisplayConnected;
extern unsigned long isRemoteDisplayConnectedMillis;
extern uint8_t pairedRemoteDisplayAddress[6];
extern int pairedRemoteRSSI;

extern bool SendTestPatternToSignFlag;
extern bool SendSignalStrengthToSignFlag;

extern unsigned long pairingStartedMillis;
extern unsigned long pairingDelay;
extern bool isWaitingForPairingDelay;
extern bool isInPull;

extern VERSION pairedTractorVersion;
extern VERSION pairedRemoteVersion;

enum wirelessSymbols { REMOTE_SETTINGS, REMOTE_RUN, TRACTOR_RUN };

// Functions for message processing and pairing responses
void processPairingResponse(uint8_t *senderAddress, int rssi);
void processPairingRemoteResponse(uint8_t *senderAddress);
void processSendRPM(const float value, int rssi);
void processRemoteAck(int rssi);

// Utility functions for MAC address formatting and UI updates
void formatMACAddress(char *macStr, const uint8_t *macAddress);
void onMessageReceived(uint8_t *senderAddress, uint8_t *incomingData,
                       uint8_t len, int rssi, bool broadcast);
void updateDisplayWithMAC(uint8_t *macAddress, lv_obj_t *btn, lv_obj_t *label);

// Helper functions for remote display connectivity and updates
void isRemoteDisplayConnectedHelper();
void updateRemoteDisplay(float value);
void updateRemoteDisplay(String value);
void broadcastJudgeStandState();

// Test pattern update handling
void handleTestPattern(bool &SendTestPatternToSignFlag);

// Address checking and pairing state management
bool isAddressEmpty(const uint8_t *address, size_t size);
void pairingStateSwitcher();

// RSSI conversion and signal strength handling
int convertRssiToSignalStrength(int rssi);
String convertRssiToSignalQuality(int rssi);
void changeRSSISymbol(signed int rssi, wirelessSymbols symbol);

// UI element visibility management
void showRemoteTestButtons(bool show);
void signalStrengthHelper();
void showUIObject(lv_obj_t *obj, bool show);

void updateVersion(struct VERSION &version, int major, int minor, int patch);

// Initialization function to load persistent data from StateManager
void initializeM4Communications();

#endif // M4COMMSHELPERS_H

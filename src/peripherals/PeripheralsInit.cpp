#include "peripherals/PeripheralsInit.h"
#include "PullStateManager.h"
#include "StateManager.h"
#include "lvgl/lvgl.h"
#include "ui/screens/ui_ScreenMain.h"

#define LOG_TAG "PeripheralsInit"
#include "Logging.h"

Mcp23017 g_mcp(MCP23017_ADDR);
Horn     g_horn(&g_mcp, MCP_HORN_PORTA_PIN, MCP_HORN_ACTIVE_HIGH == 1);
Button   g_button(IO_BUTTON, true, 30, 600);  // active_low, debounce, longpress

static void onButtonPress() {
  lv_obj_t* const active_screen = lv_scr_act();
  if (ui_ScreenMain != nullptr && active_screen == ui_ScreenMain) {
    switch (StateManager::getPullState()) {
      case PullState::READY:
        LOGI("Button pressed in main screen - handling READY/STAGE action");
        PullStateManager::handleStagePressed(nullptr);
        break;
      case PullState::STAGED:
        LOGI("Button pressed in main screen - handling STAGED/CANCEL action");
        PullStateManager::handleCancelPressed();
        break;
      case PullState::PULLING:
        LOGI("Button pressed in main screen - handling PULLING/STOP action");
        PullStateManager::handleStopPressed();
        break;
      case PullState::PULLEND:
        LOGI("Button pressed in main screen - handling PULLEND/SAVE action");
        PullStateManager::handleSavePressed();
        break;
      default:
        LOGI("Button pressed in main screen but no action is mapped for pull state %d",
             (int)StateManager::getPullState());
        break;
    }
    g_horn.pulse(150);
  }
  else {
    LOGI("Button pressed but ignored - not in main screen");
  }
  
}

//TODO figure our how to add parameters to callback this code doesnt work lol
// static void onButtonPress(int x, int y, int button_ms, int horn_ms) {
//   LOGI("Button pressed");
//   touch_inject_press(x, y, button_ms);
//   g_horn.pulse(horn_ms);
// }

void PeripheralsInit() {
  g_mcp.begin(&Wire);
  g_horn.begin();
  g_button.begin();

  // assign callback
  g_button.onPress(onButtonPress);

  g_horn.pulse(50);   // confirm startup
}

void PeripheralsPoll() {
  g_button.update();
}

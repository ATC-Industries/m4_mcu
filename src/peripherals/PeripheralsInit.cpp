#include "peripherals/PeripheralsInit.h"
#include "StateManager.h"
#include "peripherals/touch_inject.h"


Mcp23017 g_mcp(MCP23017_ADDR);
Horn     g_horn(&g_mcp, MCP_HORN_PORTA_PIN, MCP_HORN_ACTIVE_HIGH == 1);
Button   g_button(IO_BUTTON, true, 30, 600);  // active_low, debounce, longpress

static void onButtonPress() {
  Serial.println("Button Pressed");
  touch_inject_press(358, 60, 60);
  g_horn.pulse(150);
}

//TODO figure our how to add parameters to callback
// static void onButtonPress(int x, int y, int button_ms, int horn_ms) {
//   Serial.println("Button Pressed");
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

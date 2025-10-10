// PeripheralsInit.h
#ifndef PERIPHERALS_INIT_H_
#define PERIPHERALS_INIT_H_

#include <Arduino.h>
#include "peripherals/mcp23017.h"
#include "peripherals/Horn.h"
#include "peripherals/Button.h"
#include "Config.h"

extern Mcp23017 g_mcp;
extern Horn     g_horn;
extern Button   g_button;

void PeripheralsInit();
void PeripheralsPoll();

#endif

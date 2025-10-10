#ifndef PERIPHERALS_MCP23017_H_
#define PERIPHERALS_MCP23017_H_

#include <Arduino.h>
#include <Wire.h>

#include "Config.h"

class Mcp23017 {
 public:
  explicit Mcp23017(uint8_t i2c_addr);
  bool begin(TwoWire* wire = &Wire);

  // Port A helpers
  void pinModeA(uint8_t pin, uint8_t mode);
  void writeA(uint8_t pin, bool high);
  bool readA(uint8_t pin);

 private:
  uint8_t addr_;
  TwoWire* wire_;

  // bank 0, sequential op
  static constexpr uint8_t IODIRA = 0x00;
  static constexpr uint8_t GPIOA  = 0x12;
  static constexpr uint8_t OLATA  = 0x14;

  uint8_t regRead(uint8_t reg);
  void regWrite(uint8_t reg, uint8_t val);
};

#endif  // PERIPHERALS_MCP23017_H_

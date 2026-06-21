#include <Arduino.h>
#include "peripherals/mcp23017.h"

Mcp23017::Mcp23017(uint8_t i2c_addr) : addr_(i2c_addr), wire_(nullptr) {}

bool Mcp23017::begin(TwoWire* wire) {
  wire_ = wire;
  wire_->begin(I2C_SDA_PIN, I2C_SCL_PIN);
  // Leave device in default bank=0, seq-op, no MIRROR
  return true;
}

void Mcp23017::pinModeA(uint8_t pin, uint8_t mode) {
  uint8_t dir = regRead(IODIRA);
  if (mode == OUTPUT) dir &= ~(1U << pin);
  else                dir |=  (1U << pin);
  regWrite(IODIRA, dir);
}

void Mcp23017::writeA(uint8_t pin, bool high) {
  uint8_t val = regRead(GPIOA);
  if (high) val |=  (1U << pin);
  else      val &= ~(1U << pin);
  regWrite(OLATA, val);
}

bool Mcp23017::readA(uint8_t pin) {
  return (regRead(GPIOA) >> pin) & 0x1;
}

uint8_t Mcp23017::regRead(uint8_t reg) {
  wire_->beginTransmission(addr_);
  wire_->write(reg);
  wire_->endTransmission();
  wire_->requestFrom(addr_, static_cast<uint8_t>(1));
  return wire_->read();
}

void Mcp23017::regWrite(uint8_t reg, uint8_t val) {
  wire_->beginTransmission(addr_);
  wire_->write(reg);
  wire_->write(val);
  wire_->endTransmission();
}

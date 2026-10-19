/*
   labelbee pin map: ESP32-S3-N16R8 (DevKitC-1 clone), wiring in docs/schaltplan.md.

   GPIO19 (D-) and GPIO20 (D+) are the native USB PHY; the printer's USB-A socket hangs on them,
   driven by the USB host library, not by GPIO code. GPIO35-37 belong to the octal PSRAM.
*/

#pragma once

#include <driver/gpio.h>

namespace board {

/* Relay module (SRD-03VDC-SL-C, jumper on high trigger): high switches the printer's VBUS on.
   NO contact, so the printer stays off while the pin floats during power-up. The pin's level is
   held (gpio_hold_en) through software restarts and updates, so the printer stays on and keeps
   its USB session instead of rebooting with the ESP32. */
constexpr gpio_num_t PIN_RELAY = GPIO_NUM_4;

inline bool g_printer_power = false;

inline void set_printer_power(bool on) {
  g_printer_power = on;
  gpio_hold_dis(PIN_RELAY);
  gpio_set_level(PIN_RELAY, on ? 1 : 0);
  gpio_hold_en(PIN_RELAY);
}

inline bool printer_power() { return g_printer_power; }

/* After a software restart the held pin keeps its level; configuring it here does not reach it. */
inline void init() {
  gpio_config_t out = {};
  out.pin_bit_mask  = 1ULL << PIN_RELAY;
  out.mode          = GPIO_MODE_OUTPUT;
  out.pull_down_en  = GPIO_PULLDOWN_ENABLE;
  gpio_config(&out);
}

}  // namespace board

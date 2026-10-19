/*
   The DYMO LabelManager PnP on the USB host port.

   The printer first shows up as a mass storage device (0922:1001). It is told to switch with
   ESC Z 1 on its bulk OUT endpoint and comes back as a HID printer (0922:1002), which takes the
   label's command stream in output reports of 64 bytes.

   Everything the printer reports about itself (speed, descriptors, endpoints, the HID report
   descriptor) is logged when it connects.
*/

#pragma once

#include "label.h"

#include <esp_err.h>

#include <cstdint>
#include <string>
#include <vector>

namespace printer {

/* Registers the USB host client. The USB host library must be installed first. */
void start();

/* One line on what is connected: nothing, the printer in storage or printer mode, or another device. */
std::string describe();

/* Is a printer in printer mode connected, ready for send() and status()? */
bool ready();

/* Sends raw bytes as output reports, no flow control (tests). ESP_ERR_INVALID_STATE if no printer is ready. */
esp_err_t send(const std::vector<uint8_t> &data);

/* Asks for the printer's status (ESC A) and returns its answer, if it gives one within a second. */
esp_err_t status(std::vector<uint8_t> &reply);

/* The printer's power, through the relay. On from boot; off only when switched off in the settings
   (or for a moment when a printer that does not come up is power-cycled). */
void set_power(bool on);

enum class State {
  Off,       // no power
  Starting,  // power on, not (yet) ready: enumerating, switching mode
  Missing,   // power on for 20 s and still not ready
  Ready,
};

State state();

/* The last status the printer gave (refreshed every few seconds while it is on). */
struct Status {
  bool     valid      = false;
  bool     cassette   = false;
  bool     busy       = false;
  bool     charging   = false;
  uint8_t  battery_percent = 0;  // byte 3; always 100 so far, see docs/protocol.md
  uint16_t battery_mv      = 0;  // probably millivolts, see docs/protocol.md
};

Status last_status();

/* Prints a label `copies` times: waits for the printer if it is still starting, then sends
   block by block, each after the printer answered a status request. On failure `error` says why,
   in words for the web interface. */
esp_err_t print(const label::Stream &stream, int copies, std::string &error);

}  // namespace printer

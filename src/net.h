/*
   The network, from the settings (config.h): labelbee's own access point is always on (setup and
   a way in when the home network is gone), and next to it the device joins the configured WiFi,
   by DHCP or with a fixed address. mDNS announces <name>.local, SNTP sets the clock.

   Changed settings take effect after a restart (web.cpp restarts after saving): simpler and more
   reliable than tearing down and rebuilding WiFi, mDNS and SNTP in place.
*/

#pragma once

#include <esp_err.h>

#include <cstdint>
#include <string>

namespace net {

/* Starts WiFi (access point + station), mDNS and SNTP. config::load() must have run. */
void start();

struct Info {
  std::string state;  // "connected", "connecting", "unconfigured"
  std::string ssid;
  std::string ip;
  uint8_t     prefix = 0;
  std::string gateway;
  std::string dns;
  int         rssi = 0;
  std::string bssid;    // the access point joined (with several of the same name)
  int         channel = 0;
  std::string mac;
  std::string ap_ssid;
  std::string ap_ip;
  int         ap_clients = 0;
};

Info info();

/* One line for the console. */
std::string describe();

/* Who set the clock: NTP, the browser (POST /api/time, when there is no NTP), or nobody yet. */
enum class TimeSource { None, Ntp, Browser };
TimeSource time_source();
const char *time_source_name();   // "ntp", "browser", "none"

/* Is the clock set at all? */
bool time_synced();

/* Sets the clock from a browser's (Unix ms), unless NTP has set it. True if taken. */
bool set_time_from_client(int64_t epoch_ms);

/* The WiFi networks in reach as a JSON array [{ssid, rssi, channel, open}], strongest first and one
   entry per name. Blocks for about two seconds. Empty on failure: err is ESP_ERR_INVALID_STATE
   while another scan runs, otherwise the scan's own error. */
std::string scan_json(esp_err_t &err);

}  // namespace net

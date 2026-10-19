/*
   The device's settings, kept in NVS (namespace "cfg"): its name, the WiFi it joins (DHCP or a
   fixed address), its own access point (always on) and the time (NTP server, time zone).

   The web interface reads and writes them as JSON (GET/POST /api/settings); passwords are never
   sent out, only whether one is set.
*/

#pragma once

#include <esp_err.h>

#include <cstdint>
#include <string>

namespace config {

struct Wifi {
  std::string ssid;
  std::string password;
  bool        fixed  = false;  // a fixed address instead of DHCP
  std::string ip;
  uint8_t     prefix = 24;
  std::string gateway;
  std::string dns1;
  std::string dns2;
};

struct Ap {
  std::string ssid     = "labelbee";
  std::string password = "labelbee";  // WPA2; "" = open
};

struct Time {
  std::string ntp = "pool.ntp.org";
  std::string tz  = "CET-1CEST,M3.5.0,M10.5.0/3";  // POSIX TZ: Europe/Berlin
};

struct Config {
  std::string name = "labelbee";  // mDNS name (<name>.local) and DHCP host name
  Wifi        wifi;
  Ap          ap;
  Time        time;
};

/* Reads the settings (defaults where none are stored). Initialises NVS first. */
void load();

const Config &get();

/* Stores new settings in NVS. They take effect after a restart: get() keeps returning the running
   settings, so no task reads a half-changed copy. */
esp_err_t save(const Config &c);

/* Forgets all stored settings (back to the defaults after the next start). */
void reset();

/* The settings as JSON for the web interface, without passwords. */
std::string to_json();

/* Applies a (partial) JSON object to a copy of the current settings and checks it. On success
   `out` holds the new settings; otherwise `error` says what is wrong, in words for the user. */
bool from_json(const std::string &json, Config &out, std::string &error);

}  // namespace config

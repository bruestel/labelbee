#include "config.h"

#include <cJSON.h>
#include <esp_log.h>
#include <lwip/inet.h>
#include <nvs.h>
#include <nvs_flash.h>

#include <cctype>
#include <cstring>
#include <memory>

#include <utility>

namespace config {

static const char *const TAG = "config";
static const char *const NS  = "cfg";

static Config s_config;

static std::string read_str(nvs_handle_t h, const char *key, const std::string &fallback) {
  size_t len = 0;
  if (nvs_get_str(h, key, nullptr, &len) != ESP_OK || len == 0)
    return fallback;
  std::string s(len, '\0');
  nvs_get_str(h, key, s.data(), &len);
  s.resize(len - 1);  // the terminating zero
  return s;
}

void load() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  Config       c;
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READONLY, &h) == ESP_OK) {
    c.name          = read_str(h, "name", c.name);
    c.wifi.ssid     = read_str(h, "ssid", "");
    c.wifi.password = read_str(h, "password", "");
    uint8_t v       = 0;
    if (nvs_get_u8(h, "fixed", &v) == ESP_OK)
      c.wifi.fixed = v;
    if (nvs_get_u8(h, "prefix", &v) == ESP_OK)
      c.wifi.prefix = v;
    c.wifi.ip       = read_str(h, "ip", "");
    c.wifi.gateway  = read_str(h, "gateway", "");
    c.wifi.dns1     = read_str(h, "dns1", "");
    c.wifi.dns2     = read_str(h, "dns2", "");
    c.ap.ssid       = read_str(h, "ap_ssid", c.ap.ssid);
    c.ap.password   = read_str(h, "ap_password", c.ap.password);
    c.time.ntp      = read_str(h, "ntp", c.time.ntp);
    c.time.tz       = read_str(h, "tz", c.time.tz);
    nvs_close(h);
  } else if (nvs_open("net", NVS_READONLY, &h) == ESP_OK) {
    // the test firmware kept only the WiFi credentials, in "net"
    c.wifi.ssid     = read_str(h, "ssid", "");
    c.wifi.password = read_str(h, "password", "");
    nvs_close(h);
  }
  s_config = c;
}

const Config &get() { return s_config; }

esp_err_t save(const Config &c) {
  nvs_handle_t h;
  esp_err_t    err = nvs_open(NS, NVS_READWRITE, &h);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "cannot open the settings: %s", esp_err_to_name(err));
    return err;
  }
  const std::pair<const char *, const std::string *> strings[] = {
      {"name", &c.name},          {"ssid", &c.wifi.ssid},           {"password", &c.wifi.password},
      {"ip", &c.wifi.ip},         {"gateway", &c.wifi.gateway},     {"dns1", &c.wifi.dns1},
      {"dns2", &c.wifi.dns2},     {"ap_ssid", &c.ap.ssid},          {"ap_password", &c.ap.password},
      {"ntp", &c.time.ntp},       {"tz", &c.time.tz},
  };
  for (const auto &[key, value] : strings)
    if (err == ESP_OK)
      err = nvs_set_str(h, key, value->c_str());
  if (err == ESP_OK)
    err = nvs_set_u8(h, "fixed", c.wifi.fixed);
  if (err == ESP_OK)
    err = nvs_set_u8(h, "prefix", c.wifi.prefix);
  if (err == ESP_OK)
    err = nvs_commit(h);
  nvs_close(h);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "cannot save the settings: %s", esp_err_to_name(err));
    return err;
  }
  // Takes effect after the restart that follows (web.cpp); the running tasks keep the old values.
  ESP_LOGI(TAG, "settings saved");
  return ESP_OK;
}

void reset() {
  nvs_handle_t h;
  if (nvs_open(NS, NVS_READWRITE, &h) == ESP_OK) {
    nvs_erase_all(h);
    nvs_commit(h);
    nvs_close(h);
  }
  ESP_LOGW(TAG, "settings reset to the defaults");
}

std::string to_json() {
  const Config &c    = s_config;
  cJSON        *root = cJSON_CreateObject();
  cJSON        *dev  = cJSON_AddObjectToObject(root, "device");
  cJSON_AddStringToObject(dev, "name", c.name.c_str());
  cJSON *wifi = cJSON_AddObjectToObject(root, "wifi");
  cJSON_AddStringToObject(wifi, "ssid", c.wifi.ssid.c_str());
  cJSON_AddBoolToObject(wifi, "hasPassword", !c.wifi.password.empty());
  cJSON_AddStringToObject(wifi, "mode", c.wifi.fixed ? "static" : "dhcp");
  cJSON_AddStringToObject(wifi, "ip", c.wifi.ip.c_str());
  cJSON_AddNumberToObject(wifi, "prefix", c.wifi.prefix);
  cJSON_AddStringToObject(wifi, "gateway", c.wifi.gateway.c_str());
  cJSON_AddStringToObject(wifi, "dns1", c.wifi.dns1.c_str());
  cJSON_AddStringToObject(wifi, "dns2", c.wifi.dns2.c_str());
  cJSON *ap = cJSON_AddObjectToObject(root, "ap");
  cJSON_AddStringToObject(ap, "ssid", c.ap.ssid.c_str());
  cJSON_AddBoolToObject(ap, "hasPassword", !c.ap.password.empty());
  cJSON *time = cJSON_AddObjectToObject(root, "time");
  cJSON_AddStringToObject(time, "ntp", c.time.ntp.c_str());
  cJSON_AddStringToObject(time, "tz", c.time.tz.c_str());

  char       *text = cJSON_PrintUnformatted(root);
  std::string out  = text ? text : "{}";
  cJSON_free(text);
  cJSON_Delete(root);
  return out;
}

/* --- Reading the web interface's JSON -------------------------------------------------------- */

static bool valid_ipv4(const std::string &s) {
  in_addr a;
  return !s.empty() && inet_aton(s.c_str(), &a);
}

/* A host name label: lower-case letters, digits and hyphens, 1-32 characters, not starting or ending with a
   hyphen (the web interface checks the same). */
static bool valid_name(const std::string &s) {
  if (s.empty() || s.size() > 32 || s.front() == '-' || s.back() == '-')
    return false;
  for (char c : s)
    if (!std::islower(static_cast<unsigned char>(c)) && !std::isdigit(static_cast<unsigned char>(c)) && c != '-')
      return false;
  return true;
}

/* Copies a string member if present; false if present but not a string or too long. */
static bool take(const cJSON *obj, const char *key, std::string &dst, size_t max) {
  const cJSON *v = cJSON_GetObjectItemCaseSensitive(obj, key);
  if (!v)
    return true;
  if (!cJSON_IsString(v) || std::strlen(v->valuestring) > max)
    return false;
  dst = v->valuestring;
  return true;
}

bool from_json(const std::string &json, Config &out, std::string &error) {
  std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_ParseWithLength(json.data(), json.size()), cJSON_Delete);
  if (!root || !cJSON_IsObject(root.get())) {
    error = "Invalid request.";
    return false;
  }
  Config c = s_config;

  if (const cJSON *dev = cJSON_GetObjectItemCaseSensitive(root.get(), "device")) {
    if (!take(dev, "name", c.name, 32) || !valid_name(c.name)) {
      error = "Device name: 1 to 32 characters, only lower-case letters, digits and hyphens.";
      return false;
    }
  }
  if (const cJSON *w = cJSON_GetObjectItemCaseSensitive(root.get(), "wifi")) {
    if (!take(w, "ssid", c.wifi.ssid, 32) || !take(w, "password", c.wifi.password, 64)) {
      error = "WiFi name (up to 32) or password (up to 64 characters) too long.";
      return false;
    }
    if (!c.wifi.password.empty() && c.wifi.password.size() < 8) {
      error = "The WiFi password has at least 8 characters.";
      return false;
    }
    std::string mode = c.wifi.fixed ? "static" : "dhcp";
    take(w, "mode", mode, 8);
    c.wifi.fixed = mode == "static";
    if (const cJSON *p = cJSON_GetObjectItemCaseSensitive(w, "prefix"); cJSON_IsNumber(p)) {
      // checked before the cast, so 280 does not wrap to a valid 24
      c.wifi.prefix = p->valuedouble >= 1 && p->valuedouble <= 30 ? static_cast<uint8_t>(p->valuedouble) : 0;
    }
    take(w, "ip", c.wifi.ip, 15);
    take(w, "gateway", c.wifi.gateway, 15);
    take(w, "dns1", c.wifi.dns1, 15);
    take(w, "dns2", c.wifi.dns2, 15);
    if (c.wifi.fixed) {
      if (!valid_ipv4(c.wifi.ip) || c.wifi.prefix < 1 || c.wifi.prefix > 30) {
        error = "Fixed address: give the IP address and a prefix of 1 to 30.";
        return false;
      }
      if (!c.wifi.gateway.empty() && !valid_ipv4(c.wifi.gateway)) {
        error = "The gateway is not an IPv4 address.";
        return false;
      }
      if ((!c.wifi.dns1.empty() && !valid_ipv4(c.wifi.dns1)) || (!c.wifi.dns2.empty() && !valid_ipv4(c.wifi.dns2))) {
        error = "A DNS server is not an IPv4 address.";
        return false;
      }
    }
  }
  if (const cJSON *a = cJSON_GetObjectItemCaseSensitive(root.get(), "ap")) {
    if (!take(a, "ssid", c.ap.ssid, 32) || c.ap.ssid.empty()) {
      error = "The access point name has 1 to 32 characters.";
      return false;
    }
    if (!take(a, "password", c.ap.password, 63) || (!c.ap.password.empty() && c.ap.password.size() < 8)) {
      error = "The access point password is empty (open) or has 8 to 63 characters.";
      return false;
    }
  }
  if (const cJSON *t = cJSON_GetObjectItemCaseSensitive(root.get(), "time")) {
    if (!take(t, "ntp", c.time.ntp, 64) || !take(t, "tz", c.time.tz, 64)) {
      error = "NTP server or time zone too long.";
      return false;
    }
  }
  out = c;
  return true;
}

}  // namespace config

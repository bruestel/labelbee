#include "web.h"

#include "board.h"
#include "config.h"
#include "json.h"
#include "label.h"
#include "logbuf.h"
#include "net.h"
#include "printer.h"
#include "store.h"

#include <esp_app_desc.h>
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>
#include <vector>
#include <memory>
#include <string>

namespace web {

static const char *const TAG = "web";

/* A test label's length: ~42 mm at 180 dpi (the same as the console's). */
static constexpr uint32_t TEST_LABEL_WIDTH = 300;
static constexpr int      MAX_COPIES       = 50;
static constexpr size_t   MAX_TEMPLATE     = 512 * 1024;  // a design with pictures carries them as data URLs

/* The web interface: www/ packed into one blob by tools/pack_www.py (format described there). */
extern "C" const uint8_t www_pack_start[];
extern "C" const uint8_t www_pack_end[];

struct Asset {
  std::string    uri;  // "/app.js"; index.html also as "/", settings.html also as "/settings"
  std::string    type;
  const uint8_t *data;
  size_t         size;
};

static std::vector<Asset> s_assets;
static std::string        s_etag;  // of the whole pack: every file changes with a firmware update anyway

static uint16_t u16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
static uint32_t u32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<uint32_t>(p[3]) << 24; }

static void load_assets() {
  const uint8_t *p   = www_pack_start;
  const size_t   len = www_pack_end - www_pack_start;
  if (len < 9 || std::memcmp(p, "LBW1", 4) != 0) {
    ESP_LOGE(TAG, "no web interface in the firmware");
    return;
  }
  const uint32_t count = u32(p + 4);
  s_etag               = "\"" + std::string(reinterpret_cast<const char *>(p + 9), p[8]) + "\"";
  const uint8_t *e     = p + 9 + p[8];
  for (uint32_t i = 0; i < count; i++) {
    const uint16_t path_len = u16(e), type_len = u16(e + 2);
    const uint32_t offset = u32(e + 4), size = u32(e + 8);
    std::string    path(reinterpret_cast<const char *>(e + 12), path_len);
    std::string    type(reinterpret_cast<const char *>(e + 12 + path_len), type_len);
    e += 12 + path_len + type_len;
    if (offset + size > len)
      break;
    s_assets.push_back({"/" + path, type, p + offset, size});
    if (path == "index.html")
      s_assets.push_back({"/", type, p + offset, size});
    else if (path.size() > 5 && path.compare(path.size() - 5, 5, ".html") == 0)
      s_assets.push_back({"/" + path.substr(0, path.size() - 5), type, p + offset, size});
  }
  ESP_LOGI(TAG, "web interface: %u files, %u KB", static_cast<unsigned>(count), static_cast<unsigned>(len / 1024));
}

static std::string query_param(httpd_req_t *req, const char *key) {
  char query[128];
  char value[24];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK)
    return {};
  if (httpd_query_key_value(query, key, value, sizeof(value)) != ESP_OK)
    return {};
  return value;
}

/* --- Firmware rollback ------------------------------------------------------------------------
   A new image boots on probation (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE). It counts as good once
   the web server runs: at its first answered request, or 30 s after it started. WiFi is no
   criterion, labelbee may run with its own access point only. If the image crashes or restarts
   before, the bootloader starts the previous one again. */

static std::atomic<bool> s_confirmed{false};

static void confirm_firmware(const char *why) {
  if (s_confirmed.exchange(true))
    return;
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &state) == ESP_OK &&
      state == ESP_OTA_IMG_PENDING_VERIFY) {
    esp_ota_mark_app_valid_cancel_rollback();
    ESP_LOGI(TAG, "new firmware confirmed (%s)", why);
  }
}

static esp_err_t send_json(httpd_req_t *req, const std::string &json) {
  confirm_firmware("the web server answered");
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, json.data(), json.size());
}

/* The answer to an action: {"ok": ..., "message": "..."}, shown under the buttons. */
static esp_err_t send_result(httpd_req_t *req, bool ok, const std::string &message) {
  return send_json(req, std::string("{\"ok\":") + (ok ? "true" : "false") + ",\"message\":\"" +
                            json_escape(message) + "\"}");
}

/* An error for the web interface: {"error": "..."} with an HTTP status. */
static esp_err_t send_error(httpd_req_t *req, const char *status, const std::string &message) {
  httpd_resp_set_status(req, status);
  return send_json(req, "{\"error\":\"" + json_escape(message) + "\"}");
}

/* The whole request body, into PSRAM when it is large. Empty on failure. */
static bool read_body(httpd_req_t *req, std::string &out, size_t max) {
  if (req->content_len > max)
    return false;
  out.resize(req->content_len);
  size_t got      = 0;
  int    timeouts = 0;
  while (got < req->content_len) {
    const int n = httpd_req_recv(req, out.data() + got, req->content_len - got);
    if (n == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 6)  // the server's receive timeout is 5 s
      continue;
    if (n <= 0)
      return false;
    timeouts = 0;
    got += n;
  }
  return true;
}

static std::string url_decode(const char *s) {
  std::string out;
  for (; *s; s++) {
    if (*s == '%' && s[1] && s[2]) {
      unsigned v = 0;
      if (std::sscanf(s + 1, "%2x", &v) == 1) {
        out += static_cast<char>(v);
        s += 2;
        continue;
      }
    }
    out += *s == '+' ? ' ' : *s;
  }
  return out;
}

/* --- The web interface ----------------------------------------------------------------------- */

/* Every GET that no API route took: a file of the web interface, revalidated by its ETag. */
static esp_err_t get_asset(httpd_req_t *req) {
  confirm_firmware("the web server answered");
  std::string uri = req->uri;
  if (const size_t q = uri.find('?'); q != std::string::npos)
    uri.erase(q);
  const auto a = std::find_if(s_assets.begin(), s_assets.end(), [&](const Asset &x) { return x.uri == uri; });
  if (a == s_assets.end())
    return send_error(req, "404 Not Found", "Not found.");
  char inm[40] = "";
  if (httpd_req_get_hdr_value_str(req, "If-None-Match", inm, sizeof(inm)) == ESP_OK && s_etag == inm) {
    httpd_resp_set_status(req, "304 Not Modified");
    httpd_resp_set_hdr(req, "ETag", s_etag.c_str());
    return httpd_resp_send(req, nullptr, 0);
  }
  httpd_resp_set_type(req, a->type.c_str());
  httpd_resp_set_hdr(req, "ETag", s_etag.c_str());
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");  // revalidate: cheap with the ETag
  return httpd_resp_send(req, reinterpret_cast<const char *>(a->data), a->size);
}

static const char *state_name(printer::State s) {
  switch (s) {
    case printer::State::Off:
      return "off";
    case printer::State::Starting:
      return "starting";
    case printer::State::Missing:
      return "missing";
    case printer::State::Ready:
      return "ready";
  }
  return "unknown";
}

/* The printer's state for the web interface (part of /api/status and /api/info). */
static std::string printer_json() {
  const printer::State  state = printer::state();
  const printer::Status st    = printer::last_status();
  char                  json[256];
  std::snprintf(json, sizeof(json),
                "{\"model\":\"LabelManager PnP\",\"state\":\"%s\",\"power\":%s,\"statusKnown\":%s,"
                "\"cassette\":%s,\"busy\":%s,\"charging\":%s,\"batteryPercent\":%u,\"batteryMv\":%u}",
                state_name(state), board::printer_power() ? "true" : "false", st.valid ? "true" : "false",
                st.cassette ? "true" : "false", st.busy ? "true" : "false", st.charging ? "true" : "false",
                st.battery_percent, st.battery_mv);
  return json;
}

/* What the designer needs: the tapes, the resolution and the printer's state. */
static esp_err_t get_status(httpd_req_t *req) {
  char head[320];
  std::snprintf(head, sizeof(head),
                "{\"tapes\":[{\"id\":\"6mm\",\"description\":\"D1 6 mm\",\"heightPx\":32},"
                "{\"id\":\"9mm\",\"description\":\"D1 9 mm\",\"heightPx\":48},"
                "{\"id\":\"12mm\",\"description\":\"D1 12 mm\",\"heightPx\":64}],"
                "\"dotsPerMm\":%.4f,\"maxWidthPx\":%lu,\"printer\":",
                180.0 / 25.4, static_cast<unsigned long>(label::MAX_WIDTH));
  return send_json(req, std::string(head) + printer_json() + "}");
}

/* POST /api/print?tape=12mm&copies=1&width=W with the label as the printer's columns (label.h). */
static esp_err_t post_print(httpd_req_t *req) {
  const std::string tape_s = query_param(req, "tape");
  const auto        tape   = label::parse_tape(tape_s.c_str());
  if (!tape)
    return send_error(req, "400 Bad Request", "Unknown tape width \"" + tape_s + "\".");
  const std::string copies_s = query_param(req, "copies");
  const int         copies   = copies_s.empty() ? 1 : std::atoi(copies_s.c_str());
  const uint32_t width  = std::strtoul(query_param(req, "width").c_str(), nullptr, 10);
  if (copies < 1 || copies > MAX_COPIES)
    return send_error(req, "400 Bad Request", "Copies: 1 to " + std::to_string(MAX_COPIES) + ".");
  if (width == 0 || width > label::MAX_WIDTH)
    return send_error(req, "400 Bad Request", "The label is empty or too long.");
  const size_t expected = width * label::bytes_per_line(*tape);
  if (req->content_len != expected)
    return send_error(req, "400 Bad Request", "The print data does not match the width and tape.");

  std::string body;
  if (!read_body(req, body, expected))
    return send_error(req, "400 Bad Request", "The print data is incomplete.");
  const label::Stream stream =
      label::encode_columns(*tape, width, reinterpret_cast<const uint8_t *>(body.data()));
  ESP_LOGI(TAG, "print: %s, %lu dots, %d cop%s", tape_s.c_str(), static_cast<unsigned long>(width), copies,
           copies == 1 ? "y" : "ies");

  std::string     error;
  const esp_err_t err = printer::print(stream, copies, error);
  if (err != ESP_OK)
    return send_error(req, err == ESP_ERR_INVALID_STATE ? "409 Conflict" : "503 Service Unavailable", error);
  return send_json(req, "{\"printed\":" + std::to_string(copies) + "}");
}

static esp_err_t post_power(httpd_req_t *req) {
  printer::set_power(query_param(req, "on") == "1");
  return get_status(req);
}

/* All templates as one JSON object {"name": design, ...}, sent file by file. Each design gets its
   file time as "_mtime" (Unix seconds), so files saved without a date still show one. */
static esp_err_t get_templates(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  bool      first = true;
  esp_err_t err   = httpd_resp_send_chunk(req, "{", 1);
  store::list([&](const std::string &name, const std::string &json, int64_t mtime) {
    std::string head = std::string(first ? "" : ",") + "\"" + json_escape(name) + "\":";
    first            = false;
    if (mtime > 0) {
      head += "{\"_mtime\":" + std::to_string(static_cast<long long>(mtime));
      if (json.find_first_not_of(" \t\r\n", json.find('{') + 1) != json.find_last_of('}'))
        head += ',';
      err = httpd_resp_send_chunk(req, head.data(), head.size());
      if (err == ESP_OK)
        err = httpd_resp_send_chunk(req, json.data() + json.find('{') + 1, json.size() - json.find('{') - 1);
    } else {
      err = httpd_resp_send_chunk(req, head.data(), head.size());
      if (err == ESP_OK)
        err = httpd_resp_send_chunk(req, json.data(), json.size());
    }
    return err == ESP_OK;
  });
  if (err == ESP_OK)
    err = httpd_resp_send_chunk(req, "}", 1);
  if (err == ESP_OK)
    err = httpd_resp_send_chunk(req, nullptr, 0);
  return err;
}

static std::string template_name(httpd_req_t *req) {
  const char *prefix = "/api/templates/";
  std::string uri    = req->uri;
  if (const size_t q = uri.find('?'); q != std::string::npos)
    uri.erase(q);
  return uri.size() > std::strlen(prefix) ? url_decode(uri.c_str() + std::strlen(prefix)) : "";
}

static esp_err_t put_template(httpd_req_t *req) {
  const std::string name = template_name(req);
  if (name.empty() || name.size() > store::MAX_NAME)
    return send_error(req, "400 Bad Request",
                      "The name must have 1 to " + std::to_string(store::MAX_NAME) + " characters.");
  std::string body;
  if (!read_body(req, body, MAX_TEMPLATE) || body.empty() || body.front() != '{')
    return send_error(req, "400 Bad Request", "The label is empty or too large.");
  const esp_err_t err = store::save(name, body);
  if (err == ESP_ERR_INVALID_ARG)
    return send_error(req, "400 Bad Request", "The label data is not valid.");
  if (err != ESP_OK)
    return send_error(req, "500 Internal Server Error", "The label could not be saved.");
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

static esp_err_t delete_template(httpd_req_t *req) {
  if (store::remove(template_name(req)) != ESP_OK)
    return send_error(req, "404 Not Found", "There is no such file.");
  httpd_resp_set_status(req, "204 No Content");
  return httpd_resp_send(req, nullptr, 0);
}

static esp_err_t get_log(httpd_req_t *req) {
  const std::string since = query_param(req, "since");
  uint32_t          next  = 0;
  const std::string text  = logbuf::read(static_cast<uint32_t>(std::strtoul(since.c_str(), nullptr, 10)), &next);
  char              next_s[12];
  std::snprintf(next_s, sizeof(next_s), "%lu", static_cast<unsigned long>(next));
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  httpd_resp_set_hdr(req, "X-Next", next_s);
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, text.data(), text.size());
}

static esp_err_t post_test_print(httpd_req_t *req) {
  const std::string tape_s = query_param(req, "tape");
  const auto        tape   = label::parse_tape(tape_s.empty() ? "12" : tape_s.c_str());
  if (!tape)
    return send_result(req, false, "Unknown tape width \"" + tape_s + "\".");
  ESP_LOGI(TAG, "test label, %s mm", tape_s.c_str());
  std::string     error;
  // optional head dot the image starts at, to find where a narrow tape lies
  const std::string offset_s = query_param(req, "offset");
  const int offset = offset_s.empty() ? -1 : std::clamp(std::atoi(offset_s.c_str()), 0, static_cast<int>(64 - label::height_px(*tape)));
  const bool      ruler = query_param(req, "ruler") == "1";
  const esp_err_t err   = printer::print(ruler ? label::head_ruler() : label::test_pattern(*tape, TEST_LABEL_WIDTH, offset), 1, error);
  return send_result(req, err == ESP_OK, err == ESP_OK ? "Printed." : error);
}

/* Restarts a second after the answer went out. */
static void restart_soon() {
  xTaskCreate(
      [](void *) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
      },
      "restart", 2048, nullptr, 2, nullptr);
}

static esp_err_t get_settings(httpd_req_t *req) { return send_json(req, config::to_json()); }

/* New settings take effect after a restart (net.h); the printer stays on through it (board.h). */
static esp_err_t post_settings(httpd_req_t *req) {
  std::string body, error;
  config::Config c;
  if (!read_body(req, body, 4096))
    return send_error(req, "400 Bad Request", "Invalid request.");
  if (!config::from_json(body, c, error))
    return send_error(req, "400 Bad Request", error);
  if (config::save(c) != ESP_OK)
    return send_error(req, "500 Internal Server Error", "The settings could not be saved.");
  restart_soon();
  return send_result(req, true, "Saved. labelbee restarts and is then at http://" + c.name +
                                    ".local/ (or via the WiFi \"" + c.ap.ssid + "\" at http://192.168.4.1/).");
}

static esp_err_t get_info(httpd_req_t *req) {
  const esp_app_desc_t *app = esp_app_get_description();
  const net::Info       n   = net::info();
  char                  now[24] = "";
  if (net::time_synced()) {
    time_t    t = time(nullptr);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", &tm);
  }
  size_t total = 0, used = 0;
  store::usage(total, used);
  // a firmware the bootloader rolled back from (it stays marked invalid until the slot is rewritten)
  char rejected[40] = "";
  if (const esp_partition_t *bad = esp_ota_get_last_invalid_partition()) {
    esp_app_desc_t desc;
    if (esp_ota_get_partition_description(bad, &desc) == ESP_OK)
      std::snprintf(rejected, sizeof(rejected), "%s", json_escape(desc.version).c_str());
  }
  long long boot_ms = 0;   // when the device started, Unix ms (0 while the clock is not set): log times
  if (net::time_synced()) {
    timeval tv;
    gettimeofday(&tv, nullptr);
    boot_ms = static_cast<long long>(tv.tv_sec) * 1000 + tv.tv_usec / 1000 - esp_timer_get_time() / 1000;
  }
  char json[1200];
  std::snprintf(
      json, sizeof(json),
      "{\"version\":\"%s\",\"built\":\"%s %s\",\"uptimeS\":%lld,\"time\":%s%s%s,\"timeSynced\":%s,\"timeSource\":\"%s\",\"bootEpochMs\":%lld,\"rejectedVersion\":%s%s%s,"
      "\"wifi\":{\"state\":\"%s\",\"ssid\":\"%s\",\"ip\":\"%s\",\"prefix\":%u,\"gateway\":\"%s\",\"dns\":\"%s\","
      "\"rssi\":%d,\"bssid\":\"%s\",\"channel\":%d,\"mac\":\"%s\"},"
      "\"ap\":{\"ssid\":\"%s\",\"ip\":\"%s\",\"clients\":%d},"
      "\"ram\":{\"internalFreeKb\":%u,\"internalMinKb\":%u,\"psramFreeKb\":%u},"
      "\"storage\":{\"usedKb\":%u,\"totalKb\":%u},\"printer\":",
      json_escape(app->version).c_str(), app->date, app->time, esp_timer_get_time() / 1000000, now[0] ? "\"" : "",
      now[0] ? now : "null", now[0] ? "\"" : "", net::time_synced() ? "true" : "false", net::time_source_name(), boot_ms, rejected[0] ? "\"" : "", rejected[0] ? rejected : "null", rejected[0] ? "\"" : "", n.state.c_str(),
      json_escape(n.ssid).c_str(), n.ip.c_str(), n.prefix, n.gateway.c_str(), n.dns.c_str(), n.rssi, n.bssid.c_str(), n.channel, n.mac.c_str(),
      json_escape(n.ap_ssid).c_str(), n.ap_ip.c_str(), n.ap_clients,
      static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
      static_cast<unsigned>(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024),
      static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024), static_cast<unsigned>(used / 1024),
      static_cast<unsigned>(total / 1024));
  return send_json(req, std::string(json) + printer_json() + "}");
}

/* The WiFi networks in reach, for choosing one in the settings. */
static esp_err_t get_scan(httpd_req_t *req) {
  esp_err_t         err;
  const std::string list = net::scan_json(err);
  if (list.empty() && err == ESP_ERR_INVALID_STATE)
    return send_error(req, "409 Conflict", "A scan is already running. Please try again in a moment.");
  if (list.empty())
    return send_error(req, "503 Service Unavailable", std::string("The scan failed: ") + esp_err_to_name(err));
  return send_json(req, list);
}

/* The browser's clock (?ms=<Unix ms>), taken when NTP has not set the time (no internet). */
static esp_err_t post_time(httpd_req_t *req) {
  char query[64], ms[24];
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK ||
      httpd_query_key_value(query, "ms", ms, sizeof(ms)) != ESP_OK)
    return send_error(req, "400 Bad Request", "ms is missing.");
  const bool taken = net::set_time_from_client(std::strtoll(ms, nullptr, 10));
  return send_json(req, std::string("{\"ok\":true,\"taken\":") + (taken ? "true" : "false") + ",\"source\":\"" +
                            net::time_source_name() + "\"}");
}

/* Back to the defaults: settings (and with ?files=1 all stored labels), then a restart. */
static esp_err_t post_factory_reset(httpd_req_t *req) {
  char query[32] = "", files[4] = "0";
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK)
    httpd_query_key_value(query, "files", files, sizeof(files));
  config::reset();
  if (files[0] == '1')
    store::remove_all();
  restart_soon();
  return send_result(req, true, "Reset. labelbee restarts and then only opens its own WiFi \"labelbee\" (password \"labelbee\").");
}

static esp_err_t post_restart(httpd_req_t *req) {
  restart_soon();
  return send_result(req, true, "labelbee is restarting.");
}

/* A new firmware image as the request body (application/octet-stream), written to the other app
   slot and booted:  curl --data-binary @.pio/build/labelbee/firmware.bin http://labelbee.local/api/ota
   No authentication yet: on the home network and labelbee's own access point only. */
static esp_err_t post_ota(httpd_req_t *req) {
  const esp_partition_t *slot = esp_ota_get_next_update_partition(nullptr);
  if (!slot)
    return send_result(req, false, "No update partition found.");
  if (req->content_len == 0 || req->content_len > slot->size)
    return send_result(req, false, "The file is empty or larger than the update partition.");
  esp_ota_handle_t ota = 0;
  // the image's size, so only that much flash is erased up front (OTA_SIZE_UNKNOWN erases all 4 MB first)
  esp_err_t err = esp_ota_begin(slot, req->content_len, &ota);
  if (err != ESP_OK)
    return send_result(req, false, std::string("Update not started: ") + esp_err_to_name(err));
  ESP_LOGI(TAG, "update: %d bytes into %s", req->content_len, slot->label);

  char buf[4096];
  int  left     = req->content_len;
  int  timeouts = 0;
  while (left > 0) {
    const int n = httpd_req_recv(req, buf, std::min(left, static_cast<int>(sizeof(buf))));
    if (n == HTTPD_SOCK_ERR_TIMEOUT && ++timeouts < 6)  // the server's receive timeout is 5 s
      continue;
    if (n <= 0) {
      esp_ota_abort(ota);
      return send_result(req, false, "Transfer aborted.");
    }
    timeouts = 0;
    if ((err = esp_ota_write(ota, buf, n)) != ESP_OK) {
      esp_ota_abort(ota);
      return send_result(req, false, std::string("Writing failed: ") + esp_err_to_name(err));
    }
    left -= n;
  }
  if ((err = esp_ota_end(ota)) != ESP_OK || (err = esp_ota_set_boot_partition(slot)) != ESP_OK)
    return send_result(req, false, std::string("Invalid image: ") + esp_err_to_name(err));

  ESP_LOGI(TAG, "update written to %s, restarting", slot->label);
  send_result(req, true, std::string("Update written (") + slot->label + "), restarting.");
  restart_soon();
  return ESP_OK;
}

/* --- Slow requests on worker tasks ---------------------------------------------------------- */

/* The HTTP server handles one request at a time. A print waits for the printer (seconds), an
   update writes 2 MB of flash, a template write waits for the file system: on the server's own
   task they would hold up every other request, the designer's status polling included. These
   handlers run on worker tasks instead (httpd_req_async_handler_begin), the server goes on. */
static constexpr int    WORKERS      = 2;
static constexpr size_t WORKER_STACK = 10 * 1024;

struct Job {
  httpd_req_t       *req;
  esp_err_t (*handler)(httpd_req_t *);
};

static QueueHandle_t s_jobs;
static SemaphoreHandle_t s_idle;  // counts idle workers

static void worker_task(void *) {
  for (;;) {
    Job job;
    if (xQueueReceive(s_jobs, &job, portMAX_DELAY) != pdTRUE)
      continue;
    job.handler(job.req);
    httpd_req_async_handler_complete(job.req);
    xSemaphoreGive(s_idle);
  }
}

template <esp_err_t (*Handler)(httpd_req_t *)> static esp_err_t on_worker(httpd_req_t *req) {
  // all workers busy: answer at once rather than block the server
  if (xSemaphoreTake(s_idle, 0) != pdTRUE)
    return send_error(req, "503 Service Unavailable", "labelbee is busy, please try again in a moment.");
  httpd_req_t *copy = nullptr;
  if (httpd_req_async_handler_begin(req, &copy) != ESP_OK) {
    xSemaphoreGive(s_idle);
    return Handler(req);
  }
  const Job job = {copy, Handler};
  xQueueSend(s_jobs, &job, portMAX_DELAY);
  return ESP_OK;
}

void start() {
  s_jobs = xQueueCreate(WORKERS, sizeof(Job));
  s_idle = xSemaphoreCreateCounting(WORKERS, WORKERS);
  for (int i = 0; i < WORKERS; i++)
    xTaskCreatePinnedToCore(worker_task, "http_worker", WORKER_STACK, nullptr, 5, nullptr, 1);

  httpd_config_t config   = HTTPD_DEFAULT_CONFIG();
  config.stack_size       = 8192;
  config.max_uri_handlers = 24;
  config.uri_match_fn     = httpd_uri_match_wildcard;  // /api/templates/<name>
  // Browsers keep connections open and open several at once; with the sockets used up the server
  // stops answering. Room for more, and the least recently used one closes when they run out.
  config.max_open_sockets = 12;
  config.lru_purge_enable = true;
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &config) != ESP_OK) {
    ESP_LOGE(TAG, "cannot start the web server");
    return;
  }
  load_assets();
  const esp_timer_create_args_t confirm = {
      .callback = [](void *) { confirm_firmware("the web server has run for 30 s"); },
      .name     = "fw_confirm",
  };
  esp_timer_handle_t timer = nullptr;
  if (esp_timer_create(&confirm, &timer) == ESP_OK)
    esp_timer_start_once(timer, 30 * 1000 * 1000);
  // a browser closing its connections (tab closed, network switched) is no news
  esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
  const httpd_uri_t routes[] = {
      {.uri = "/api/status", .method = HTTP_GET, .handler = get_status},
      {.uri = "/api/print", .method = HTTP_POST, .handler = on_worker<post_print>},
      {.uri = "/api/power", .method = HTTP_POST, .handler = post_power},
      {.uri = "/api/test-print", .method = HTTP_POST, .handler = on_worker<post_test_print>},
      {.uri = "/api/templates", .method = HTTP_GET, .handler = on_worker<get_templates>},
      {.uri = "/api/templates/*", .method = HTTP_PUT, .handler = on_worker<put_template>},
      {.uri = "/api/templates/*", .method = HTTP_DELETE, .handler = on_worker<delete_template>},
      {.uri = "/api/settings", .method = HTTP_GET, .handler = get_settings},
      {.uri = "/api/settings", .method = HTTP_POST, .handler = on_worker<post_settings>},
      {.uri = "/api/info", .method = HTTP_GET, .handler = get_info},
      {.uri = "/api/log", .method = HTTP_GET, .handler = get_log},
      {.uri = "/api/ota", .method = HTTP_POST, .handler = on_worker<post_ota>},
      {.uri = "/api/restart", .method = HTTP_POST, .handler = post_restart},
      {.uri = "/api/scan", .method = HTTP_GET, .handler = on_worker<get_scan>},
      {.uri = "/api/time", .method = HTTP_POST, .handler = post_time},
      {.uri = "/api/factory-reset", .method = HTTP_POST, .handler = on_worker<post_factory_reset>},
      {.uri = "/*", .method = HTTP_GET, .handler = get_asset},  // last: wildcards match in order
  };
  for (const httpd_uri_t &r : routes)
    httpd_register_uri_handler(server, &r);
}

}  // namespace web

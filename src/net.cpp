#include "net.h"

#include "config.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_netif_sntp.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <esp_rrm.h>
#include <esp_wnm.h>
#include <lwip/inet.h>
#include <mdns.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>
#include <vector>

namespace net {

static const char *const TAG = "net";

static constexpr int64_t RETRY_US       = 10 * 1000 * 1000;
static constexpr int64_t FIRST_RETRY_US = 1 * 1000 * 1000;   // right after losing the AP: try another one at once

// Roaming: below ROAM_RSSI, ask the AP for a better one (802.11v) and scan ourselves; switch when
// another AP of the same name is at least ROAM_GAIN dB stronger. Re-armed after ROAM_PAUSE_US.
static constexpr int32_t ROAM_RSSI     = -70;
static constexpr int     ROAM_GAIN     = 8;
static constexpr int64_t ROAM_PAUSE_US = 60 * 1000 * 1000;

static esp_netif_t          *s_sta;
static esp_netif_t          *s_ap;
static esp_timer_handle_t    s_retry;
static esp_timer_handle_t    s_roam_rearm;
static esp_timer_handle_t    s_unpin;
static std::atomic<bool>     s_roam_scan{false};   // a scan is running (roaming or the settings page)
static std::atomic<bool>     s_roaming{false};     // ... and it is the roaming one
static std::atomic<bool>     s_bssid_pinned{false};
static int                   s_failures = 0;
static std::atomic<bool>     s_connected{false};
static std::atomic<TimeSource> s_time{TimeSource::None};

static uint32_t prefix_to_mask(uint8_t prefix) {
  return prefix == 0 ? 0 : htonl(0xFFFFFFFFu << (32 - prefix));
}

static uint8_t mask_to_prefix(uint32_t mask) { return static_cast<uint8_t>(__builtin_popcount(mask)); }

static void set_dns(esp_netif_t *netif, esp_netif_dns_type_t type, const std::string &addr) {
  if (addr.empty())
    return;
  esp_netif_dns_info_t dns = {};
  dns.ip.type              = ESP_IPADDR_TYPE_V4;
  dns.ip.u_addr.ip4.addr   = inet_addr(addr.c_str());
  esp_netif_set_dns_info(netif, type, &dns);
}

/* A fixed address replaces DHCP on the station interface. */
static void apply_fixed_address(const config::Wifi &w) {
  esp_netif_dhcpc_stop(s_sta);
  esp_netif_ip_info_t ip = {};
  ip.ip.addr             = inet_addr(w.ip.c_str());
  ip.netmask.addr        = prefix_to_mask(w.prefix);
  ip.gw.addr             = w.gateway.empty() ? 0 : inet_addr(w.gateway.c_str());
  ESP_ERROR_CHECK(esp_netif_set_ip_info(s_sta, &ip));
  set_dns(s_sta, ESP_NETIF_DNS_MAIN, w.dns1.empty() ? w.gateway : w.dns1);
  set_dns(s_sta, ESP_NETIF_DNS_BACKUP, w.dns2);
  ESP_LOGI(TAG, "fixed address %s/%u, gateway %s", w.ip.c_str(), w.prefix,
           w.gateway.empty() ? "-" : w.gateway.c_str());
}

static void start_mdns(const std::string &name) {
  if (mdns_init() != ESP_OK) {
    ESP_LOGW(TAG, "mDNS unavailable; use the IP address");
    return;
  }
  mdns_hostname_set(name.c_str());
  mdns_instance_name_set("labelbee");
  mdns_service_add(nullptr, "_http", "_tcp", 80, nullptr, 0);
}

static void start_sntp(const config::Time &t) {
  setenv("TZ", t.tz.c_str(), 1);
  tzset();
  if (t.ntp.empty())
    return;
  esp_sntp_config_t sntp = ESP_NETIF_SNTP_DEFAULT_CONFIG(t.ntp.c_str());
  sntp.sync_cb           = [](struct timeval *) {
    if (s_time.exchange(TimeSource::Ntp) != TimeSource::Ntp) {
      char      now[32];
      time_t    t = time(nullptr);
      struct tm tm;
      localtime_r(&t, &tm);
      strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", &tm);
      ESP_LOGI(TAG, "time set by NTP: %s", now);
    }
  };
  esp_netif_sntp_init(&sntp);
}

static std::string bssid_str(const uint8_t *b) {
  char s[18];
  std::snprintf(s, sizeof(s), "%02x:%02x:%02x:%02x:%02x:%02x", b[0], b[1], b[2], b[3], b[4], b[5]);
  return s;
}

/* Watch the signal: WIFI_EVENT_STA_BSS_RSSI_LOW fires once below the threshold, then needs re-arming. */
static void arm_roaming() { esp_wifi_set_rssi_threshold(ROAM_RSSI); }

/* The signal got weak: ask the AP where to go (802.11v) and look for a stronger AP of the same name. */
static void start_roaming(int32_t rssi) {
  const config::Config &c = config::get();
  ESP_LOGI(TAG, "weak signal (%ld dBm): looking for a better access point", static_cast<long>(rssi));
  if (esp_wnm_is_btm_supported_connection())
    esp_wnm_send_bss_transition_mgmt_query(REASON_RSSI, nullptr, 0);   // the AP may answer with a transition request
  wifi_scan_config_t scan = {};
  scan.ssid               = reinterpret_cast<uint8_t *>(const_cast<char *>(c.wifi.ssid.c_str()));
  if (s_roam_scan.exchange(true)) {   // the settings page is scanning: try again later
    esp_timer_start_once(s_roam_rearm, ROAM_PAUSE_US);
  } else if (esp_wifi_scan_start(&scan, false) == ESP_OK) {
    s_roaming = true;
  } else {
    s_roam_scan = false;
    esp_timer_start_once(s_roam_rearm, ROAM_PAUSE_US);
  }
}

/* The roaming scan is done: move to the strongest AP if it is clearly better than the current one. */
static void finish_roaming() {
  s_roam_scan = false;
  s_roaming   = false;
  uint16_t n  = 0;
  esp_wifi_scan_get_ap_num(&n);
  n = std::min<uint16_t>(n, 16);
  wifi_ap_record_t recs[16];
  if (n == 0 || esp_wifi_scan_get_ap_records(&n, recs) != ESP_OK) {
    esp_timer_start_once(s_roam_rearm, ROAM_PAUSE_US);
    return;
  }
  wifi_ap_record_t cur;
  if (esp_wifi_sta_get_ap_info(&cur) != ESP_OK) {
    esp_timer_start_once(s_roam_rearm, ROAM_PAUSE_US);
    return;
  }
  const wifi_ap_record_t *best = nullptr;
  for (uint16_t i = 0; i < n; i++)
    if (std::memcmp(recs[i].bssid, cur.bssid, 6) != 0 && (!best || recs[i].rssi > best->rssi))
      best = &recs[i];
  if (!best || best->rssi < cur.rssi + ROAM_GAIN) {
    if (best)
      ESP_LOGI(TAG, "staying with %s (%d dBm): best other %s has %d dBm", bssid_str(cur.bssid).c_str(), cur.rssi,
               bssid_str(best->bssid).c_str(), best->rssi);
    else
      ESP_LOGI(TAG, "staying with %s (%d dBm): no other access point found", bssid_str(cur.bssid).c_str(), cur.rssi);
    esp_timer_start_once(s_roam_rearm, ROAM_PAUSE_US);
    return;
  }
  ESP_LOGI(TAG, "roaming from %s (%d dBm) to %s (%d dBm, channel %d)", bssid_str(cur.bssid).c_str(), cur.rssi,
           bssid_str(best->bssid).c_str(), best->rssi, best->primary);
  wifi_config_t sta;
  esp_wifi_get_config(WIFI_IF_STA, &sta);
  std::memcpy(sta.sta.bssid, best->bssid, 6);
  sta.sta.bssid_set = true;   // just for this join; cleared again after it (see on_event)
  sta.sta.channel   = best->primary;
  s_bssid_pinned    = true;
  esp_wifi_set_config(WIFI_IF_STA, &sta);
  esp_wifi_connect();
}

/* After a roam the station must not stay pinned to that AP: later joins pick the strongest again.
   Only once the join is complete: while the driver still counts as "connecting" it refuses new
   settings ("sta is connecting, cannot set config"). The supplicant tries the same at the join itself
   when 802.11v is on, and is refused for that reason; this catches up on it. */
static void unpin_bssid() {
  wifi_config_t sta;
  if (esp_wifi_get_config(WIFI_IF_STA, &sta) != ESP_OK)
    return;
  const bool pinned = s_bssid_pinned.exchange(false);
  if (!sta.sta.bssid_set && sta.sta.channel == 0)
    return;
  ESP_LOGI(TAG, "no longer tied to %s / channel %d%s", sta.sta.bssid_set ? bssid_str(sta.sta.bssid).c_str() : "any AP",
           sta.sta.channel, pinned ? " (after roaming)" : "");
  sta.sta.bssid_set = false;
  sta.sta.channel   = 0;
  if (esp_wifi_set_config(WIFI_IF_STA, &sta) != ESP_OK && pinned)
    s_bssid_pinned = true;   // try again after the next join
}

static void on_event(void *, esp_event_base_t base, int32_t id, void *data) {
  const config::Config &c = config::get();
  if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
    const auto *ev = static_cast<wifi_event_sta_disconnected_t *>(data);
    // Lost the AP (it went away, or we roamed): retry soon, the strongest remaining AP is taken.
    const bool was = s_connected.exchange(false);
    // while a roam is under way the driver is joining the new AP itself: give it time
    const int64_t wait = s_bssid_pinned ? RETRY_US / 2 : (was || s_failures == 0) ? FIRST_RETRY_US : RETRY_US;
    s_failures = was ? 0 : s_failures + 1;
    if (was)
      ESP_LOGW(TAG, "disconnected from \"%s\" (reason %d)", c.wifi.ssid.c_str(), ev ? ev->reason : 0);
    else
      ESP_LOGW(TAG, "could not join \"%s\" (reason %d), retrying in %llds", c.wifi.ssid.c_str(), ev ? ev->reason : 0,
               wait / 1000000);
    if (s_failures > 0 && s_bssid_pinned)
      unpin_bssid();   // a pinned AP that does not take us: back to any AP of that name (disconnected: allowed)
    esp_timer_stop(s_retry);
    esp_timer_start_once(s_retry, wait);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
    const auto *ev = static_cast<wifi_event_sta_connected_t *>(data);
    s_failures     = 0;
    esp_timer_stop(s_retry);
    ESP_LOGI(TAG, "joined access point %s on channel %d", ev ? bssid_str(ev->bssid).c_str() : "?", ev ? ev->channel : 0);
    esp_timer_stop(s_unpin);
    esp_timer_start_once(s_unpin, 2 * 1000 * 1000);   // once the join is complete (see unpin_bssid)
    esp_timer_stop(s_roam_rearm);
    arm_roaming();
    if (c.wifi.fixed) {
      s_connected = true;  // no IP_EVENT_STA_GOT_IP without DHCP on every version: count the join
      ESP_LOGI(TAG, "connected to \"%s\": http://%s/ and http://%s.local/", c.wifi.ssid.c_str(), c.wifi.ip.c_str(),
               c.name.c_str());
    }
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_BSS_RSSI_LOW) {
    const auto *ev = static_cast<wifi_event_bss_rssi_low_t *>(data);
    if (!s_roam_scan)
      start_roaming(ev ? ev->rssi : 0);
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_SCAN_DONE) {
    if (s_roam_scan && s_roaming)
      finish_roaming();
  } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
    const auto *ev = static_cast<ip_event_got_ip_t *>(data);
    s_connected    = true;
    ESP_LOGI(TAG, "connected to \"%s\": http://" IPSTR "/ and http://%s.local/", c.wifi.ssid.c_str(),
             IP2STR(&ev->ip_info.ip), c.name.c_str());
    ESP_LOGI(TAG, "access point offers 802.11k: %s, 802.11v: %s", esp_rrm_is_rrm_supported_connection() ? "yes" : "no",
             esp_wnm_is_btm_supported_connection() ? "yes" : "no");
  } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
    ESP_LOGI(TAG, "a device joined the access point \"%s\"", c.ap.ssid.c_str());
  }
}

void start() {
  const config::Config &c = config::get();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  s_sta = esp_netif_create_default_wifi_sta();
  s_ap  = esp_netif_create_default_wifi_ap();
  esp_netif_set_hostname(s_sta, c.name.c_str());  // before DHCP runs, so the router learns the name

  wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&init));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, on_event, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, on_event, nullptr, nullptr));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));  // the settings live in config's namespace
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

  // The access point: always on. Its channel follows the station's once that has joined.
  wifi_config_t ap = {};
  std::memcpy(ap.ap.ssid, c.ap.ssid.data(), std::min(c.ap.ssid.size(), sizeof(ap.ap.ssid)));
  ap.ap.ssid_len = static_cast<uint8_t>(std::min(c.ap.ssid.size(), sizeof(ap.ap.ssid)));
  std::memcpy(ap.ap.password, c.ap.password.data(), std::min(c.ap.password.size(), sizeof(ap.ap.password)));
  ap.ap.channel        = 1;
  ap.ap.authmode       = c.ap.password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
  ap.ap.max_connection = 4;
  ap.ap.pmf_cfg.capable = true;
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));

  // The access point's DHCP offers no router and no DNS server: labelbee does not route, and a
  // laptop that joins it must keep its other connection for everything else (with a default
  // route via 192.168.4.1 it lost the home network, labelbee's own address there included).
  uint8_t offer = 0;
  esp_netif_dhcps_stop(s_ap);
  esp_netif_dhcps_option(s_ap, ESP_NETIF_OP_SET, ESP_NETIF_ROUTER_SOLICITATION_ADDRESS, &offer, sizeof(offer));
  esp_netif_dhcps_start(s_ap);

  const esp_timer_create_args_t retry = {
      .callback = [](void *) { esp_wifi_connect(); },
      .name     = "wifi_retry",
  };
  ESP_ERROR_CHECK(esp_timer_create(&retry, &s_retry));
  const esp_timer_create_args_t rearm = {
      .callback = [](void *) { arm_roaming(); },
      .name     = "wifi_roam",
  };
  ESP_ERROR_CHECK(esp_timer_create(&rearm, &s_roam_rearm));
  const esp_timer_create_args_t unpin = {
      .callback = [](void *) { unpin_bssid(); },
      .name     = "wifi_unpin",
  };
  ESP_ERROR_CHECK(esp_timer_create(&unpin, &s_unpin));

  if (!c.wifi.ssid.empty()) {
    wifi_config_t sta = {};
    std::memcpy(sta.sta.ssid, c.wifi.ssid.data(), std::min(c.wifi.ssid.size(), sizeof(sta.sta.ssid)));
    std::memcpy(sta.sta.password, c.wifi.password.data(),
                std::min(c.wifi.password.size(), sizeof(sta.sta.password)));
    // WPA2 at least when there is a password (WPA3 networks too); an open network without one
    sta.sta.threshold.authmode = c.wifi.password.empty() ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    sta.sta.pmf_cfg.capable    = true;
    sta.sta.sae_pwe_h2e        = WPA3_SAE_PWE_BOTH;
    // With several access points of the same name, join the strongest, not the first one found.
    sta.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    sta.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    // 802.11k/v: neighbour reports and BSS transition requests from the APs (network-assisted roaming)
    sta.sta.rm_enabled  = true;
    sta.sta.btm_enabled = true;
    sta.sta.failure_retry_cnt = 1;   // on a failed join, try the next AP in the list instead of stopping
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    if (c.wifi.fixed)
      apply_fixed_address(c.wifi);
  }

  ESP_ERROR_CHECK(esp_wifi_start());
  // Mains powered: no modem sleep (it delays replies), and the full transmit power (20 dBm).
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_max_tx_power(80);   // in 0.25 dBm
  ESP_LOGI(TAG, "access point \"%s\" (%s) at http://192.168.4.1/", c.ap.ssid.c_str(),
           c.ap.password.empty() ? "open" : "WPA2");
  if (!c.wifi.ssid.empty()) {
    ESP_LOGI(TAG, "joining \"%s\" (%s)", c.wifi.ssid.c_str(), c.wifi.fixed ? "fixed address" : "DHCP");
    esp_wifi_connect();
  } else {
    ESP_LOGW(TAG, "no WiFi configured: join \"%s\" and open http://192.168.4.1/settings", c.ap.ssid.c_str());
  }

  start_mdns(c.name);
  start_sntp(c.time);
}

static std::string ip_str(uint32_t addr) {
  if (!addr)
    return {};
  esp_ip4_addr_t a = {.addr = addr};
  char           s[16];
  esp_ip4addr_ntoa(&a, s, sizeof(s));
  return s;
}

Info info() {
  const config::Config &c = config::get();
  Info                  i;
  i.ssid  = c.wifi.ssid;
  i.state = c.wifi.ssid.empty() ? "unconfigured" : s_connected ? "connected" : "connecting";
  if (s_connected) {
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(s_sta, &ip) == ESP_OK) {
      i.ip      = ip_str(ip.ip.addr);
      i.prefix  = mask_to_prefix(ip.netmask.addr);
      i.gateway = ip_str(ip.gw.addr);
    }
    esp_netif_dns_info_t dns;
    if (esp_netif_get_dns_info(s_sta, ESP_NETIF_DNS_MAIN, &dns) == ESP_OK)
      i.dns = ip_str(dns.ip.u_addr.ip4.addr);
    wifi_ap_record_t ap;
    if (esp_wifi_sta_get_ap_info(&ap) == ESP_OK) {
      i.rssi    = ap.rssi;
      i.channel = ap.primary;
      char b[18];
      std::snprintf(b, sizeof(b), "%02x:%02x:%02x:%02x:%02x:%02x", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3],
                    ap.bssid[4], ap.bssid[5]);
      i.bssid = b;
    }
  }
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    char m[18];
    std::snprintf(m, sizeof(m), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    i.mac = m;
  }
  i.ap_ssid = c.ap.ssid;
  esp_netif_ip_info_t ap_ip;
  if (esp_netif_get_ip_info(s_ap, &ap_ip) == ESP_OK)
    i.ap_ip = ip_str(ap_ip.ip.addr);
  wifi_sta_list_t list;
  if (esp_wifi_ap_get_sta_list(&list) == ESP_OK)
    i.ap_clients = list.num;
  return i;
}

std::string describe() {
  const Info i = info();
  std::string s = "WiFi: " + i.state;
  if (!i.ssid.empty())
    s += " \"" + i.ssid + "\"";
  if (!i.ip.empty())
    s += ", " + i.ip;
  s += "; access point \"" + i.ap_ssid + "\" " + i.ap_ip + " (" + std::to_string(i.ap_clients) + " devices)";
  return s;
}

TimeSource time_source() { return s_time; }

const char *time_source_name() {
  switch (s_time.load()) {
    case TimeSource::Ntp: return "ntp";
    case TimeSource::Browser: return "browser";
    default: return "none";
  }
}

bool time_synced() { return s_time != TimeSource::None; }

bool set_time_from_client(int64_t epoch_ms) {
  if (s_time == TimeSource::Ntp || epoch_ms < 1704067200000LL)   // NTP wins; nothing before 2024
    return false;
  timeval tv = {.tv_sec = static_cast<time_t>(epoch_ms / 1000), .tv_usec = static_cast<suseconds_t>((epoch_ms % 1000) * 1000)};
  settimeofday(&tv, nullptr);
  if (s_time.exchange(TimeSource::Browser) != TimeSource::Browser) {
    char      now[32];
    time_t    t = time(nullptr);
    struct tm tm;
    localtime_r(&t, &tm);
    strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S", &tm);
    ESP_LOGI(TAG, "time set by the browser (no NTP): %s", now);
  }
  return true;
}

std::string scan_json(esp_err_t &err) {
  err = ESP_ERR_INVALID_STATE;
  if (s_roam_scan.exchange(true))
    return "";   // the roaming scan is running
  wifi_scan_config_t scan = {};
  err                     = esp_wifi_scan_start(&scan, true);
  s_roam_scan             = false;
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "scan: %s", esp_err_to_name(err));
    return "";
  }
  uint16_t n = 0;
  esp_wifi_scan_get_ap_num(&n);
  n = std::min<uint16_t>(n, 30);
  wifi_ap_record_t *recs = static_cast<wifi_ap_record_t *>(calloc(n ? n : 1, sizeof(wifi_ap_record_t)));
  if (!recs)
    return "[]";
  esp_wifi_scan_get_ap_records(&n, recs);   // sorted by signal, strongest first
  std::string out = "[";
  std::vector<std::string> seen;
  for (uint16_t i = 0; i < n; i++) {
    std::string ssid(reinterpret_cast<const char *>(recs[i].ssid));
    if (ssid.empty() || std::find(seen.begin(), seen.end(), ssid) != seen.end())
      continue;
    seen.push_back(ssid);
    std::string esc;
    for (char ch : ssid) {
      if (ch == '"' || ch == '\\')
        esc += '\\';
      if (static_cast<unsigned char>(ch) >= 0x20)
        esc += ch;
    }
    char item[160];
    std::snprintf(item, sizeof(item), "%s{\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d,\"open\":%s}", out.size() > 1 ? "," : "",
                  esc.c_str(), recs[i].rssi, recs[i].primary, recs[i].authmode == WIFI_AUTH_OPEN ? "true" : "false");
    out += item;
  }
  free(recs);
  return out + "]";
}

}  // namespace net

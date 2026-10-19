#include "store.h"

#include <cJSON.h>


#include <esp_littlefs.h>
#include <esp_log.h>

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <functional>
#include <mutex>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace store {

static const char *const TAG = "store";

static constexpr const char *BASE = "/data";
static constexpr const char *TEMPLATE_DIR = "/data/templates";

static std::string hex(const std::string &s) {
  static const char digits[] = "0123456789abcdef";
  std::string       out;
  for (unsigned char c : s) {
    out += digits[c >> 4];
    out += digits[c & 0x0F];
  }
  return out;
}

static bool unhex(const std::string &h, std::string &out) {
  if (h.size() % 2)
    return false;
  out.clear();
  for (size_t i = 0; i < h.size(); i += 2) {
    unsigned v = 0;
    if (std::sscanf(h.c_str() + i, "%2x", &v) != 1)
      return false;
    out += static_cast<char>(v);
  }
  return true;
}

static std::string path(const std::string &name) { return std::string(TEMPLATE_DIR) + "/" + hex(name) + ".json"; }

static bool read_file(const std::string &p, std::string &out) {
  FILE *f = std::fopen(p.c_str(), "rb");
  if (!f)
    return false;
  out.clear();
  char   buf[1024];
  size_t n;
  while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
    out.append(buf, n);
  std::fclose(f);
  return true;
}

void start() {
  esp_vfs_littlefs_conf_t conf = {};
  conf.base_path               = BASE;
  conf.partition_label         = "storage";
  conf.format_if_mount_failed  = true;
  esp_err_t err                = esp_vfs_littlefs_register(&conf);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "cannot mount the storage partition: %s", esp_err_to_name(err));
    return;
  }
  mkdir(TEMPLATE_DIR, 0755);
  size_t total = 0, used = 0;
  esp_littlefs_info("storage", &total, &used);
  ESP_LOGI(TAG, "storage: %u of %u KB used", static_cast<unsigned>(used / 1024), static_cast<unsigned>(total / 1024));
}

// One store user at a time: saves, deletes and the listing (the web server has two workers).
static std::mutex s_mutex;

/* A design file looks like a JSON object. Full parsing would need a lot of memory for designs
   with images, so this only rejects what is clearly broken (cut short, not an object). */
static bool looks_like_object(const std::string &json) {
  const size_t a = json.find_first_not_of(" \t\r\n");
  const size_t b = json.find_last_not_of(" \t\r\n");
  return a != std::string::npos && json[a] == '{' && json[b] == '}';
}

void list(const std::function<bool(const std::string &name, const std::string &json, int64_t mtime)> &each) {
  std::lock_guard<std::mutex> lock(s_mutex);
  std::vector<std::pair<std::string, std::string>> items;  // name, file
  if (DIR *d = opendir(TEMPLATE_DIR)) {
    while (dirent *e = readdir(d)) {
      std::string file = e->d_name;
      if (file.size() < 6 || file.compare(file.size() - 5, 5, ".json") != 0)
        continue;
      std::string name;
      if (unhex(file.substr(0, file.size() - 5), name))
        items.emplace_back(name, std::string(TEMPLATE_DIR) + "/" + file);
    }
    closedir(d);
  }
  std::sort(items.begin(), items.end());

  std::string json;
  for (const auto &[name, file] : items) {
    if (!read_file(file, json) || !looks_like_object(json)) {
      ESP_LOGW(TAG, "skipping unreadable template \"%s\"", name.c_str());
      continue;
    }
    struct stat st = {};
    if (!each(name, json, stat(file.c_str(), &st) == 0 ? static_cast<int64_t>(st.st_mtime) : 0))
      break;   // the receiver gave up
  }
}

esp_err_t save(const std::string &name, const std::string &json) {
  if (name.empty() || name.size() > MAX_NAME)
    return ESP_ERR_INVALID_ARG;
  if (!looks_like_object(json))
    return ESP_ERR_INVALID_ARG;
  // Parsed once here (not on every listing): one broken file would break the whole list.
  cJSON *doc = cJSON_ParseWithLength(json.data(), json.size());
  const bool valid = cJSON_IsObject(doc);
  cJSON_Delete(doc);
  if (!valid)
    return ESP_ERR_INVALID_ARG;
  std::lock_guard<std::mutex> lock(s_mutex);
  // Written to a temporary file and renamed, so a power cut never leaves half a template.
  const std::string p   = path(name);
  const std::string tmp = p + ".tmp";
  FILE             *f   = std::fopen(tmp.c_str(), "wb");
  if (!f)
    return ESP_FAIL;
  const bool ok = std::fwrite(json.data(), 1, json.size(), f) == json.size();
  std::fclose(f);
  if (!ok || std::rename(tmp.c_str(), p.c_str()) != 0) {
    std::remove(tmp.c_str());
    ESP_LOGE(TAG, "cannot save template \"%s\" (errno %d)", name.c_str(), errno);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "saved template \"%s\" (%u bytes)", name.c_str(), static_cast<unsigned>(json.size()));
  return ESP_OK;
}

void usage(size_t &total, size_t &used) {
  total = used = 0;
  esp_littlefs_info("storage", &total, &used);
}

esp_err_t remove(const std::string &name) {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (std::remove(path(name).c_str()) != 0)
    return ESP_ERR_NOT_FOUND;
  ESP_LOGI(TAG, "deleted template \"%s\"", name.c_str());
  return ESP_OK;
}

void remove_all() {
  std::lock_guard<std::mutex> lock(s_mutex);
  if (DIR *d = opendir(TEMPLATE_DIR)) {
    while (dirent *e = readdir(d))
      std::remove((std::string(TEMPLATE_DIR) + "/" + e->d_name).c_str());
    closedir(d);
  }
  ESP_LOGW(TAG, "all templates deleted");
}

}  // namespace store

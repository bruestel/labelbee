/*
   The label templates, one file each on the LittleFS partition "storage" (mounted at /data).

   A template is the web interface's design, opaque JSON stored as it arrives. The file name is
   the template's name in hex, so any name works without escaping rules of the file system.
*/

#pragma once

#include <esp_err.h>

#include <cstdint>
#include <functional>
#include <string>

namespace store {

/* Longest template name in bytes (UTF-8): its hex form must fit LittleFS's name length. */
constexpr size_t MAX_NAME = 60;

/* Mounts the partition, formatting it the first time. */
void start();

/* Calls `each` for every stored template, sorted by name, with its JSON and file time (Unix
   seconds, 0 if unknown). One file at a time, so the whole set never has to fit in memory.
   `each` returns false to stop. */
void list(const std::function<bool(const std::string &name, const std::string &json, int64_t mtime)> &each);

/* ESP_ERR_INVALID_ARG for a bad name or JSON that is not an object. */
esp_err_t save(const std::string &name, const std::string &json);

/* Size and use of the file system, in bytes. */
void usage(size_t &total, size_t &used);

/* ESP_ERR_NOT_FOUND if there is no template of that name. */
esp_err_t remove(const std::string &name);

/* Deletes every template. */
void remove_all();

}  // namespace store

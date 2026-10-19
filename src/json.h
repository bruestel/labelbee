/*
   The little JSON the firmware writes itself: escaping a string for use between quotes.
*/

#pragma once

#include <cstdio>
#include <string>

inline std::string json_escape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < 0x20) {
      char esc[8];
      std::snprintf(esc, sizeof(esc), "\\u%04x", c);
      out += esc;
    } else {
      out += c;
    }
  }
  return out;
}

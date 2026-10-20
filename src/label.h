/*
   DYMO LabelManager protocol: turns a 1-bit label into the printer's command stream.

   Derived from the dymo-print Java library (Copyright (C) 2021 piobyte GmbH) and the Python
   project dymoprint (Copyright (c) 2011 Sebastian J. Bronner, now labelle), both licensed under
   the Apache License 2.0; see THIRD-PARTY-NOTICES.md. Details and measurements: docs/protocol.md.
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace label {

enum class Tape {
  D1_6mm,
  D1_9mm,
  D1_12mm,
};

/* "6", "9" or "12" (millimetres), also with "mm" as the web interface sends it. */
std::optional<Tape> parse_tape(const char *s);

/* Bytes per printed column: 4, 6 or 8. */
size_t bytes_per_line(Tape tape);

/* Printable height in dots; a label must have exactly this height. */
inline uint32_t height_px(Tape tape) { return static_cast<uint32_t>(bytes_per_line(tape) * 8); }

/* Upper bound for the label length in dots (~70 cm at 180 dpi). */
constexpr uint32_t MAX_WIDTH = 5000;

/* Is the dot at column x, row y (0 = top) black? */
using Pixels = std::function<bool(uint32_t x, uint32_t y)>;

/* The command stream for one label, with the places where the sender waits for the printer. */
struct Stream {
  std::vector<uint8_t> data;
  /* Offsets into data, ascending, the first one 0: before each block of up to SYNS_PER_BLOCK lines
     the sender asks for the status and waits for the answer (flow control as labelle does it --
     the printer's buffer is small). */
  std::vector<size_t> blocks;
};

constexpr size_t SYNS_PER_BLOCK = 64;

/* The complete command stream for one label of the given width; empty if the width is 0 or above
   MAX_WIDTH. offset: the head dot the image starts at (-1: the tape's own). */
Stream encode(Tape tape, uint32_t width, const Pixels &black, int offset = -1);

/* A label sent as the printer's columns, left to right: bytes_per_line(tape) bytes each, the
   topmost dot the most significant bit of a column's first byte (what the web interface sends). */
Stream encode_columns(Tape tape, uint32_t width, const uint8_t *columns);

/* A test label: a frame, "^TOP" and the head offset, diagonal stripes and a solid block, `width`
   dots long. offset: the head dot the image starts at, to try (-1: the tape's own). */
Stream test_pattern(Tape tape, uint32_t width, int offset = -1);

/* A diagnostic over all 64 dots of the head, whatever tape is in: shows where a narrow tape lies. */
Stream head_ruler();

}  // namespace label

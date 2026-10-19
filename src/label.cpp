#include "label.h"

#include <cstring>

namespace label {

static constexpr uint8_t ESC                = 0x1B;
static constexpr uint8_t SYN                = 0x16;
static constexpr uint8_t CMD_HEIGHT         = 'B';
static constexpr uint8_t CMD_COLOR          = 'C';
static constexpr uint8_t CMD_BYTES_PER_LINE = 'D';

/* Margins: the print head sits ~8.6 mm behind the cutter, so every label starts with that much blank
   tape, the smallest margin there is. No extra blank lines before the label (older implementations
   sent 2, 0.3 mm), and after it just enough feed for the same margin at the end. Measured on a printed test
   label (2026-10-18): with 2 leading and 112 trailing lines, 8.9 mm at the start and 8.0 mm at the
   end; with 0 leading and 116 trailing lines, 8.0 mm first out and 8.5 mm last. 113 trailing lines
   (15.9 mm) give ~8 mm at both ends; the feed itself varies by a few tenths of a millimetre. */
static constexpr size_t LEADING_LINES  = 0;
static constexpr size_t TRAILING_LINES = 113;

std::optional<Tape> parse_tape(const char *s) {
  if (std::strcmp(s, "6") == 0 || std::strcmp(s, "6mm") == 0)
    return Tape::D1_6mm;
  if (std::strcmp(s, "9") == 0 || std::strcmp(s, "9mm") == 0)
    return Tape::D1_9mm;
  if (std::strcmp(s, "12") == 0 || std::strcmp(s, "12mm") == 0)
    return Tape::D1_12mm;
  return std::nullopt;
}

size_t bytes_per_line(Tape tape) {
  switch (tape) {
    case Tape::D1_6mm:
      return 4;
    case Tape::D1_9mm:
      return 6;
    case Tape::D1_12mm:
      return 8;
  }
  return 8;
}

static uint8_t height_code(Tape tape) {
  switch (tape) {
    case Tape::D1_6mm:
      return 2;
    case Tape::D1_9mm:
      return 1;
    case Tape::D1_12mm:
      return 0;
  }
  return 0;
}

/* Starts a line: a block boundary before every SYNS_PER_BLOCK-th one. */
static void syn(Stream &s, size_t &lines) {
  if (lines++ % SYNS_PER_BLOCK == 0)
    s.blocks.push_back(s.data.size());
  s.data.push_back(SYN);
}

static void empty_lines(Stream &s, size_t &lines, size_t count) {
  s.data.insert(s.data.end(), {ESC, CMD_BYTES_PER_LINE, 0});
  for (size_t i = 0; i < count; i++)
    syn(s, lines);
}

/* `column(x, out)` writes column x's bytes. */
template <typename Column> static Stream encode_with(Tape tape, uint32_t width, Column column) {
  Stream s;
  if (width == 0 || width > MAX_WIDTH)
    return s;

  const size_t bpl   = bytes_per_line(tape);
  size_t       lines = 0;
  s.data.reserve(16 + LEADING_LINES + TRAILING_LINES + width * (bpl + 1));

  s.data.insert(s.data.end(), {ESC, CMD_COLOR, 0});
  s.data.insert(s.data.end(), {ESC, CMD_HEIGHT, height_code(tape)});
  if (LEADING_LINES)
    empty_lines(s, lines, LEADING_LINES);

  // The printer receives the label column by column, starting at the right edge.
  s.data.insert(s.data.end(), {ESC, CMD_BYTES_PER_LINE, static_cast<uint8_t>(bpl)});
  for (uint32_t x = width; x-- > 0;) {
    syn(s, lines);
    const size_t at = s.data.size();
    s.data.resize(at + bpl);
    column(x, s.data.data() + at);
  }

  empty_lines(s, lines, TRAILING_LINES);
  s.blocks.front() = 0;  // the header goes with the first block
  return s;
}

Stream encode(Tape tape, uint32_t width, const Pixels &black) {
  const size_t bpl = bytes_per_line(tape);
  return encode_with(tape, width, [&](uint32_t x, uint8_t *out) {
    // the topmost dot is the most significant bit of a column's first byte
    for (size_t byte = 0; byte < bpl; byte++) {
      uint8_t value = 0;
      for (uint32_t bit = 0; bit < 8; bit++)
        if (black(x, static_cast<uint32_t>(byte * 8 + bit)))
          value |= 0x80 >> bit;
      out[byte] = value;
    }
  });
}

Stream encode_columns(Tape tape, uint32_t width, const uint8_t *columns) {
  const size_t bpl = bytes_per_line(tape);
  return encode_with(tape, width, [&](uint32_t x, uint8_t *out) { std::memcpy(out, columns + x * bpl, bpl); });
}

Stream test_pattern(Tape tape, uint32_t width) {
  const uint32_t h = height_px(tape);
  return encode(tape, width, [&](uint32_t x, uint32_t y) {
    if (x < 2 || x >= width - 2 || y < 2 || y >= h - 2)
      return true;                           // frame
    if (x >= width - 2 - h && x < width - 2)
      return true;                           // solid block at the label's end
    return (x + y) % 16 < 4;                 // diagonal stripes
  });
}

}  // namespace label

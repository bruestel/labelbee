#include "label.h"

#include <cstdio>
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


/* The print head has 64 dots. A narrower tape covers only part of it, and not in the middle, so
   the image goes out as all 64 dots of a column, shifted to where the tape is, with ESC B 0.
   Measured with head_ruler() (64-dot columns, ESC B 0): 6 mm tape lies on about the dots 5 to 47;
   test prints with the image starting at dots 9 to 12 were centred best at 11, so its 32 dots start
   at dot 11. 9 mm: not measured yet (the image starts at dot 0). */
static constexpr size_t HEAD_BYTES = 8;

static uint32_t head_offset(Tape tape) {
  switch (tape) {
    case Tape::D1_6mm:
      return 11;
    case Tape::D1_9mm:
      return 0;
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
template <typename Column> static Stream encode_with(Tape tape, uint32_t width, Column column, int offset = -1) {
  Stream s;
  if (width == 0 || width > MAX_WIDTH)
    return s;

  const size_t bpl   = bytes_per_line(tape);
  size_t       lines = 0;
  s.data.reserve(16 + LEADING_LINES + TRAILING_LINES + width * (HEAD_BYTES + 1));

  s.data.insert(s.data.end(), {ESC, CMD_COLOR, 0});
  // ESC B ("dot tab") 0: with 64-dot columns the PnP does apply it and would move the image by that
  // many bytes on top of head_offset.
  s.data.insert(s.data.end(), {ESC, CMD_HEIGHT, 0});
  if (LEADING_LINES)
    empty_lines(s, lines, LEADING_LINES);

  // The printer receives the label column by column, starting at the right edge, as full columns
  // of the head with the tape's image shifted into place (head_offset, or `offset` to try another).
  const uint32_t shift = offset >= 0 ? static_cast<uint32_t>(offset) : head_offset(tape);
  s.data.insert(s.data.end(), {ESC, CMD_BYTES_PER_LINE, static_cast<uint8_t>(HEAD_BYTES)});
  uint8_t image[HEAD_BYTES];
  for (uint32_t x = width; x-- > 0;) {
    syn(s, lines);
    column(x, image);
    uint64_t dots = 0;   // the topmost dot in the most significant bit
    for (size_t i = 0; i < bpl; i++)
      dots |= static_cast<uint64_t>(image[i]) << (56 - 8 * i);
    dots >>= shift;
    for (size_t i = 0; i < HEAD_BYTES; i++)
      s.data.push_back(static_cast<uint8_t>(dots >> (56 - 8 * i)));
  }

  empty_lines(s, lines, TRAILING_LINES);
  s.blocks.front() = 0;  // the header goes with the first block
  return s;
}

Stream encode(Tape tape, uint32_t width, const Pixels &black, int offset) {
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
  }, offset);
}

Stream encode_columns(Tape tape, uint32_t width, const uint8_t *columns) {
  const size_t bpl = bytes_per_line(tape);
  return encode_with(tape, width, [&](uint32_t x, uint8_t *out) { std::memcpy(out, columns + x * bpl, bpl); });
}

// 5x7 glyphs for the test pattern, one byte per row, bit 4 = leftmost dot.
static const uint8_t *glyph(char c) {
  static constexpr uint8_t DIGITS[10][7] = {
      {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},    {14, 17, 1, 2, 4, 8, 31},
      {31, 2, 4, 2, 1, 17, 14},     {2, 6, 10, 18, 31, 2, 2},   {31, 16, 30, 1, 1, 17, 14},
      {6, 8, 16, 30, 17, 17, 14},   {31, 1, 2, 4, 8, 8, 8},     {14, 17, 17, 14, 17, 17, 14},
      {14, 17, 17, 15, 1, 2, 12}};
  static constexpr uint8_t T[7] = {31, 4, 4, 4, 4, 4, 4}, O[7] = {14, 17, 17, 17, 17, 17, 14},
                           P[7] = {30, 17, 17, 30, 16, 16, 16}, UP[7] = {4, 14, 21, 4, 4, 4, 4};
  if (c >= '0' && c <= '9') return DIGITS[c - '0'];
  switch (c) {
    case 'T': return T;
    case 'O': return O;
    case 'P': return P;
    case '^': return UP;
  }
  return nullptr;
}

Stream test_pattern(Tape tape, uint32_t width, int offset) {
  // A frame two dots wide, "^TOP" and the head offset upright at the top edge (readable when the
  // label is held the right way up, so the two side margins can be told apart), diagonal stripes,
  // and a solid block at the end.
  const uint32_t h = height_px(tape);
  const uint32_t scale = h >= 48 ? 3 : 2;
  char text[16];
  std::snprintf(text, sizeof text, "^TOP %u", static_cast<unsigned>(offset >= 0 ? offset : head_offset(tape)));
  const uint32_t text_x = 6, text_y = 4, advance = 6 * scale;
  const uint32_t text_end = text_x + std::strlen(text) * advance + 4;
  return encode(tape, width, [&](uint32_t x, uint32_t y) {
    if (x < 2 || x >= width - 2 || y < 2 || y >= h - 2)
      return true;                           // frame
    if (x >= width - 2 - h && x < width - 2)
      return true;                           // solid block at the label's end
    if (x < text_end) {
      if (x < text_x || y < text_y || y >= text_y + 7 * scale) return false;
      const uint32_t i = (x - text_x) / advance, gx = (x - text_x) % advance / scale;
      const uint8_t *g = glyph(text[i]);
      return g && gx < 5 && ((g[(y - text_y) / scale] >> (4 - gx)) & 1);
    }
    return (x + y) % 16 < 4;                 // diagonal stripes
  }, offset);
}

Stream head_ruler() {
  // Bands of 8 dots, top (dot 0) to bottom (dot 63): band b is a bar of (b + 1) * 14 dots, so the
  // visible bars on a narrow tape tell which dots of the head lie on it.
  constexpr uint32_t STEP = 14, WIDTH = 8 * STEP + 8;
  return encode(Tape::D1_12mm, WIDTH, [&](uint32_t x, uint32_t y) {
    const uint32_t band = y / 8;
    return x >= 4 && x < 4 + (band + 1) * STEP && y % 8 != 7;   // one blank row between the bands
  });
}

}  // namespace label

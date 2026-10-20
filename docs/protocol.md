# DYMO LabelManager PnP: USB protocol

This is the protocol LabelBee uses to drive a DYMO LabelManager PnP, as implemented in
`src/printer.cpp` (USB) and `src/label.cpp` (print data) and as measured on a real printer with
the ESP32-S3 USB host. Values marked "probably" are interpretations of observed data, not
documented facts.

## Credits

The command set comes from earlier open source work:

- [dymoprint](https://github.com/computerlyrik/dymoprint) by computerlyrik (Python), and its
  successor [labelle](https://github.com/labelle-org/labelle), which also showed the flow control
  described below.
- The Java library [dymo-print](https://github.com/piobyte/dymo-print).

## USB identity and modes

The PnP has two modes with different product IDs (vendor `0x0922`, DYMO):

| Mode | ID | When |
|---|---|---|
| mass storage | `0922:1001` | after power-on; presents itself as a small drive |
| printer | `0922:1002` | after the mode switch below |

The device is full speed (12 Mbit/s) and asks for at most 500 mA. The product string is
"DYMO LabelManager PnP". With VBUS switched on it enumerates by itself after about 0.4 s; no key
press is needed.

### Interfaces in mass storage mode (`0922:1001`)

| Interface | Class | Endpoints |
|---|---|---|
| 0 | HID | interrupt OUT `0x01`, interrupt IN `0x81`, 64 bytes each; report descriptor 34 bytes |
| 1 | mass storage | bulk OUT `0x02`, bulk IN `0x82` |

### Interfaces in printer mode (`0922:1002`)

| Interface | Class | Endpoints |
|---|---|---|
| 0 | printer | bulk OUT `0x05`, bulk IN `0x85` |
| 1 | mass storage | |
| 2 | HID | interrupt OUT `0x01`, interrupt IN `0x81` |

HID report descriptor in printer mode: vendor-defined usage page, 64-byte output report, 8-byte
input report, no report IDs. LabelBee uses only the HID interface: all commands and print data go
to interrupt OUT `0x01`, status replies come from interrupt IN `0x81`. (If a device had no
interrupt OUT endpoint, the firmware would fall back to `SET_REPORT` on the control endpoint; the
PnP does not need that.)

## Mode switch

In mass storage mode, send the three bytes `1B 5A 01` (`ESC Z 1`) to the HID interrupt OUT
endpoint `0x01` of interface 0. Sent to the bulk endpoint `0x02` the command is ignored. The
printer then disconnects and comes back as `0922:1002` after about 5.5 s. On Linux,
usb_modeswitch does the same with `MessageEndpoint=0x01`.

For a few seconds after the switch the printer does not answer status requests.

## Print data

All bytes are sent as HID output reports of up to 64 bytes (report ID 0, so no ID byte is
prepended); the last report of a stream may be shorter.

Constants: `ESC = 0x1B`, `SYN = 0x16`, `'A' = 0x41`, `'B' = 0x42`, `'C' = 0x43`, `'D' = 0x44`.

### Tape geometry

Resolution is 180 dpi (about 7.09 dots per mm). The print head has 64 dots (about 9 mm). A narrower
tape covers only part of the head, and **not in its middle**:

| Tape | Image height (dots) | Head dots on the tape | Image placed at head dots |
|---|---|---|---|
| 12 mm | 64 | all | 0 to 63 |
| 9 mm | 48 | not measured yet | 0 to 47 |
| 6 mm | 32 | about 5 to 47 (measured) | 11 to 42 (centred on test prints) |

The reference implementations send `ESC B h` ("dot tab", h = 2 for 6 mm, 1 for 9 mm) and fewer
bytes per column (`ESC D 4` or `ESC D 6`). On the PnP that prints a 6 mm label off centre, right at
one edge of the tape, whatever h is (h = 0 and h = 2 came out identical). With 64-dot columns, in
turn, `ESC B` does move the image (h = 2 by 16 dots). LabelBee therefore always sends all 64 dots
of a column (`ESC D 8`) with `ESC B 0`, and shifts the tape's image itself to the dots in the table. The measurement: a test print over the whole
head (`POST /api/test-print?tape=6mm&ruler=1`) with a bar per 8 dots, each bar longer than the one
before, shows which dots land on the tape. The exact start then comes from test prints with
`&offset=N` (the pattern shows "^TOP" and N upright at the image's top edge): for 6 mm the margins
were equal at 11.

LabelBee limits a label to 5000 dots (about 70 cm).

### Command stream

1. `ESC C 0`: colour (always 0).
2. `ESC B 0`: no dot tab; the image is placed within the 64 dots instead.
3. `ESC D 8`: 8 bytes (64 dots) per column.
4. For each image column, **from right to left** (the printer receives the end of the label
   first): `SYN` followed by 8 bytes, the tape's image shifted to its head dots (table above). The
   topmost dot is the most significant bit of the first byte; a set bit prints black.
5. `ESC D 0` followed by 113 × `SYN`: blank lines that feed the label out to the cutter.

A blank line is a `SYN` while the bytes per column are set to 0 (`ESC D 0`).

### Margins

The print head sits about 8.6 mm behind the cutter, so every label starts with that much blank
tape; this is the smallest possible margin. LabelBee therefore sends no blank lines before the
image (older implementations sent `ESC D 0` + 2 × `SYN`, which adds about 0.3 mm). After the
image it sends 113 blank lines (about 15.9 mm), which gives about 8 mm of margin at both ends.
Measured on printed labels: 2 leading and 112 trailing lines gave 8.9 mm at the start and 8.0 mm
at the end; 0 leading and 116 trailing lines gave 8.0 mm and 8.5 mm. The feed itself varies by a
few tenths of a millimetre.

## Status (`ESC A`)

Sending `ESC A` (`1B 41`) makes the printer answer with an 8-byte input report on interrupt IN
`0x81`. Observed over several cassettes, power cycles, printing and charging:

| Byte | Meaning |
|---|---|
| 0, bit 6 (`0x40`) | **cassette inserted** (certain) |
| 0, bit 5 (`0x20`) | **printing** (certain; `0x60` instead of `0x40` while printing) |
| 1 | always 0 |
| 2, bit 4 (`0x10`) | **probably charging**: set after powering on with a flat battery and after printing, clears at full voltage, toggles near the charge threshold. It is not the tape type; all cassettes showed `0x02` here. |
| 3 | always `0x64` (100); possibly the charge level in percent, never seen to change |
| 4 and 5 | big-endian, **probably battery voltage in mV**: about 3520 at rest, drops briefly to about 3490 when a cassette is changed and recovers over 30 to 60 s, rises to about 3528 while charging |
| 6, 7 | always 0 |

The cassette and printing bits match the status byte of the DYMO LabelWriter 450 Duo technical
reference, which also defines bit 4 as cutter jam and bit 2 as general error.

The tape width is apparently not reported: different 12 mm cassettes all gave the same status.

Timing: right after the mode switch the printer stays silent for a few seconds, and while
printing it answers late (more than 1 s). Use generous timeouts. LabelBee waits 1 s for a normal
status reply and up to 10 s for one during printing.

## Flow control

The printer's buffer is small. Before every block of 64 lines (`SYN`s), send `ESC A` and wait for
the reply; the printer answers once it has room for the next block. LabelBee splits the stream
at every 64th `SYN` (the header goes with the first block) and queries the status before each
block, as labelle does. Sending without pauses works for short labels (300 columns printed fine)
but is not reliable for long ones.

Before printing, LabelBee waits for the printer to be ready and for a first status reply, and
refuses to print if no cassette is inserted.

## Not yet verified

- Where 9 mm tape lies on the head: not measured yet (print the head ruler on 9 mm tape).
- The PnP's current draw while printing and while charging has not been measured.

# Case

Base and lid for the ESP32-S3, the relay and the two USB breakouts, glued with its floor onto the dark
side face of the DYMO LabelManager PnP. Everything is in `labelbee-case.scad` (dimensions measured
2026-10-18). Photos of the assembled case: see the main README.

| File | Content |
|---|---|
| `labelbee-case.scad` | model, all dimensions as variables |
| `base.stl`, `lid.stl` | exported; regenerate after changing the model (commands below) |
| `lid-inlay.stl` | white inlay for multi-filament printers, fits exactly into `lid.stl` |
| `preview.png` | base with parts, lid above it |
| `lid-face.png` | lid from the outside in the two print colours |

```sh
openscad -D 'PART="base"' -o base.stl labelbee-case.scad
openscad -D 'PART="lid"'  -o lid.stl  labelbee-case.scad
openscad -D 'PART="lid_inlay"' -o lid-inlay.stl labelbee-case.scad
openscad -D 'PART="face"' --camera=50,0,0,0,0,0,260 --projection=ortho --imgsize=800,800 -o lid-face.png labelbee-case.scad
```

The lid needs the font **Barlow Condensed Bold** (Google Fonts, OFL) installed on the computer that exports it.

## Lid

- Logo: 8-bit bee above "label" + "bee" in a box, parallel to the bottom edge, recessed 1.2 mm.
- Ventilation: groups of honeycomb holes (1.6 mm) forming a flight path from the bottom left to the bee's
  sting. Under each group sits a baffle plate (2 mm lower, open on two sides) so you cannot see inside.
- Two ribs under the lid hold the ESP at its short ends (module shield at the top, USB-C sockets at the
  bottom), with 0.5 mm clearance (`presser` in `lid()`).
- Screws: 4× M2 × 10 countersunk, self-tapping, recessed. The rim under the lid is cut away at the screw
  posts (0.5 mm clearance), otherwise it hits them at the narrow end.

### Printing in two colours (one filament change)

1. The lid lies as exported, outside face down. Layer height 0.2 mm.
2. Layers 1 to 6 (up to 1.2 mm) in **matte black**.
3. Insert a pause or M600 at 1.2 mm (before layer 7), continue in **white**.

The recesses then show the white: bee, "label" and the box in white, "bee" inside the box in black.
The baffle plates are bridged freely (up to about 9 mm), without supports.

### With multiple filaments (AMS, MMU, IDEX)

Load `lid.stl` and `lid-inlay.stl` together as **one object with two parts** (for example Bambu/Orca:
open both files at once and confirm "load as a single object"; PrusaSlicer: load `lid.stl`, then
"Add part" → `lid-inlay.stl`). The positions already match, do not move anything. Lid in black,
inlay in white: the logo then sits flush in the smooth surface.

## Base

- Screws: relay 4× M2 × 6, each breakout 2× M2 × 6 (self-tapping, 1.6 mm pilot holes).
- Gluing: degrease the surface with isopropyl alcohol, use thin double-sided tape (for example VHB).

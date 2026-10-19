// labelbee: case for the ESP32-S3 board, the relay module and the two USB breakouts, glued with
// its floor onto the dark panel on the side of the DYMO LabelManager PnP.
//
// The outline is that panel, taken from a 200 dpi scan (the user marked the corners): a symmetric
// trapezoid with rounded corners and slightly bulging edges: the long edge at the printer's front,
// the short edge at its back, where both USB sockets are (bottom: to the printer, top: power in).
// The scan lay 3.5 deg askew; against the tilted axis, top and bottom edges are exactly +-8.1 deg.
//
// Top view (X right, Y up; origin = middle of the long left edge), floor glued to the printer:
//
//        ___________________________
//       /                           ''''--.._
//      |  relay   |       | ESP |              |== USB-B  (power in)
//      | (upright)|       |     |              |
//      |          |       |_____|              |== USB-A  (to the printer)
//       \___________________________ ...--''
//
// Every size is a variable below; values marked "measure!" are guesses, check them with a caliper.
//   openscad -D 'PART="base"' -o base.stl labelbee-case.scad
//   openscad -D 'PART="lid"'  -o lid.stl  labelbee-case.scad
//   openscad -D 'PART="lid_inlay"' -o lid-inlay.stl labelbee-case.scad   (second filament, same position)

PART = "preview";  // "base", "lid", "preview", "inside" (base with the parts, no lid)

/* [Outline: the printer's side panel] */
panel_w   = 100.7; // long axis: front (left) to back (right) edge
panel_hl  = 106.5; // height of the left (front) edge
panel_hr  = 77.8;  // height of the right (back) edge
panel_rl  = 6.7;   // corner radius at the left edge
panel_rr  = 11.4;  // corner radius at the right edge
bulge_lr  = 1.0;   // the left and right edges bulge out this much in their middle
bulge_tb  = 1.5;   // the top and bottom edges
inset     = 0.5;   // the case is this much smaller all round, so it sits inside the panel's border

/* [Case] */
wall      = 2.4;
floor_t   = 2.0;   // the glued face
lid_t     = 2.0;
case_h    = 30;    // total height (thickness off the printer's side) incl. lid
fit       = 0.3;
screw_d   = 1.6;   // pilot holes for the lid's self-tapping screws: M2 x 10 (Messbogen 5a)
screw_hole = 2.4;  // through the lid
screw_head = 4.2;  // countersink diameter (M2 head ~3.8)
post      = 7;
post_clear = 0.5;  // the lid's rim keeps this far from the posts

/* [ESP32-S3-N16R8: upright, USB-C end down, rests on ledges at its short ends, held by the lid] */
// Measured 2026-10-18 (Messbogen 1a-1m). USB-C end down: the pins wired to the relay (5Vin, GND,
// 3V3, 4) face left, GND/19/20 (to the printer socket) face right, near the bottom. The module's
// antenna sticks out at the upper end; nothing metal near it.
esp_l     = 57;    // PCB length (1a 63 incl. antenna - 1l 6)
esp_w     = 27.9;  // 1b
esp_usbc  = 0.3;   // 1c: the USB-C sockets stand out this much at the lower end
esp_pcb   = 1.3;   // 1d
esp_parts = 3.1;   // 1e: tallest part above the PCB
esp_wires = 4;     // 1f: the soldered wires above the PCB
esp_under = 1.1;   // 1g: solder joints below the PCB
esp_ant   = 6;     // 1l: the antenna stands out this much at the upper end
esp_ant_w = 18;    // 1m
esp_rows  = 25;    // 1j: pin rows, centre to centre (first pin 1.5 mm from the ends: 1h, 1i)
esp_lift  = 3;     // board underside above the floor (> esp_under)
esp_top   = max(esp_parts, esp_wires) + 2;   // room above the PCB
esp_ledge_w = esp_rows - 4;   // the ledges sit between the pin rows (underside free there: 1k)
esp_x     = 30;    // left edge of the board from the case's left outside

/* [Relay module: upright along the left edge, terminals at top and bottom] */
// Measured 2026-10-18 (Messbogen 2a-2l).
rel_l     = 70;    // 2a (terminals flush: 2b)
rel_w     = 17;    // 2c
rel_h     = 16.9;  // 2d: PCB underside to the relay's top
rel_x     = 5;     // from the case's left outside
rel_hole_dx = 65.8;  // 2g: hole spacing along the length
rel_hole_dy = 12.8;  // 2h: across (holes 2 mm from the edges: 2i, 2j)
rel_hole_d  = 1.6;   // the holes are 2.1 mm (2f): M2 x 6 self-tapping screws (5c), 1.6 mm pilot holes
rel_stand   = 4;     // standoff height; solder joints below the PCB: 2.4 mm (2k)
rel_wires   = 12;    // 2l: the wires stand out this much at both ends before they bend

/* [USB breakouts: flat on standoffs, screwed, socket through the right edge] */
// One board as a vector: [pcb_l, pcb_w, pcb_t, socket overhang beyond the PCB's right edge,
// socket width, socket height above the PCB, socket front side from the PCB's front edge,
// hole diameter, holes [[from the right edge, from the front edge], ...], below the PCB, above]
// "Front" = the lower edge in the case (towards -Y). The socket's face ends flush with the outside.
// USB-A (to the printer), measured 2026-10-18 (Messbogen 3a-3o):
usba = [17, 17, 1.5, 5.6, 14.3, 6.8, 1.6, 3.0, [[11, 2], [11, 15]], 2.5, 20];
usba_y = -14;      // centre of the PCB (lower)
// USB-B (power in), measured 2026-10-18 (Messbogen 4a-4o):
usbb = [20, 20, 1.3, 7, 12, 10.6, 4.2, 3.0, [[12.5, 2.5], [12.5, 17.5]], 2, 3];
usbb_y = 17;       // centre of the PCB (upper)
bo_lift   = 3.5;   // standoff height: above the pins below the PCB (2.5 mm)
bo_hole_d = 1.6;   // pilot holes for M2 x 6 self-tapping screws (5b; board holes 3 mm)
sock_fit  = 0.4;   // play around the sockets in the wall

/* [Vents: small groups of honeycomb cells along a flight path to the bee] */
// Each group: [centre, cells (8: rows 3-2-3, 7: a flower, 3, 2, 1), angle]. They start bottom left,
// run up the front edge and turn towards the bee's sting, getting smaller. Under each a baffle.
vent_hex   = 1.6;  // across flats
vent_web   = 1.0;  // between the cells
vent_groups = [
  [[11, -34], 8, 90], [[11, -15], 7, 0], [[14, 2], 7, 0], [[21, 11], 3, 20],
  [[27, 13], 2, 0], [[32, 13], 1, 0], [[37, 12], 1, 0],
];
baffle_gap = 2;    // air gap between the lid's underside and the baffle (relay top: 5.1 mm below)
baffle_t   = 1.0;
baffle_m   = 1.5;  // the baffle overlaps its group by this much

/* [Lid artwork: recessed, shows the second colour after a filament change] */
// Print the lid face down: the first art_depth (6 layers at 0.2 mm) in the background colour
// (black), then pause / M600 and the rest in the logo colour (white). The recesses show it.
// Needs the font "Barlow Condensed" Bold installed (fonts.google.com, OFL).
art_depth = 1.2;   // 0.6 was too thin: single white pixels stayed on the print bed
art_font  = "Barlow Condensed:style=Bold";
art_angle = atan(((panel_hl - panel_hr) / 2) / (panel_w - 2 * inset));   // parallel to the bottom edge
art_at    = [60, -1];  // centre of the artwork
art_px    = 2.0;       // one dot of the 8-bit bee

$fn = 48;
eps = 0.01;

W   = panel_w - 2 * inset;
HL  = panel_hl - 2 * inset;
HR  = panel_hr - 2 * inset;
slope = (HL - HR) / 2 / W;                 // the top edge falls this much per mm to the right
function half_h(x) = HL / 2 - slope * x;   // outside half height at x
base_h  = case_h - lid_t;
inner_z = base_h - floor_t;

// The outline, shrunk by `shrink` (for the inside): the hull of the four corner circles and of
// points along each edge that bulge out (parabolic, `bulge` in the edge's middle).
function unit(v) = v / norm(v);
module outline(shrink = 0) {
  rl = panel_rl; rr = panel_rr;
  c = [[rl, half_h(rl) - rl], [W - rr, half_h(W - rr) - rr],      // top left, top right
       [W - rr, -(half_h(W - rr) - rr)], [rl, -(half_h(rl) - rl)]]; // bottom right, bottom left
  r = [rl, rr, rr, rl];
  n = [unit([slope, 1]), [1, 0], unit([slope, -1]), [-1, 0]];      // top, right, bottom, left
  b = [bulge_tb, bulge_lr, bulge_tb, bulge_lr];
  steps = 24;
  edge_pts = [for (e = [0 : 3], i = [1 : steps - 1])
    let(a = c[e] + n[e] * (r[e] - shrink), z = c[(e + 1) % 4] + n[e] * (r[(e + 1) % 4] - shrink),
        t = i / steps)
      a + (z - a) * t + n[e] * b[e] * 4 * t * (1 - t)];
  hull() {
    for (i = [0 : 3]) translate(c[i]) circle(r = max(r[i] - shrink, 0.3));
    polygon(edge_pts);
  }
}

module shell() {
  difference() {
    linear_extrude(base_h) outline();
    translate([0, 0, floor_t]) linear_extrude(inner_z + eps) outline(wall);
  }
}

// screw posts for the lid, near the four corners but clear of the parts
post_xy = [for (x = [26, W - 14], s = [-1, 1]) [x, s * (half_h(x) - wall - post / 2 - 0.5)]];

module posts() {
  for (p = post_xy)
    translate([p[0], p[1], 0])
      difference() {
        translate([-post / 2, -post / 2, 0]) cube([post, post, base_h]);
        translate([0, 0, base_h - 12]) cylinder(d = screw_d, h = 12 + eps);
      }
}

module esp_supports() {
  for (s = [-1, 1])   // ledges under the short ends, between the pin rows
    translate([esp_x + (esp_w - esp_ledge_w) / 2, s > 0 ? esp_l / 2 - 3 : -esp_l / 2, 0])
      cube([esp_ledge_w, 3, floor_t + esp_lift]);
  for (x = [esp_x - 2, esp_x + esp_w])   // side stops, below the PCB, clear of the pin headers
    translate([x, -5, 0]) cube([2, 10, floor_t + esp_lift + esp_pcb]);
}

module relay_standoffs() {
  translate([rel_x + rel_w / 2, 0, 0])
    for (sx = [-1, 1], sy = [-1, 1])
      translate([sx * rel_hole_dy / 2, sy * rel_hole_dx / 2, 0])
        difference() {
          cylinder(d = 5.5, h = floor_t + rel_stand);
          translate([0, 0, 0.6]) cylinder(d = rel_hole_d, h = floor_t + rel_stand);   // into the floor: M2 x 6
        }
}

// a board's PCB: right edge so far in that the socket's face ends flush with the outside
function bo_x(b) = W - b[3] - b[0];          // left edge of the PCB
function bo_front(b, y) = y - b[1] / 2;      // front (lower) edge of the PCB

module breakout_mount(b, y) {
  for (h = b[8])
    translate([bo_x(b) + b[0] - h[0], bo_front(b, y) + h[1], 0])
      difference() {
        cylinder(d = 5.5, h = floor_t + bo_lift);
        translate([0, 0, 0.6]) cylinder(d = bo_hole_d, h = floor_t + bo_lift);   // into the floor: M2 x 6
      }
}

// the sockets through the right wall (cut deep enough for its bulge); the socket sits on its PCB
module socket_cut(b, y) {
  z = floor_t + bo_lift + b[2];
  translate([bo_x(b) + b[0], bo_front(b, y) + b[6] - sock_fit, z - sock_fit])
    cube([b[3] + bulge_lr + 1, b[4] + 2 * sock_fit, b[5] + 2 * sock_fit]);
}

module cutouts() {
  socket_cut(usba, usba_y);
  socket_cut(usbb, usbb_y);
}

module base() {
  difference() {
    union() {
      shell();
      intersection() {   // everything inside stays within the walls
        linear_extrude(base_h) outline(wall - eps);
        union() {
          posts();
          esp_supports();
          relay_standoffs();
          breakout_mount(usba, usba_y);
          breakout_mount(usbb, usbb_y);
        }
      }
    }
    cutouts();
  }
}

module presser(x, y, lx, ly, top) {
  h = inner_z - top - 0.5;
  if (h > 0.5) translate([x, y, -h]) cube([lx, ly, h]);
}

bee_rows = [
  "..........###....",
  ".....###.#...#...",
  "....#...##...#...",
  "....#...##...#...",
  "....##########...",
  "...#..##.##...#..",
  "..#...##.##....#.",
  "..#...##.##....#.",
  ".##...##.##.##.#.",
  "###...##.##.##.#.",
  ".##...##.##....#.",
  "..#...##.##....#.",
  "..#...##.##....#.",
  "...#..##.##...#..",
  "....##########...",
];

// the logo: the 8-bit bee (logo/labelbee-bee-1bit.svg) over the wordmark "label" + "bee" in a box
module art() {
  translate(art_at) rotate(art_angle) {
    translate([-17 * art_px / 2, 3]) for (y = [0 : len(bee_rows) - 1], x = [0 : 16])
      if (bee_rows[y][x] == "#") translate([x * art_px, (len(bee_rows) - 1 - y) * art_px]) square(art_px + 0.01);
    translate([1, -22]) text("label", size = 13, font = art_font, halign = "right");
    difference() {
      translate([3, -25]) offset(r = 1.5) square([26.5, 16.5]);
      translate([5.3, -22]) text("bee", size = 13, font = art_font);
    }
  }
}

vent_pitch = vent_hex + vent_web;
vent_r     = vent_hex / sqrt(3);                      // corner radius (pointy top)
function vent_group_cells(n) =
  let(p = vent_pitch, h = p * sqrt(3) / 2)
  n == 8 ? [for (x = [-1, 0, 1]) [x * p, h], for (x = [-0.5, 0.5]) [x * p, 0], for (x = [-1, 0, 1]) [x * p, -h]] :
  n == 7 ? concat([[0, 0]], [for (a = [0 : 60 : 300]) p * [cos(a), sin(a)]]) :
  n == 3 ? [[-p / 2, -h / 3], [p / 2, -h / 3], [0, 2 * h / 3]] :
  n == 2 ? [[-p / 2, 0], [p / 2, 0]] : [[0, 0]];
// half the extent of a group, for its baffle
function vent_group_half(n) = n >= 7 ? 1.5 * vent_pitch : n >= 2 ? vent_pitch : vent_pitch / 2;

module vent_cells() {
  for (g = vent_groups) translate(g[0]) rotate(g[2]) for (c = vent_group_cells(g[1]))
    translate(c) rotate(30) circle(r = vent_r, $fn = 6);
}

module lid_holes() { vent_cells(); for (p = post_xy) translate(p) circle(d = screw_head); }   // for the face preview

// under each group: a plate on two ribs, open at the other two sides
module baffle() {
  for (g = vent_groups) translate(g[0]) rotate(g[2]) {
    a = vent_group_half(g[1]) + baffle_m;
    translate([-a, -a, -baffle_gap - baffle_t]) cube([2 * a, 2 * a, baffle_t]);
    for (y = [-a, a - 1.2]) translate([-a, y, -baffle_gap - baffle_t]) cube([2 * a, 1.2, baffle_gap + baffle_t]);
  }
}

module lid() {
  difference() {
    union() {
      linear_extrude(lid_t) outline();
      // the rim that locates the lid inside the walls, left out at the screw posts (they reach the top)
      translate([0, 0, -2]) linear_extrude(2) difference() {
        outline(wall + fit); outline(wall + fit + 1.6);
        for (p = post_xy) translate(p) offset(delta = post_clear) square(post, center = true);
      }
      baffle();
      for (s = [-1, 1])   // hold the ESP down at both ends
        presser(esp_x + (esp_w - esp_ledge_w) / 2, s > 0 ? esp_l / 2 - 3 : -esp_l / 2, esp_ledge_w, 3,
                esp_lift + esp_pcb + esp_parts);   // on the module (top) and the USB-C sockets (bottom)
    }
    for (p = post_xy)
      translate([p[0], p[1], -3]) {
        cylinder(d = screw_hole, h = lid_t + 4);
        translate([0, 0, 3 + lid_t - 1.2]) cylinder(d1 = screw_hole, d2 = screw_head, h = 1.2 + eps);
      }
    translate([0, 0, -eps]) linear_extrude(lid_t + 2 * eps) vent_cells();
    translate([0, 0, lid_t - art_depth]) linear_extrude(art_depth + eps) art();
  }
}

module ghosts() {
  % union() {
    translate([esp_x, -esp_l / 2, floor_t + esp_lift]) cube([esp_w, esp_l, esp_pcb + esp_top]);
    translate([esp_x + (esp_w - esp_ant_w) / 2, esp_l / 2, floor_t + esp_lift + esp_pcb])
      cube([esp_ant_w, esp_ant, esp_parts]);   // the antenna
    translate([rel_x, -rel_l / 2, floor_t + rel_stand]) cube([rel_w, rel_l, rel_h]);
    for (s = [-1, 1])   // room for the wires at the terminals
      translate([rel_x + 2, s > 0 ? rel_l / 2 : -rel_l / 2 - rel_wires, floor_t + rel_stand + 6]) cube([rel_w - 4, rel_wires, 6]);
    for (bb = [[usba, usba_y], [usbb, usbb_y]]) {
      b = bb[0]; y = bb[1];
      translate([bo_x(b), bo_front(b, y), floor_t + bo_lift]) cube([b[0], b[1], b[2]]);   // PCB
      translate([bo_x(b) + b[0] - 12, bo_front(b, y) + b[6], floor_t + bo_lift + b[2]])
        cube([12 + b[3], b[4], b[5]]);                                                  // socket
      translate([bo_x(b) + 1, bo_front(b, y) + 1, floor_t + bo_lift + b[2]])
        cube([4, b[1] - 2, b[10]]);                                                      // wires, caps
    }
  }
}

if (PART == "base") {
  base();
} else if (PART == "lid") {
  rotate([180, 0, 0]) lid();   // printed upside down: flat top on the bed
} else if (PART == "lid_inlay") {   // for a multi-filament printer: fills the recesses flush, load with "lid"
  rotate([180, 0, 0]) translate([0, 0, lid_t - art_depth]) linear_extrude(art_depth)
    difference() { intersection() { outline(); art(); } lid_holes(); }
} else if (PART == "face") {   // the lid seen from outside, in its two colours
  color("#f4f4f0") linear_extrude(lid_t - 0.01) difference() { intersection() { outline(); art(); } lid_holes(); }
  color("#1b1b1b") linear_extrude(lid_t) difference() { outline(); art(); lid_holes(); }
} else if (PART == "inside") {
  base();
  ghosts();
} else {
  base();
  ghosts();
  translate([0, 0, base_h + 25]) lid();
}

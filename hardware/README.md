# Hardware

A 2-channel microphone and instrument input board for a Teensy 4.0 stacked on a PJRC Audio Shield Rev D. Designed in Fusion Electronics between June and September 2026.

Final revision (2026-09-07): 100 x 140 mm, 2 copper layers, 1.6 mm, 57 placed parts, 188 holes from 0.66 to 2.92 mm. Every part in the BOM is through-hole or a plug-in module.

## Contents

| Path | What it is |
|---|---|
| `design/` | Final schematic and board (2026-09-07), saved as Eagle XML by Fusion Electronics, and the design-rules file |
| `design/history/` | First schematic (2026-07-14) and first board (2026-08-06) |
| `fab-2026-09-07/` | Gerbers, drill files, BOM and pick-and-place export of the final board |
| `cnc-milled-2026-08/` | August revision: schematic and board as saved, Gerbers, drill files, G-code and height maps for milling the board on a desktop CNC |
| `renders/` | PNG renders made from the Gerber and drill files |

## Revisions

Times are Pacific.

| Date | Where | What it is |
|---|---|---|
| 2026-07-14 | `design/history/2026-07-14_v1_schematic.sch` | First full schematic, 41 parts. 0402 chip resistors and capacitors, 1206 tantalum capacitors, two NCJ6FI-H combo jacks, three pots and two toggle switches on the board, an XL6009 boost module for 48 V and a DD1718PA module for the op-amp rails, Teensy 4.0, a 0.96 in OLED module. The op-amps are generic symbols. |
| 2026-08-06 | `design/history/2026-08-06_v1_board.brd` | Matching board: parts placed, traces not routed. |
| 2026-08-06 to 08-13 | `cnc-milled-2026-08/design/` | New project started 2026-08-06. Through-hole parts (axial film resistors, film and electrolytic capacitors, DIP-8 ICs), THAT1512 and OPA1642 in place of generic op-amps, the Audio Shield footprint in place of separate Teensy and OLED parts, and headers for the jacks, gain pots, switches, encoder and OLED. Board outline set to 100 x 140 mm the same night. |
| 2026-08-14 | `cnc-milled-2026-08/` | Gerber and drill export from the board saved at 02:20, plus G-code and height maps for the top copper layer. |
| 2026-08-16 | (in `design/`) | The DD1718PA module is replaced by 5-pin header JP1, so the +12 V and -12 V rails now come from an external module. |
| 2026-09-07 | `design/`, `fab-2026-09-07/` | Final revision. Gerbers, drill, BOM and pick-and-place exported at 16:17; a second zip with two drill diameters merged was made at 16:33. |

## Final board

- Two THAT1512 microphone preamps (IC1, IC2). Gain is set by an off-board pot on header GAIN1 or GAIN2 in series with a 5.7 ohm resistor across the preamp's gain pins.
- One OPA1642 dual op-amp (IC3) as two unity-gain buffers for instrument inputs, 1 Mohm to ground at each input.
- 48 V phantom power: XL6009 boost module, two L-C stages (TDK B82144A2105J000 inductors with 47 nF film capacitors), a 2N5551 transistor ripple filter, 6.8 kohm feed resistors, 22 uF 63 V coupling capacitors, and an off-board switch per channel.
- The XL6009 module's input is the 5 V rail from the Audio Shield. Its footprint is four pads on a 46.0 x 20.6 mm rectangle (IN-, OUT-, OUT+, IN+).
- +12 V and -12 V for the op-amps come from an external module on header JP1 (pins: 5 V, GND, +12 V, GND, -12 V). The module is not in the BOM.
- A footprint for the Audio Shield (SparkFun DEV-15845 library) with the Teensy 4.0 stacked on it. The Teensy is not a BOM line. Only the shield's line-in, I2C, 3.3 V, 5 V, ground and pins 2, 3 and 4 are wired; the shield's output pins are not routed to the board.
- Mic or instrument selection per channel is a switch on header INST_S1 or INST_S2.

Headers (2.54 mm):

| Ref | Pins |
|---|---|
| XLR1, XLR2 | 1 (ground), 2, 3, tip, sleeve. Pins 2 and 3 are the balanced pair, tip is the instrument input. |
| GAIN1, GAIN2 | Gain pot |
| PHTM_S1, PHTM_S2 | Phantom switch: 1 ground, 2 feed resistors, 3 filtered 48 V |
| INST_S1, INST_S2 | Input switch: 1 mic path, 2 to the Audio Shield input, 3 instrument path |
| JP1 | 5 V, GND, +12 V, GND, -12 V |
| JP9 | Rotary encoder: 1 GND, 2 3.3 V, 3 SW, 4 DT, 5 CLK |
| JP10 | I2C OLED: 1 3.3 V, 2 GND, 3 SCL, 4 SDA |

## Ordering the final board

1. Zip the `.gbr` files in `fab-2026-09-07/gerbers/` together with one drill file from step 2, and upload the zip as a 2-layer, 100 x 140 mm, 1.6 mm board.
2. I ordered mine from PCBWay (five boards, HASL finish, 1 oz copper) with the original Sep 7 zip, so the drill file is `fab-2026-09-07/drill/drill_1_64.xln`. `drill_1_64_JLC_consolidated.xln` is an alternate I made for JLCPCB: the same 188 holes with 0.750 mm holes changed to 0.762 mm and 1.000 mm holes changed to 1.016 mm, which leaves 8 distinct diameters instead of 10.
3. `fab-2026-09-07/assembly/` has the Fusion BOM and pick-and-place text exports. All parts are through-hole, so assembly is by hand.
4. Not in the BOM: the XLR or combo jacks, gain pots, switches, encoder, OLED, the +/-12 V module, the Audio Shield and the Teensy.
5. The BOM row for IC3 is labeled OPA1642 but its package and vendor fields were copied from the THAT1512 entry. Check the footprint before ordering parts.

## Rendering and design rules

`renders/sep7_top.png` and `sep7_bottom.png` show the final board with solder mask and silkscreen; `aug14_cnc_copper_top.png` and `aug14_cnc_copper_bottom.png` show the copper of the August revision. `design/design-rules_customPCB.edru` sets 6 mil minimum copper width, 35 mil minimum drill, and 16 to 20 mil between wires, pads and vias.

## Third-party data

The schematic and board embed the part symbols and footprints they use. The audio shield, Teensy, OLED, NCJ6FI-H and OPA1642 library files this work started from carry a CC BY-SA 4.0 (with Design Exception 1.0) header. The BOM carries an Ultra Librarian copyright line for the TE Connectivity resistor footprint. The 2N5551 and QYX2A104JTP parts come from SamacSys and carry SamacSys's licence text.

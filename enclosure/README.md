# Enclosure

Artwork and printed-part files for the audio interface enclosure. The 3D design lives in Fusion; the Fusion geometry cannot be exported here, so this folder holds the 2D artwork and the 3D-print project, plus a list of the Fusion designs.

## top-plate/

A line drawing of the board layout for the top plate: top copper solid, bottom copper dashed, part labels, the wires running from the board headers to the XLR jacks, gain pots, switches, OLED and encoder, and the text "BARE-METAL AUDIO", "2-CHANNEL MIC PREAMP / USB AUDIO INTERFACE", "TEENSY 4.0, THAT1512, OPA1642, PCB 100 x 140 mm, 2 LAYER".

| File | Date | Notes |
|---|---|---|
| `top-plate.svg` | 2026-09-15 | 180 x 168 mm drawing area |
| `top-plate.png` | 2026-09-15 | 1555 x 1420 px, readable orientation |
| `top-plate.dxf` | 2026-09-15 | Millimeters, 149.59 x 136.09 mm. Layers: PLATE, CU_BOT, CU_TOP, PADS, SILK, HARNESS, PANEL, PANEL_DETAIL, OUTLINE, RULE, TITLE, TEXT |
| `top-plate_rotated-180.dxf` | 2026-09-15 | The same drawing rotated 180 degrees |

## printed-parts/

`audiointerfacesketching.3mf` is a Bambu Studio 2.4.0.70 project saved 2026-09-19: 13 plates and 54 objects for a Bambu Lab A1 with a 0.4 mm nozzle, Bambu PLA Matte, 0.16 mm layers, 15 percent infill, 2 wall loops and supports on.

Part names in the project: backedgemetal, body4, bottom, bottommetal, bracket, bracket2, cornerconnector, cutout1, cutout2, edgeconnector, edgeconnector2, edgeconnector3, edgeunbent, feet, frontmetal, headphoneholder, metalcutoutfront, metalcutoutside, new_cutout, newcornerbracket, one, two, topmetal, wireholder and Stencil.

## Fusion designs

Autodesk Fusion cloud designs, read from the offline cache. Dates are Pacific time. "Versions" is the cloud version count. For `Bare Metal Audio` the last-saved date is the date of the local file. Cloud version 55 was uploaded on 2026-10-29 with the description "Updated From Offline Mode".

| Design | Versions | Created | Last saved |
|---|---|---|---|
| XLR Combo | 3 | 2026-09-07 19:23 | 2026-09-09 21:27 |
| Switch | 1 | 2026-09-07 19:31 | 2026-09-07 19:31 |
| Pot | 4 | 2026-09-07 19:36 | 2026-09-09 21:06 |
| OLED | 6 | 2026-09-07 19:47 | 2026-09-11 08:39 |
| TeensyAudioBoard | 1 | 2026-09-07 20:00 | 2026-09-07 20:00 |
| Teensy_4.0_Assembly | 3 | 2026-09-07 20:03 | 2026-09-07 20:13 |
| audio interface enclosure | 16 | 2026-09-07 21:34 | 2026-09-08 22:00 |
| audio interface enclosure unpatterned | 2 | 2026-09-08 22:35 | 2026-09-08 22:52 |
| Bare Metal Audio | 55 | 2026-09-08 22:40 | 2026-09-19 14:14 |
| ROTENC | 2 | 2026-09-09 13:41 | 2026-09-09 13:56 |
| Bare Metal Audio Fixer | 4 | 2026-09-14 12:44 | 2026-09-14 13:39 |
| interfaceFEET | 2 | 2026-09-15 20:37 | 2026-09-15 20:37 |
| WireHolder | 7 | 2026-09-17 09:50 | 2026-09-17 16:26 |

Names found inside the `Bare Metal Audio` file include "audio interface enclosure unpatterned", "Text1" and "Real PCB Engraving".

The `Bare Metal Audio` design also holds CAM: 4 setups and 8 operations (six 2D contours 0.10 to 0.20 mm deep with 0.04 to 0.15 mm stepdown, and two Trace operations), The contours use a 3.175 mm 2-flute carbide flat end mill (tool entry "Alumium Dreanique SP2FC 3.1mm 2F") at 10,000 rpm, 200 mm/min, plunge 80. The Trace operations use V-bits (chamfer mills): one with a 10 degree taper and 0.2 mm tip, one with a 15 degree taper and 0.1 mm tip, at 10,000 rpm, 100 mm/min, plunge 40. The setups use model extents of 143 x 34 x 0.5 mm, 129.5 x 34 x 0.5 mm and 136.09 x 149.59 x 0.5 mm (two setups); the last matches the extents of `top-plate/top-plate.dxf`.

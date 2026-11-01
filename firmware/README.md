# Firmware

Two Arduino sketches for a Teensy 4.0 stacked on a PJRC Audio Shield Rev D (SGTL5000 codec). Both make the board show up on a computer as a USB audio device.

| Folder | File date | What it is |
|---|---|---|
| `audio_interface/` | 2026-06-30 | First sketch, about 100 lines. Line-in goes to USB, USB playback goes to the headphone and line outputs, with an optional monitor mix. |
| `audio_interface_ui/` | 2026-08-04 | About 1,050 lines. Adds a 128x64 OLED and a rotary encoder: level meters, scope view, test tone, mic/instrument select per channel, settings kept in EEPROM. |

## Build

1. Install the Arduino IDE and Teensyduino.
2. Tools > Board: Teensy 4.0.
3. Tools > USB Type: Audio. This is what makes the computer see an audio device. `audio_interface_ui` also accepts "Serial + MIDI + Audio".
4. Open the sketch folder and upload.

Libraries used: `Audio` (PJRC Teensy Audio Library), `Wire` and `EEPROM`. All three come with Teensyduino. The OLED driver, 5x7 font, icons, encoder decoder and menus are written inside `audio_interface_ui.ino`, so no display or encoder library is needed.

## audio_interface

- Line-in from the codec goes to USB out. USB in goes through two output mixers to the codec's headphone and line outputs.
- A direct-monitor path mixes line-in into the outputs. It is off by default (`monitorLevel = 0.0`).
- Codec settings in `setup()`: line-in level 5 (range 0 to 15), line-out level 13, headphone volume 0.5.
- An optional headphone-volume pot on pin 15 (A1) is compiled out by default (`USE_VOLUME_POT 0`).
- The header comment describes the result as a 2-in, 2-out USB device at 16-bit, 44.1 kHz.

## audio_interface_ui

Screens: boot animation, home, left input, right input, settings.

- Home: left and right input cells (MIC or INST), a settings icon, headphone volume, a meter area, and a dBFS readout. "CLIPPING DETECTED" replaces the readout when a record-path peak reaches 0.99 of full scale; the message is held for 1 to 9 seconds (default 2).
- Meter styles (setting METER): WAVE, a scrolling 128-column peak history that adds a dashed playback outline when MON BLEND is above 0 (default); BARS; SCOPE.
- Scope: samples go into a 2048-sample ring per channel through `AudioRecordQueue`. Each frame searches backwards for a trigger crossing and draws 128 columns with min/max decimation. Timebase is 1, 2, 4 or 8 samples per pixel (2.9, 5.8, 11.6 or 23.2 ms across the screen). Trigger level runs 0 to 50 in steps of 5, rising or falling edge, AUTO or NORMAL mode.
- Test tone: 440 Hz at 0.5 amplitude (-6 dBFS) on left, right or both. It replaces the live input, so it shows on the meters, in the headphones and on the USB stream. It is never saved.
- Monitor mix: MON BLEND crossfades between live input and computer playback with equal-power gains. MONITOR switches the output path on or off.
- Settings list: MIC GAIN, INST GAIN, MONITOR, MON BLEND, METER, TIMEBASE, TRIG LVL, TRIG EDGE, TRIG MODE, CLIP HOLD, BOOT ANIM, TEST TONE, DEFAULTS, BACK. The four scope items only appear when METER is SCOPE.
- Input choice per channel sets the SGTL5000 line-in level (defaults: mic 12, instrument 0) and drives pins 5 and 9.
- EEPROM: a settings struct at address 0 with a magic number, version 3 and a checksum. It is written 2 seconds after the last change.
- Boot animation lasts about 2.9 seconds; a knob press skips it, and BOOT ANIM turns it off.
- Display: SH1106 by default (`DISPLAY_SH1106 1`). Set it to 0 for an SSD1306. I2C address 0x3C, I2C clock 1 MHz. `ENC_REVERSE` flips the encoder direction.
- Encoder and button are read from a 2 kHz `IntervalTimer`, not from `loop()`. Button debounce is 25 ms and a long press is 650 ms.

## Pins used by the sketches

| Pin | Use |
|---|---|
| 18, 19 | I2C SDA and SCL: the OLED and the Audio Shield's SGTL5000 control bus |
| 2 | Encoder CLK |
| 3 | Encoder DT |
| 4 | Encoder switch |
| 5 | Left input select, HIGH = instrument, LOW = mic |
| 9 | Right input select |
| 15 | Optional volume pot (`audio_interface` only, disabled) |

The OLED runs from 3.3 V. The sketch comments warn against 5 V because Teensy 4 pins are not 5 V tolerant.

## Match with the hardware

The 2026-09-07 board in `../hardware/design/` does not follow these pin numbers exactly. Its schematic wires the encoder header JP9 pins labeled SW, DT and CLK to Teensy pins 2, 3 and 4, and leaves pins 5 and 9 unconnected. On that board mic or instrument is chosen with the INST_S1 and INST_S2 switches, so the menu choice only changes the codec gain setting. Change the `PIN_ENC_*` defines to match the wiring you build.

# Bare Metal Audio

<p align="center">
  <img src="media/2026-09-17-1941-assembled.jpg" width="560" alt="The finished interface in an aluminum box: two combo jacks, two gain knobs, toggle switches and an OLED reading BARE METAL AUDIO, 2 CH USB INTERFACE">
</p>

A 2-channel USB audio interface I designed and built over summer 2026. Each channel takes a mic (with 48 V phantom power) or an instrument, a Teensy 4.0 handles the USB audio, and there's a little OLED screen with one knob. It worked on a breadboard in August. I milled my own circuit board on a desktop CNC, gave up on that and ordered proper ones from PCBWay, and built the whole thing into an aluminum box with the circuit engraved on the lid. About ten seconds after I took the photo above, I fried it.

I'm still proud of it. That's such a cool way to completely fail at something.

**[Watch it working on the breadboard (2 min)](media/2026-08-05-first-dual-recording.mp4).** The sound in that video is the interface's own recording: my voice through the mic input and a guitar through the instrument input, both going through it at the same time.

## How it went

### June: a bench at home

<p>
  <img src="media/2026-06-27-bench.jpg" width="380" alt="A desk with an oscilloscope, a soldering station, a lamp and trays of parts">
  <img src="media/2026-06-29-first-sine.jpg" width="230" alt="A sine wave on the oscilloscope">
</p>

I spent the summer at home in Colorado with an internship during the day, so most of this got built at night. It started as a plan on June 18 and a first schematic on June 22 (surface-mount parts, with the jacks and knobs on the board). Then a bench: a Rigol scope, a soldering station, a bench supply and a lot of little drawers.

### June 30, 1:11 am: 48 volts

<p>
  <img src="media/2026-06-30-0111-48v.jpg" width="380" alt="Oscilloscope reading a flat line at about 49.9 volts">
  <img src="media/2026-06-30-0112-boost-converter.jpg" width="380" alt="A boost converter module on a breadboard with probes clipped on">
</p>

Condenser mics need 48 V phantom power. By 1 am on June 30 the boost converter that makes it was putting out about 50 V on the scope.

### July: breadboards

<p>
  <img src="media/2026-06-30-2351-bench.jpg" width="250" alt="Film-look photo of the bench at night: breadboards, scope and bench supply">
  <img src="media/2026-07-13-2327-breadboard.jpg" width="250" alt="Film-look close-up of a breadboard full of jumper wires">
  <img src="media/2026-07-15-0006-mic.jpg" width="250" alt="Film-look photo of an AKG condenser mic in a shock mount in front of the breadboards">
</p>

The first recording came out of it on July 13 ([listen](media/2026-07-13-first-output.mp3)). I'm pretty sure the song is one I wrote and never put out anywhere. Two nights later, at midnight, I was testing my AKG condenser mic on the breadboard preamp.

### August 4-5: it works, kind of

<p>
  <img src="media/2026-08-04-boot-animation.gif" width="300" alt="OLED boot animation: BARE METAL AUDIO between two sine waves, then level meters">
  <img src="media/2026-08-04-menu.gif" width="300" alt="Turning the knob through the settings menu on the OLED">
</p>

The firmware grew a whole little UI: a boot animation, level meters for both channels, a clipping warning, a scope view with a trigger, a test tone, and a settings menu that saves itself. It's one 1,050-line sketch with its own OLED driver and font, so it doesn't need any display libraries.

Then I recorded both channels at once and filmed it ([video](media/2026-08-05-first-dual-recording.mp4)). From that night:

> I am kind of in awe here at the fact that I got this to work at all, but it's very scuffed.

> You can definitely hear the "pspspsps" from the audio buffer currently being incorrectly handled in the firmware, but that is fixable.

> One of the really cool things about these audio electronics breadboards is you can, like, come and just tap on things. See how it affects it. This makes me really scared for the final enclosure, because I don't know how on earth I'm going to prevent this.

### August 6-7: starting the board over

<img src="hardware/renders/sep7_top.png" width="300" alt="Render of the through-hole board: two preamps, a buffer, the boost converter footprint and a row of headers along one edge">

The next night I started the board over from scratch. The new version is all through-hole parts, names the actual chips (THAT1512 preamps, an OPA1642 buffer) instead of generic op-amp symbols, treats the Teensy and its audio shield as one module, and moves every jack, knob and switch off the board onto headers. I started it at 9:57 pm on August 6 and it was fully routed at 11:09 pm on August 7. Fusion saved the schematic 30 times and the board 47 times that week.

### August 11-14: milling it myself

<p>
  <img src="media/2026-08-11-copper-and-end-mills.jpg" width="220" alt="Copper-clad boards and a row of tiny end mills laid out on a red Stanford mousepad">
  <img src="media/2026-08-12-cnc.jpg" width="300" alt="A small desktop CNC router with a spoil board on its bed">
  <img src="media/2026-08-14-milling.gif" width="300" alt="The CNC cutting isolation traces into a copper board">
</p>

Copper clad, a pile of tiny end mills, and a desktop CNC (a Genmitsu 3018-PROVer). The copper isn't flat enough to just cut, since it varied by about as much as the cut depth, so I probed a 14 x 10 grid of heights first and let the controller follow it. The two toolpaths from the night of August 13 are called `Good State.nc` (0.16 mm deep, 2:26 am) and `idontevenknow.nc` (0.08 mm deep, 3:25 am). That same night the Fusion project went from `AUDIO INTERFACE` to `AUDIO INTERFACE_BROKEN` to `AUDIO INTERFACE_brokener?`. I'd edited some of the library footprints so the mill wouldn't drill certain holes, and that left footprints misaligned and the design-rule check full of errors. Every attempt was save a backup, try a fix, and I ran out of names.

<p>
  <img src="media/2026-08-14-0153-milling.jpg" width="300" alt="The mill partway through cutting traces into copper">
  <img src="media/2026-08-14-0554-milled.jpg" width="300" alt="Film-look photo of the finished milled board on the CNC bed with copper offcuts around it">
  <img src="media/2026-08-14-0555-traces.jpg" width="220" alt="Close-up of milled copper traces and pads">
</p>

The last photo of that board is from 5:55 am. It didn't work. Copper dust packed into the cut channels and shorted the traces, and I didn't have the right chemicals or tools to clean it out. Trace width was a problem across the whole board, and every failed cut meant over an hour to set up and probe a fresh blank.

### September: real boards

<p>
  <img src="media/2026-09-16-boards-arrive.jpg" width="250" alt="The black PCBWay board with the Teensy module and power modules soldered on">
  <img src="media/2026-09-17-0021-wiring.jpg" width="300" alt="Film-look photo of the front panel being wired to the board">
  <img src="media/2026-09-17-0114-populated.jpg" width="300" alt="The fully populated board: preamps, film capacitors, big electrolytics, the boost converter and the Teensy">
</p>

On the afternoon of September 7 I was still making toolpaths and drill programs for the mill (`07mmholes`, `actual07holes`, `08holes`, `08real`, `1mmtop`, `1mmreal`). Then I made the call to stop milling and have the boards made. At 5:17 pm I packaged the board (the Fusion project for that one is called `AUDIO INTERFACE_brokener_forbuying`) and ordered five from PCBWay for $79.86. The build time was supposed to be 3 to 4 days, but production hit an error and the order slipped by several days, in the middle of a race to finish the whole thing before I left for Stanford. By September 16 the boards were on my desk. Seeing the thing I designed in this real, magical form factor was awesome. The board was fully populated by 1:14 am on the 17th.

### September 7-17: the box

<p>
  <img src="media/2026-09-14-panel-milling.gif" width="300" alt="The CNC cutting a rectangular window into an aluminum panel">
  <img src="media/2026-09-15-front-panel.jpg" width="300" alt="The 3D-printed prototype of the front panel: black, textured, with two combo jacks, knobs, four toggle switches and a window for the screen">
  <img src="media/2026-09-17-1940-meters.jpg" width="300" alt="The assembled interface with the OLED showing input labels and levels in dBFS">
</p>

Three hours after packaging the boards I started modeling the enclosure (the Fusion design for it has 55 versions). It's flat aluminum plates held together with printed brackets. I 3D-printed a prototype of every piece first to make sure it all fit (that's the printed front panel above), then cut and engraved the aluminum panels on the same CNC, including the window for the screen. The lid has the whole board layout engraved on it, traces and all, with labels and the wires running out to the jacks:

<img src="enclosure/top-plate/top-plate.png" width="520" alt="Line drawing of the board for the lid: top and bottom copper, part labels, wires to every jack, BARE-METAL AUDIO title and my name">

By 7:40 pm on September 17 it was all in the box with the screen working.

### About ten seconds later: I fried it

The photo at the top of this page is the last one of it working. I'd connected the front panel parts to the board with ordinary Dupont jumper connectors that I crimped by hand, and my guess is that a few of them shorted. The Teensy is dead dead. I haven't checked the shield or the preamps, since nothing works without the Teensy anyway.

There was no time to diagnose it before I left for Stanford, and I've been too busy on campus since then to give it the effort. It lives in a box in my dorm now.

## How it works

```
 mic (XLR) --- 22 uF coupling --- THAT1512 preamp (gain pot) ---\
                                                                  [mic/inst switch] --- SGTL5000 line in --- Teensy 4.0 --- USB
 instrument (1/4") --- OPA1642 buffer, 1 Mohm input ------------/      (PJRC Audio Shield)                (2 in / 2 out, 16-bit, 44.1 kHz)

 48 V phantom:  5 V --- XL6009 boost --- two LC stages + 2N5551 ripple filter --- switch per channel --- 6.8 kohm feeds to XLR pins 2 and 3
 +/-12 V for the preamps and the buffer: an external module on header JP1
 OLED (128x64, I2C) and a rotary encoder on the Teensy
```

### Analog side

- **Mic preamps:** two THAT1512s. The mic's balanced pair comes in through 22 uF 63 V coupling capacitors (which keep the 48 V off the preamp), and gain is set by an off-board pot across the gain pins.
- **Instrument inputs:** an OPA1642 dual op-amp as two unity-gain buffers with 1 Mohm inputs, so a guitar sees a high-impedance input.
- **Phantom power:** an XL6009 boost module makes about 48 V, then two inductor-capacitor stages and a 2N5551 transistor filter clean it up before it goes out through a toggle switch and 6.8 kohm feed resistors on each channel.
- **Selection:** a switch per channel picks the mic path or the instrument path into the codec's line input.

### Digital side

A Teensy 4.0 on a PJRC Audio Shield (SGTL5000 codec) shows up on a computer as a 2-in, 2-out USB audio device. The UI firmware ([`firmware/`](firmware/)) adds:

- level meters (scrolling waveform, bars, or a triggered scope view with four timebases), with a "CLIPPING DETECTED" warning that holds for a few seconds,
- a per-channel mic or instrument setting that sets the codec gain,
- a monitor blend between the live input and computer playback,
- a 440 Hz test tone on either channel,
- a boot animation (a sine wave sweeps in and collapses while the name types out),
- settings saved to EEPROM two seconds after the last change.

The display is slow to update (a full redraw blocks for about 26 ms), so the knob is read from a 2 kHz timer interrupt instead of the main loop, and nothing gets missed while the screen draws.

### The board

100 x 140 mm, 2 layers, 57 parts, 188 holes, all through-hole. No vias and no ground pour. Every trace is 16 mil, and ground is routed like any other signal. Revisions, the parts list and how to order one are in [`hardware/README.md`](hardware/README.md).

<p>
  <img src="hardware/renders/sep7_bottom.png" width="250" alt="Render of the bottom side of the final board">
  <img src="hardware/renders/aug14_cnc_copper_top.png" width="250" alt="Render of the top copper of the August board, the layer I milled">
</p>

## What's in here

| Path | What it is |
|---|---|
| [`firmware/`](firmware/) | The first USB audio sketch (June 30) and the UI sketch (August 4) |
| [`hardware/`](hardware/) | The Eagle schematic and board files (final, first and August versions), Gerbers, drill files, BOM and pick-and-place for the board I ordered, the August files I milled (Gerbers, G-code, height maps), and renders |
| [`enclosure/`](enclosure/) | The lid engraving (SVG, PNG, DXF), the 3D-print project for the brackets and panels, and a list of the Fusion designs |
| [`media/`](media/) | Photos, GIFs, the demo video and the first recording |

## Honest notes

- I don't have noise or latency numbers for it. The two recordings in `media/` are what I have.
- The breadboard version has the buffer bug you can hear in the video.
- I never tracked down what shorted.
- The UI sketch was written for the breadboard, and its encoder pins don't match the final board's encoder header. [`firmware/README.md`](firmware/README.md) has the details.

## Credits

- PJRC for the Teensy, the Audio Shield and the Teensy Audio Library.
- Candle, for running the CNC and following the height maps.
- Part symbols and footprints from the libraries listed in [`hardware/README.md`](hardware/README.md).

## License

MIT for my firmware and design files ([`LICENSE`](LICENSE)). The part libraries embedded in the Eagle files keep their own licences; see [`hardware/README.md`](hardware/README.md#third-party-data).

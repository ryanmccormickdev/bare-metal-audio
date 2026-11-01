/*
 * DIY USB Audio Interface  —  Teensy 4.0 + Audio Shield Rev D (SGTL5000)
 * Presents to your laptop as a 2-in / 2-out USB Audio device (16-bit, 44.1 kHz).
 *
 * SIGNAL ROUTING
 *   Line IN  (your analog front-end) ->  USB OUT           (your DAW records this)
 *   USB IN   (DAW playback)          ->  Line / Headphone OUT (you hear this)
 *   Optional zero-latency monitoring blends Line IN straight into the output.
 *
 * BUILD SETTINGS (Arduino IDE, after installing Teensyduino)
 *   Tools > Board    > Teensy 4.0
 *   Tools > USB Type > "Audio"      <-- REQUIRED. This is what makes the laptop
 *                                       see an audio device at all.
 *
 * This is milestone B-B: flash it, confirm the laptop sees "Teensy Audio,"
 * play something from the laptop (you should hear it), and record from it.
 * The inputs stay silent until your analog front-end is wired to LINE IN —
 * that's expected at this stage.
 */

#include <Audio.h>
#include <Wire.h>

// ---------- Audio objects ----------
AudioInputI2S        lineIn;   // stereo LINE IN from the SGTL5000 (your preamps/buffers)
AudioInputUSB        usbIn;    // audio coming FROM the computer (DAW playback)
AudioMixer4          mixL;     // left  output mix: playback + direct monitor
AudioMixer4          mixR;     // right output mix
AudioOutputI2S       lineOut;  // stereo output to the SGTL5000 (headphones / line out)
AudioOutputUSB       usbOut;   // audio going TO the computer (your recording)
AudioControlSGTL5000 codec;

// ---------- Connections ----------
// Recording path: physical inputs -> computer
AudioConnection patchRecL (lineIn, 0, usbOut, 0);
AudioConnection patchRecR (lineIn, 1, usbOut, 1);

// Playback path: computer -> output mixers (mixer input 0)
AudioConnection patchPlayL(usbIn,  0, mixL, 0);
AudioConnection patchPlayR(usbIn,  1, mixR, 0);

// Direct-monitor path: physical inputs -> output mixers (mixer input 1)
AudioConnection patchMonL (lineIn, 0, mixL, 1);
AudioConnection patchMonR (lineIn, 1, mixR, 1);

// Output mixers -> physical output
AudioConnection patchOutL (mixL,   0, lineOut, 0);
AudioConnection patchOutR (mixR,   0, lineOut, 1);

// ---------- User-tunable settings ----------
float headphoneVolume = 0.5f;  // 0.0 - 1.0, headphone amp level
float playbackLevel   = 1.0f;  // DAW playback into the output mix
float monitorLevel    = 0.0f;  // direct input monitoring; 0.0 = OFF.
                               // Raise (e.g. 0.8) for zero-latency monitoring,
                               // but then turn OFF your DAW's software monitor
                               // for that track or you'll hear a doubled echo.

// Optional physical headphone-volume pot on pin A1 (Teensy pin 15).
// Keep this 0 until the pot is actually wired — a floating analog pin
// will make the volume jitter.
#define USE_VOLUME_POT 0
const int VOLUME_POT_PIN = 15;   // A1

void setup() {
  AudioMemory(20);

  codec.enable();
  codec.unmuteLineout();
  codec.lineOutLevel(13);
  codec.inputSelect(AUDIO_INPUT_LINEIN);   // your front-end feeds LINE IN, not the mic pin
  codec.volume(headphoneVolume);

  // LINE IN sensitivity: 0 (highest full-scale voltage) .. 15 (most sensitive).
  // This is your curriculum "fix #2" level-match knob: pick the LOWEST number
  // that does not clip on your loudest source, so you use the converter's full
  // range without overloading it.
  codec.lineInLevel(5);

  // Output mixer gains
  mixL.gain(0, playbackLevel);
  mixR.gain(0, playbackLevel);
  mixL.gain(1, monitorLevel);
  mixR.gain(1, monitorLevel);

#if USE_VOLUME_POT
  pinMode(VOLUME_POT_PIN, INPUT);
#endif
}

void loop() {
  // The audio engine runs on interrupts, so loop() is free.
#if USE_VOLUME_POT
  static elapsedMillis t;
  if (t > 50) {                              // update ~20x per second
    t = 0;
    float v = analogRead(VOLUME_POT_PIN) / 1023.0f;
    codec.volume(v);
  }
#endif
}

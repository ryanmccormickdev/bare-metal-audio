/* ============================================================================
   2-Channel USB Audio Interface — control surface firmware
   Teensy 4.0 / 4.1 + PJRC Audio Shield Rev D
   128x64 I2C OLED (SH1106 or SSD1306) + KY-040 rotary encoder

   No display or encoder libraries: the SSD1306/SH1106 driver, 5x7 font,
   icons, quadrature decoder and menu system are all in this file.
   Only Wire (I2C), EEPROM and Audio are used, and all three ship with
   Teensyduino.

   Tools > USB Type must be set to "Audio" (or "Serial + MIDI + Audio").
   ========================================================================== */

#include <Wire.h>
#include <EEPROM.h>
#include <Audio.h>

/* ------------------------------ build config ----------------------------- */

// Most 1.3" modules are SH1106. Most 0.96" modules are SSD1306.
// If the image looks shifted 2px sideways or the edges wrap, flip this.
#define DISPLAY_SH1106    1
#define OLED_ADDR         0x3C

#define ENC_REVERSE       0        // set to 1 if the knob scrolls backwards

#define DEVICE_L1         "BARE METAL"
#define DEVICE_L2         "AUDIO"
#define DEVICE_SUB        "2 CH USB INTERFACE"

/* --------------------------------- pins ---------------------------------- */
// OLED SDA -> 18, SCL -> 19  (shared with the audio shield's control bus)
// OLED VDD -> 3.3V           (NEVER 5V: Teensy 4 pins are not 5V tolerant)

#define PIN_ENC_CLK       2
#define PIN_ENC_DT        3
#define PIN_ENC_SW        4
#define PIN_SEL_L         5        // HIGH = instrument, LOW = mic
#define PIN_SEL_R         9

/* ------------------------------ audio graph ------------------------------ */

AudioInputI2S            i2sIn;
AudioInputUSB            usbIn;         // playback from the computer
AudioSynthWaveformSine   testTone;
AudioMixer4              srcL;          // live input + test tone
AudioMixer4              srcR;
AudioAnalyzePeak         peakL;
AudioAnalyzePeak         peakR;
AudioAnalyzePeak         peakPbL;       // playback, metered separately
AudioAnalyzePeak         peakPbR;
AudioOutputUSB           usbOut;
AudioMixer4              monL;
AudioMixer4              monR;
AudioOutputI2S           i2sOut;
AudioRecordQueue         qL;            // raw samples for the scope view
AudioRecordQueue         qR;
AudioControlSGTL5000     sgtl;

// Everything downstream taps srcL/srcR, so the tone shows up on the meters,
// in the headphones and on the USB stream — one switch, whole chain tested.
// The monitor mixers blend that live path (input 0) against computer
// playback (input 1); only the live path is ever recorded.
AudioConnection pc1 (i2sIn,    0, srcL,   0);
AudioConnection pc2 (i2sIn,    1, srcR,   0);
AudioConnection pc3 (testTone, 0, srcL,   1);
AudioConnection pc4 (testTone, 0, srcR,   1);
AudioConnection pc5 (srcL,     0, peakL,  0);
AudioConnection pc6 (srcR,     0, peakR,  0);
AudioConnection pc7 (srcL,     0, usbOut, 0);
AudioConnection pc8 (srcR,     0, usbOut, 1);
AudioConnection pc9 (srcL,     0, monL,   0);
AudioConnection pc10(srcR,     0, monR,   0);
AudioConnection pc11(usbIn,    0, monL,   1);
AudioConnection pc12(usbIn,    1, monR,   1);
AudioConnection pc13(usbIn,    0, peakPbL, 0);
AudioConnection pc14(usbIn,    1, peakPbR, 0);
AudioConnection pc15(monL,     0, i2sOut, 0);
AudioConnection pc16(monR,     0, i2sOut, 1);
AudioConnection pc17(srcL,     0, qL,     0);
AudioConnection pc18(srcR,     0, qR,     0);

/* ------------------------------- 5x7 font -------------------------------- */
/* ASCII 32..95. Five column bytes per glyph, bit 0 = top row.               */

static const uint8_t font5x7[64][5] = {
  {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00}, // sp !
  {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14}, // " #
  {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62}, // $ %
  {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00}, // & '
  {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00}, // ( )
  {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08}, // * +
  {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08}, // , -
  {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02}, // . /
  {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00}, // 0 1
  {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31}, // 2 3
  {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39}, // 4 5
  {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03}, // 6 7
  {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E}, // 8 9
  {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00}, // : ;
  {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14}, // < =
  {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06}, // > ?
  {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E}, // @ A
  {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22}, // B C
  {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41}, // D E
  {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A}, // F G
  {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00}, // H I
  {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41}, // J K
  {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F}, // L M
  {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E}, // N O
  {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E}, // P Q
  {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31}, // R S
  {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F}, // T U
  {0x1F,0x20,0x40,0x20,0x1F}, {0x7F,0x20,0x18,0x20,0x7F}, // V W
  {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07}, // X Y
  {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00}, // Z [
  {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00}, // \ ]
  {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40}  // ^ _
};

/* --------------------------------- icons --------------------------------- */
/* 11x11, one uint16_t per row, bit 10 = leftmost column.                    */

static const uint16_t icoMic[11] = {
  0x070,0x0F8,0x0F8,0x0F8,0x0F8,0x104,0x104,0x0F8,0x020,0x020,0x0F8
};
static const uint16_t icoPlug[11] = {
  0x0F8,0x0F8,0x0F8,0x0F8,0x070,0x070,0x0F8,0x070,0x070,0x020,0x020
};
/*  ....###....
    .#..###..#.
    ..#######..
    ..###.###..
    ####...####
    ###.....###
    ####...####
    ..###.###..
    ..#######..
    .#..###..#.
    ....###....   */
static const uint16_t icoGear[11] = {
  0x070,0x272,0x1FC,0x1DC,0x78F,0x707,0x78F,0x1DC,0x1FC,0x272,0x070
};
static const uint16_t icoPhones[11] = {
  0x0F8,0x104,0x202,0x401,0x401,0x603,0x603,0x603,0x603,0x603,0x000
};

/* ----------------------------- display driver ---------------------------- */

static uint8_t fb[1024];

static void dispCmd(uint8_t c) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(c);
  Wire.endTransmission();
}

static void dispInit() {
#if DISPLAY_SH1106
  static const uint8_t seq[] = {
    0xAE, 0xD5,0x80, 0xA8,0x3F, 0xD3,0x00, 0x40,
    0xAD,0x8B, 0x32, 0xA1, 0xC8, 0xDA,0x12,
    0x81,0x80, 0xD9,0x1F, 0xDB,0x40, 0xA4, 0xA6, 0xAF
  };
#else
  static const uint8_t seq[] = {
    0xAE, 0xD5,0x80, 0xA8,0x3F, 0xD3,0x00, 0x40,
    0x8D,0x14, 0x20,0x00, 0xA1, 0xC8, 0xDA,0x12,
    0x81,0xCF, 0xD9,0xF1, 0xDB,0x40, 0xA4, 0xA6, 0xAF
  };
#endif
  for (unsigned i = 0; i < sizeof(seq); i++) dispCmd(seq[i]);
}

static void dispFlush() {
  for (uint8_t page = 0; page < 8; page++) {
    dispCmd(0xB0 | page);
#if DISPLAY_SH1106
    dispCmd(0x02);          // SH1106 RAM is 132 wide; visible area starts at 2
    dispCmd(0x10);
#else
    dispCmd(0x00);
    dispCmd(0x10);
#endif
    uint8_t *p = fb + page * 128;
    for (uint8_t chunk = 0; chunk < 8; chunk++) {
      Wire.beginTransmission(OLED_ADDR);
      Wire.write(0x40);
      Wire.write(p, 16);
      Wire.endTransmission();
      p += 16;
    }
  }
}

static inline void px(int x, int y, bool on) {
  if (x < 0 || x > 127 || y < 0 || y > 63) return;
  uint16_t i = (y >> 3) * 128 + x;
  uint8_t  m = 1 << (y & 7);
  if (on) fb[i] |= m; else fb[i] &= ~m;
}

static void clearFb() { memset(fb, 0, sizeof(fb)); }

static void hLine(int x, int y, int w, bool on) {
  for (int i = 0; i < w; i++) px(x + i, y, on);
}
static void vLine(int x, int y, int h, bool on) {
  for (int i = 0; i < h; i++) px(x, y + i, on);
}
static void fillRect(int x, int y, int w, int h, bool on) {
  for (int j = 0; j < h; j++) hLine(x, y + j, w, on);
}
static void frameRect(int x, int y, int w, int h) {
  hLine(x, y, w, 1); hLine(x, y + h - 1, w, 1);
  vLine(x, y, h, 1); vLine(x + w - 1, y, h, 1);
}
static void invertRect(int x, int y, int w, int h) {
  for (int j = 0; j < h; j++)
    for (int i = 0; i < w; i++) {
      int xx = x + i, yy = y + j;
      if (xx < 0 || xx > 127 || yy < 0 || yy > 63) continue;
      uint16_t k = (yy >> 3) * 128 + xx;
      fb[k] ^= (1 << (yy & 7));
    }
}

static void drawChar(int x, int y, char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c < 32 || c > 95) c = '?';
  const uint8_t *g = font5x7[c - 32];
  for (uint8_t col = 0; col < 5; col++)
    for (uint8_t row = 0; row < 7; row++)
      if ((g[col] >> row) & 1) px(x + col, y + row, 1);
}

static void drawStr(int x, int y, const char *s) {
  while (*s) { drawChar(x, y, *s++); x += 6; }
}
static int strW(const char *s) { return (int)strlen(s) * 6 - 1; }

static void drawStrC(int cx, int y, const char *s) {
  drawStr(cx - strW(s) / 2, y, s);
}

static void drawIcon(int x, int y, const uint16_t *ic) {
  for (uint8_t r = 0; r < 11; r++)
    for (uint8_t c = 0; c < 11; c++)
      if ((ic[r] >> (10 - c)) & 1) px(x + c, y + r, 1);
}

/* ------------------------------- settings -------------------------------- */

#define SET_MAGIC   0xA5C3
#define SET_VERSION 3
#define EE_ADDR     0

enum { IN_MIC = 0, IN_INST = 1 };
enum { METER_WAVE = 0, METER_BARS = 1, METER_SCOPE = 2 };
enum { EDGE_RISE = 0, EDGE_FALL = 1 };
enum { TRIG_AUTO = 0, TRIG_NORMAL = 1 };

struct Settings {
  uint16_t magic;
  uint8_t  version;
  uint8_t  inputL;      // IN_MIC / IN_INST
  uint8_t  inputR;
  uint8_t  hpVol;       // 0..100
  uint8_t  micLevel;    // SGTL5000 lineInLevel 0..15
  uint8_t  instLevel;
  uint8_t  monitor;     // 0/1, master enable for the headphone path
  uint8_t  blend;       // 0 = all live input, 100 = all computer playback
  uint8_t  meterStyle;  // METER_WAVE / METER_BARS / METER_SCOPE
  uint8_t  timebase;    // index into sppTable: 1, 2, 4, 8 samples per pixel
  uint8_t  trigLvl;     // 0..50 % of full scale
  uint8_t  trigEdge;    // EDGE_RISE / EDGE_FALL
  uint8_t  trigMode;    // TRIG_AUTO / TRIG_NORMAL
  uint8_t  clipHold;    // seconds, 1..9
  uint8_t  bootAnim;    // 0/1
  uint8_t  sum;
};

Settings cfg;
bool     cfgDirty = false;
uint32_t cfgTouched = 0;

// Runtime only, never written to EEPROM: a test tone left on across a power
// cycle would be a confusing thing to boot into.
enum { TONE_OFF = 0, TONE_L = 1, TONE_R = 2, TONE_BOTH = 3 };
uint8_t  toneMode = TONE_OFF;
#define TONE_HZ    440.0f
#define TONE_LEVEL 0.5f          // -6 dBFS, comfortably below clipping

// Deliberately takes no arguments: the Arduino IDE hoists auto-generated
// prototypes above the struct definition, so a Settings parameter here would
// fail to compile. Operates on the global cfg instead.
static uint8_t cfgSum() {
  const uint8_t *p = (const uint8_t *)&cfg;
  uint8_t v = 0x5A;
  for (unsigned i = 0; i < sizeof(Settings) - 1; i++) v = (uint8_t)(v * 31 + p[i]);
  return v;
}

static void cfgDefaults() {
  cfg.magic      = SET_MAGIC;
  cfg.version    = SET_VERSION;
  cfg.inputL     = IN_MIC;
  cfg.inputR     = IN_INST;
  cfg.hpVol      = 45;
  cfg.micLevel   = 12;   // mic preamp runs hot-ish; sensitive ADC setting
  cfg.instLevel  = 0;    // guitar buffer is near line level
  cfg.monitor    = 1;
  cfg.blend      = 0;    // tracking default: hear yourself, not the computer
  cfg.meterStyle = METER_WAVE;
  cfg.timebase   = 1;    // 2 samples/px, about 5.8 ms across the screen
  cfg.trigLvl    = 0;    // zero crossing — the right default for audio
  cfg.trigEdge   = EDGE_RISE;
  cfg.trigMode   = TRIG_AUTO;
  cfg.clipHold   = 2;
  cfg.bootAnim   = 1;
}

static void cfgLoad() {
  EEPROM.get(EE_ADDR, cfg);
  if (cfg.magic != SET_MAGIC || cfg.version != SET_VERSION || cfg.sum != cfgSum())
    cfgDefaults();
}

static void cfgTouch() { cfgDirty = true; cfgTouched = millis(); }

static void cfgSaveIfIdle() {
  if (!cfgDirty) return;
  if (millis() - cfgTouched < 2000) return;   // coalesce writes, spare the cells
  cfg.sum = cfgSum();
  EEPROM.put(EE_ADDR, cfg);
  cfgDirty = false;
}

/* --------------------------- hardware application ------------------------ */

static uint8_t levelFor(uint8_t inputType) {
  return (inputType == IN_MIC) ? cfg.micLevel : cfg.instLevel;
}

static void applyAudio() {
  sgtl.volume(cfg.hpVol / 100.0f);
  sgtl.lineInLevel(levelFor(cfg.inputL), levelFor(cfg.inputR));

  // Equal-power crossfade: a straight (1-b, b) fade would dip about 3 dB in
  // the middle, since live input and playback are uncorrelated signals.
  float m = cfg.monitor ? 1.0f : 0.0f;
  float b = cfg.blend / 100.0f;
  float gLive = m * cosf(b * (float)M_PI_2);
  float gPlay = m * sinf(b * (float)M_PI_2);
  monL.gain(0, gLive);
  monR.gain(0, gLive);
  monL.gain(1, gPlay);
  monR.gain(1, gPlay);

  // Tone replaces the live input rather than mixing with it, so what you see
  // on the meters is a known reference and nothing else. Routed per channel:
  // sending it to one side only makes crosstalk and swapped wiring obvious.
  bool tL = (toneMode == TONE_L) || (toneMode == TONE_BOTH);
  bool tR = (toneMode == TONE_R) || (toneMode == TONE_BOTH);
  srcL.gain(0, tL ? 0.0f : 1.0f);
  srcL.gain(1, tL ? 1.0f : 0.0f);
  srcR.gain(0, tR ? 0.0f : 1.0f);
  srcR.gain(1, tR ? 1.0f : 0.0f);
}

// Muted switchover: the analog switch will click no matter what, so drop the
// headphone amp first and let it recover after the relay/switch has settled.
static void applyRouting(bool muteRamp) {
  if (muteRamp) { sgtl.volume(0.0f); delay(30); }
  digitalWriteFast(PIN_SEL_L, cfg.inputL == IN_INST);
  digitalWriteFast(PIN_SEL_R, cfg.inputR == IN_INST);
  if (muteRamp) delay(60);
  applyAudio();
}

/* -------------------------- encoder + pushbutton ------------------------- */
/* Sampled by an IntervalTimer at 2 kHz, NOT from loop(). dispFlush() blocks
   for ~26 ms at a time, so loop()-rate polling misses most detents.        */

static const int8_t encTab[16] = {
  0,-1, 1, 0,  1, 0, 0,-1, -1, 0, 0, 1,  0, 1,-1, 0
};

IntervalTimer inputTimer;

volatile uint8_t encState = 0;
volatile int8_t  encAcc   = 0;
volatile int8_t  encDelta = 0;

volatile bool     btnLevel = true, btnStable = true;
volatile uint32_t btnEdge = 0, btnDown = 0;
volatile bool     btnShort = false, btnLong = false, btnLongFired = false;

static void inputPoll() {
  uint8_t s = (digitalReadFast(PIN_ENC_CLK) << 1) | digitalReadFast(PIN_ENC_DT);
  if (s != encState) {
    encAcc += encTab[(encState << 2) | s];
    encState = s;
    if (encAcc >= 4)       { encDelta += ENC_REVERSE ? -1 : 1; encAcc = 0; }
    else if (encAcc <= -4) { encDelta += ENC_REVERSE ?  1 : -1; encAcc = 0; }
    if (encDelta >  24) encDelta =  24;    // don't wrap on a fast spin
    if (encDelta < -24) encDelta = -24;
  }

  bool raw = digitalReadFast(PIN_ENC_SW);
  uint32_t now = millis();
  if (raw != btnLevel) { btnLevel = raw; btnEdge = now; }
  if (now - btnEdge > 25 && btnStable != btnLevel) {
    btnStable = btnLevel;
    if (!btnStable) { btnDown = now; btnLongFired = false; }
    else if (!btnLongFired) btnShort = true;
  }
  if (!btnStable && !btnLongFired && now - btnDown > 650) {
    btnLong = true; btnLongFired = true;
  }
}

static int8_t takeDelta() {
  noInterrupts(); int8_t d = encDelta; encDelta = 0; interrupts(); return d;
}
static bool tookShort() {
  noInterrupts(); bool b = btnShort; btnShort = false; interrupts(); return b;
}
static bool tookLong() {
  noInterrupts(); bool b = btnLong;  btnLong  = false; interrupts(); return b;
}

/* --------------------------------- meters -------------------------------- */

uint8_t  histL[128], histR[128];
uint8_t  histPbL[128], histPbR[128];
uint8_t  histHead = 0;
float    curL = 0, curR = 0;
float    curPbL = 0, curPbR = 0;
uint32_t clipUntilL = 0, clipUntilR = 0;
uint32_t meterTick = 0;

static void meterUpdate() {
  if (millis() - meterTick < 25) return;
  meterTick = millis();

  if (peakL.available())   curL   = peakL.read();
  if (peakR.available())   curR   = peakR.read();
  if (peakPbL.available()) curPbL = peakPbL.read();
  if (peakPbR.available()) curPbR = peakPbR.read();

  // Clip detection stays on the record path only — playback is somebody
  // else's problem and must never trip the recording clip warning.
  if (curL >= 0.99f) clipUntilL = millis() + cfg.clipHold * 1000UL;
  if (curR >= 0.99f) clipUntilR = millis() + cfg.clipHold * 1000UL;

  histL[histHead]   = (uint8_t)(curL   * 255.0f);
  histR[histHead]   = (uint8_t)(curR   * 255.0f);
  histPbL[histHead] = (uint8_t)(curPbL * 255.0f);
  histPbR[histHead] = (uint8_t)(curPbR * 255.0f);
  histHead = (histHead + 1) & 127;
}

static int dbOf(float p) {
  if (p <= 0.0002f) return -99;
  int d = (int)lroundf(20.0f * log10f(p));
  if (d < -99) d = -99;
  if (d > 0) d = 0;
  return d;
}

static void drawWaveLane(int cy, const uint8_t *hist, const uint8_t *over) {
  for (int x = 0; x < 128; x += 4) px(x, cy, 1);
  for (int x = 0; x < 128; x++) {
    uint8_t v = hist[(histHead + x) & 127];
    int h = (v * 7) / 255;
    vLine(x, cy - h, 2 * h + 1, 1);
  }
  // Playback envelope: outline only, dashed, so it reads as a separate layer
  // sitting over the solid record-path waveform rather than merging with it.
  if (over) {
    for (int x = 0; x < 128; x += 2) {
      uint8_t v = over[(histHead + x) & 127];
      int h = (v * 7) / 255;
      px(x, cy - h, 1);
      px(x, cy + h, 1);
    }
  }
}

static void drawBarLane(int cy, float lvl, const char *tag) {
  drawStr(0, cy - 3, tag);
  int x0 = 12, w = 116;
  frameRect(x0, cy - 5, w, 11);
  int fillw = (int)(lvl * (w - 4));
  if (fillw > w - 4) fillw = w - 4;
  fillRect(x0 + 2, cy - 3, fillw, 7, 1);
  int mark = x0 + 2 + (int)(0.708f * (w - 4));   // -3 dBFS tick
  vLine(mark, cy - 5, 11, 1);
}

/* ---------------------------------- scope -------------------------------- */
/* Raw samples land in a ring per channel. Each frame we hunt backwards for a
   level crossing and draw 128 columns forward from it, so successive frames
   start at the same point in the waveform and the trace appears to stand
   still. Columns use min/max decimation, not point sampling, so a fast wave
   at a slow timebase shows as a solid band instead of aliasing into lies.  */

#define SCOPE_BUF 2048                          // power of two, masked indexing
int16_t  scopeL[SCOPE_BUF], scopeR[SCOPE_BUF];
uint16_t scopeHead = 0;

static const uint8_t     sppTable[4] = { 1, 2, 4, 8 };
static const char *const tbNames[4]  = { "2.9MS", "5.8MS", "11.6MS", "23.2MS" };

struct Lane { int8_t top[128], bot[128]; bool valid; };
Lane laneL, laneR;

static void scopeDrain() {
  // Both channels advance together so the two rings stay index-aligned.
  while (qL.available() > 0 && qR.available() > 0) {
    int16_t *bL = qL.readBuffer();
    int16_t *bR = qR.readBuffer();
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
      scopeL[scopeHead] = bL[i];
      scopeR[scopeHead] = bR[i];
      scopeHead = (scopeHead + 1) & (SCOPE_BUF - 1);
    }
    qL.freeBuffer();
    qR.freeBuffer();
  }
  // If one side ever runs away, dump the excess rather than stalling forever.
  while (qL.available() > 40) { qL.readBuffer(); qL.freeBuffer(); }
  while (qR.available() > 40) { qR.readBuffer(); qR.freeBuffer(); }
}

static int scopeTrigger(const int16_t *buf, int need) {
  int lvl = (int)cfg.trigLvl * 300;             // 0..50 -> 0..15000
  int searchLen = SCOPE_BUF - need - 8;
  if (searchLen < 1) return -1;
  int base = ((int)scopeHead - need + SCOPE_BUF) & (SCOPE_BUF - 1);
  bool rise = (cfg.trigEdge == EDGE_RISE);
  for (int k = 0; k < searchLen; k++) {         // newest candidate first
    int i  = (base - k + SCOPE_BUF) & (SCOPE_BUF - 1);
    int im = (i - 1 + SCOPE_BUF) & (SCOPE_BUF - 1);
    int a = buf[im], b = buf[i];
    if (rise ? (a < lvl && b >= lvl) : (a > lvl && b <= lvl)) return i;
  }
  return -1;
}

static inline int scopeY(int s, int half) {
  int y = -(s * half) / 32768;                  // positive sample -> up
  if (y < -half) y = -half;
  if (y >  half) y =  half;
  return y;
}

// These take a channel index (0 = L, 1 = R) rather than a Lane reference:
// the IDE hoists prototypes above the struct, so a Lane parameter won't build.
static void scopeCompute(int ch, int spp, int half) {
  Lane &ln = ch ? laneR : laneL;
  const int16_t *buf = ch ? scopeR : scopeL;
  int need = 128 * spp;
  int t = scopeTrigger(buf, need);
  if (t < 0) {
    // TRIG_NORMAL holds the last good frame; TRIG_AUTO free-runs so that a
    // silent input draws a flat line instead of a frozen stale one.
    if (cfg.trigMode == TRIG_NORMAL && ln.valid) return;
    t = ((int)scopeHead - need + SCOPE_BUF) & (SCOPE_BUF - 1);
  }
  int prev = 0;
  for (int x = 0; x < 128; x++) {
    int mn = 32767, mx = -32768, last = 0;
    for (int k = 0; k < spp; k++) {
      int s = buf[(t + x * spp + k) & (SCOPE_BUF - 1)];
      if (s < mn) mn = s;
      if (s > mx) mx = s;
      last = s;
    }
    int a = scopeY(mx, half);                   // top of the column
    int b = scopeY(mn, half);                   // bottom
    if (prev < a) a = prev;                     // bridge to the previous
    if (prev > b) b = prev;                     // column so the trace joins up
    ln.top[x] = (int8_t)a;
    ln.bot[x] = (int8_t)b;
    prev = scopeY(last, half);
  }
  ln.valid = true;
}

static void drawScopeLane(int cy, int half, int ch) {
  const Lane &ln = ch ? laneR : laneL;
  for (int x = 0; x < 128; x += 4) px(x, cy, 1);
  if (!ln.valid) return;
  for (int x = 0; x < 128; x++)
    vLine(x, cy + ln.top[x], ln.bot[x] - ln.top[x] + 1, 1);
}

/* --------------------------------- UI state ------------------------------ */

enum Screen { SCR_BOOT, SCR_HOME, SCR_INPUT_L, SCR_INPUT_R, SCR_SETTINGS };
Screen  screen   = SCR_BOOT;
int     homeSel  = 0;      // 0=L 1=R 2=gear 3=volume
bool    editing  = false;
int     subSel   = 0;
int     setSel   = 0, setTop = 0;
uint32_t bootStart = 0;

enum {
  S_MICGAIN = 0, S_INSTGAIN, S_MONITOR, S_BLEND, S_METER,
  S_TIMEBASE, S_TRIGLVL, S_TRIGEDGE, S_TRIGMODE,
  S_CLIPHOLD, S_BOOTANIM, S_TESTTONE, S_DEFAULTS, S_BACK,
  N_SETTINGS
};

static const char *setNames[N_SETTINGS] = {
  "MIC GAIN", "INST GAIN", "MONITOR", "MON BLEND", "METER",
  "TIMEBASE", "TRIG LVL", "TRIG EDGE", "TRIG MODE",
  "CLIP HOLD", "BOOT ANIM", "TEST TONE", "DEFAULTS", "BACK"
};

// Scope controls are meaningless in WAVE and BARS, so they're hidden there.
// The values stay in EEPROM either way — hiding is purely a UI concern.
static bool setVisible(int i) {
  if (i >= S_TIMEBASE && i <= S_TRIGMODE) return cfg.meterStyle == METER_SCOPE;
  return true;
}

int vis[N_SETTINGS];
int nVis = 0;

static void buildVis() {
  nVis = 0;
  for (int i = 0; i < N_SETTINGS; i++) if (setVisible(i)) vis[nVis++] = i;
}
static int visPos(int item) {
  for (int p = 0; p < nVis; p++) if (vis[p] == item) return p;
  return 0;
}

static int setGet(int i) {
  switch (i) {
    case S_MICGAIN:  return cfg.micLevel;
    case S_INSTGAIN: return cfg.instLevel;
    case S_MONITOR:  return cfg.monitor;
    case S_BLEND:    return cfg.blend;
    case S_METER:    return cfg.meterStyle;
    case S_TIMEBASE: return cfg.timebase;
    case S_TRIGLVL:  return cfg.trigLvl;
    case S_TRIGEDGE: return cfg.trigEdge;
    case S_TRIGMODE: return cfg.trigMode;
    case S_CLIPHOLD: return cfg.clipHold;
    case S_BOOTANIM: return cfg.bootAnim;
    case S_TESTTONE: return toneMode;
  }
  return 0;
}
static int setMin(int i) { return (i == S_CLIPHOLD) ? 1 : 0; }
static int setMax(int i) {
  switch (i) {
    case S_MICGAIN: case S_INSTGAIN: return 15;
    case S_MONITOR: case S_BOOTANIM: case S_TRIGEDGE: case S_TRIGMODE: return 1;
    case S_BLEND:    return 100;
    case S_METER:    return 2;
    case S_TIMEBASE: return 3;
    case S_TRIGLVL:  return 50;
    case S_CLIPHOLD: return 9;
    case S_TESTTONE: return 3;
  }
  return 0;
}
static int setStep(int i) {
  if (i == S_BLEND)   return 5;
  if (i == S_TRIGLVL) return 5;
  return 1;
}
static bool setIsAction(int i) { return i >= S_DEFAULTS; }

static void setPut(int i, int v) {
  if (i == S_TESTTONE) { toneMode = v; applyAudio(); return; }  // never saved
  switch (i) {
    case S_MICGAIN:  cfg.micLevel   = v; break;
    case S_INSTGAIN: cfg.instLevel  = v; break;
    case S_MONITOR:  cfg.monitor    = v; break;
    case S_BLEND:    cfg.blend      = v; break;
    case S_METER:    cfg.meterStyle = v; laneL.valid = laneR.valid = false; break;
    case S_TIMEBASE: cfg.timebase   = v; laneL.valid = laneR.valid = false; break;
    case S_TRIGLVL:  cfg.trigLvl    = v; break;
    case S_TRIGEDGE: cfg.trigEdge   = v; break;
    case S_TRIGMODE: cfg.trigMode   = v; break;
    case S_CLIPHOLD: cfg.clipHold   = v; break;
    case S_BOOTANIM: cfg.bootAnim   = v; break;
  }
  cfgTouch();
  applyAudio();
}

static void setValStr(int i, char *out) {
  switch (i) {
    case S_MICGAIN: case S_INSTGAIN: sprintf(out, "%d", setGet(i)); break;
    case S_MONITOR: case S_BOOTANIM:
      strcpy(out, setGet(i) ? "ON" : "OFF"); break;
    case S_TESTTONE:
      strcpy(out, toneMode == TONE_OFF ? "OFF" :
                  toneMode == TONE_L   ? "L"   :
                  toneMode == TONE_R   ? "R"   : "BOTH");
      break;
    case S_BLEND:
      if (cfg.blend == 0)        strcpy(out, "LIVE");
      else if (cfg.blend == 100) strcpy(out, "PLAY");
      else sprintf(out, "%d%%", cfg.blend);
      break;
    case S_METER:
      strcpy(out, cfg.meterStyle == METER_WAVE ? "WAVE" :
                  cfg.meterStyle == METER_BARS ? "BARS" : "SCOPE");
      break;
    case S_TIMEBASE: strcpy(out, tbNames[cfg.timebase]); break;
    case S_TRIGLVL:
      if (cfg.trigLvl == 0) strcpy(out, "ZERO");
      else sprintf(out, "%d%%", cfg.trigLvl);
      break;
    case S_TRIGEDGE: strcpy(out, cfg.trigEdge == EDGE_RISE ? "RISE" : "FALL"); break;
    case S_TRIGMODE: strcpy(out, cfg.trigMode == TRIG_AUTO ? "AUTO" : "NORM"); break;
    case S_CLIPHOLD: sprintf(out, "%dS", cfg.clipHold); break;
    default: out[0] = 0;
  }
}

/* -------------------------------- rendering ------------------------------ */

static const int cellX[4] = {  0, 37, 74, 89 };
static const int cellW[4] = { 36, 36, 14, 39 };

static void drawHome() {
  char buf[24];

  sprintf(buf, "L:%s", cfg.inputL == IN_MIC ? "MIC " : "INST");
  drawStr(cellX[0] + 1, 2, buf);
  sprintf(buf, "R:%s", cfg.inputR == IN_MIC ? "MIC " : "INST");
  drawStr(cellX[1] + 1, 2, buf);
  drawIcon(cellX[2] + 2, 0, icoGear);
  drawIcon(cellX[3] + 1, 0, icoPhones);
  sprintf(buf, "%3d", cfg.hpVol);
  drawStr(cellX[3] + 15, 2, buf);

  invertRect(cellX[homeSel], 0, cellW[homeSel], 11);
  if (editing && (millis() >> 8) & 1)
    invertRect(cellX[homeSel], 0, cellW[homeSel], 11);

  hLine(0, 12, 128, 1);

  if (cfg.meterStyle == METER_WAVE) {
    const uint8_t *pbL = cfg.blend ? histPbL : NULL;
    const uint8_t *pbR = cfg.blend ? histPbR : NULL;
    drawWaveLane(21, histL, pbL);
    drawWaveLane(38, histR, pbR);
    drawStr(0, 15, "L");
    drawStr(0, 32, "R");
  } else if (cfg.meterStyle == METER_SCOPE) {
    int spp = sppTable[cfg.timebase];
    scopeCompute(0, spp, 7);
    scopeCompute(1, spp, 7);
    drawScopeLane(21, 7, 0);
    drawScopeLane(38, 7, 1);
  } else {
    drawBarLane(21, curL, "L");
    drawBarLane(38, curR, "R");
  }

  hLine(0, 47, 128, 1);

  bool clipping = (millis() < clipUntilL) || (millis() < clipUntilR);
  if (clipping) {
    fillRect(0, 50, 128, 13, 1);
    invertRect(0, 50, 128, 13);
    drawStrC(64, 53, "CLIPPING DETECTED");
    invertRect(0, 50, 128, 13);
  } else if (toneMode != TONE_OFF) {
    fillRect(0, 50, 128, 13, 1);
    invertRect(0, 50, 128, 13);
    const char *tag = (toneMode == TONE_L) ? "440-L" :
                      (toneMode == TONE_R) ? "440-R" : "440-LR";
    sprintf(buf, "%s L%3d R%3d", tag, dbOf(curL), dbOf(curR));
    drawStrC(64, 53, buf);
    invertRect(0, 50, 128, 13);
  } else {
    sprintf(buf, "L%3d  R%3d  DBFS", dbOf(curL), dbOf(curR));
    drawStrC(64, 53, buf);
  }

  if (editing && homeSel == 3) {          // volume overlay on the right edge
    fillRect(115, 11, 13, 39, 0);         // punch a black margin through the
    frameRect(118, 13, 10, 35);           // waveform so the bar reads clearly
    int h = (cfg.hpVol * 31) / 100;
    fillRect(120, 46 - h, 6, h, 1);
  }
}

static void drawInputMenu(bool isLeft) {
  uint8_t cur = isLeft ? cfg.inputL : cfg.inputR;
  drawStrC(64, 2, isLeft ? "LEFT INPUT" : "RIGHT INPUT");
  hLine(0, 12, 128, 1);

  drawIcon(6, 16, icoMic);
  drawStr(24, 19, "MIC");
  if (cur == IN_MIC) drawStr(110, 19, "*");

  drawIcon(6, 32, icoPlug);
  drawStr(24, 35, "INSTRUMENT");
  if (cur == IN_INST) drawStr(110, 35, "*");

  drawStr(6, 52, "< BACK");

  int y = (subSel == 0) ? 15 : (subSel == 1) ? 31 : 50;
  int h = (subSel == 2) ? 11 : 13;
  invertRect(0, y, 128, h);
}

static void drawSettings() {
  char val[10];
  drawStrC(64, 2, "SETTINGS");
  hLine(0, 12, 128, 1);

  if (setSel < setTop) setTop = setSel;
  if (setSel > setTop + 3) setTop = setSel - 3;

  for (int r = 0; r < 4; r++) {
    int p = setTop + r;
    if (p >= nVis) break;
    int i = vis[p];
    int y = 16 + r * 12;
    drawStr(4, y, setNames[i]);
    if (!setIsAction(i)) {
      setValStr(i, val);
      drawStr(124 - strW(val), y, val);
    }
    if (p == setSel) {
      invertRect(0, y - 2, 128, 11);
      if (editing && (millis() >> 8) & 1) invertRect(0, y - 2, 128, 11);
    }
  }

  if (nVis > 4) {                          // scrollbar
    vLine(127, 14, 48, 1);
    int barH = 48 * 4 / nVis;
    int barY = 14 + (48 - barH) * setTop / (nVis - 4);
    fillRect(125, barY, 3, barH, 1);
  }
}

/* ----------------------------- startup screen ---------------------------- */
/* A waveform sweeps in, collapses to a line, the name resolves out of it,
   then a scan bar wipes the screen. Press the knob to skip.                 */

static void drawBoot(uint32_t t) {
  clearFb();

  if (t < 900) {
    // a waveform sweeps in from the left, amplitude growing behind it
    float grow = t / 900.0f;
    int sweep = (int)(128 * grow);
    for (int x = 0; x < sweep; x++) {
      float env = grow * (0.25f + 0.75f * (float)x / 128.0f);
      float a = sinf((x * 0.22f) - t * 0.012f) * 26.0f * env;
      px(x, 32 + (int)a, 1);
      px(x, 32 - (int)a, 1);
    }
  } else if (t < 2000) {
    // the wave collapses toward the centre while the name types out over it
    float k = (t - 900) / 1100.0f;
    float shrink = 1.0f - k;
    for (int x = 0; x < 128; x++) {
      float a = sinf((x * 0.22f) - t * 0.012f) * 26.0f * shrink;
      px(x, 32 + (int)a, 1);
      px(x, 32 - (int)a, 1);
    }
    int len1 = strlen(DEVICE_L1), len2 = strlen(DEVICE_L2);
    int n = (int)(k * (len1 + len2) * 1.5f);
    if (n > len1 + len2) n = len1 + len2;
    int n1 = (n < len1) ? n : len1;
    int n2 = n - n1;
    char b1[20], b2[20];
    memcpy(b1, DEVICE_L1, n1); b1[n1] = 0;
    memcpy(b2, DEVICE_L2, n2); b2[n2] = 0;
    if (n1) { int w = strW(b1); fillRect(64 - w/2 - 3, 20, w + 6, 11, 0); drawStrC(64, 22, b1); }
    if (n2) { int w = strW(b2); fillRect(64 - w/2 - 3, 32, w + 6, 11, 0); drawStrC(64, 34, b2); }
  } else {
    drawStrC(64, 17, DEVICE_L1);
    drawStrC(64, 28, DEVICE_L2);
    hLine(14, 40, 100, 1);
    drawStrC(64, 46, DEVICE_SUB);
    int scan = (int)((t - 2000) / 5);
    if (scan < 128) { vLine(scan, 0, 64, 1); vLine(scan + 1, 0, 64, 1); }
  }
  dispFlush();
}

/* --------------------------------- input --------------------------------- */

static void handleHome(int8_t d, bool sc, bool lg) {
  if (editing) {
    if (d) {
      int v = cfg.hpVol + d * 2;
      cfg.hpVol = (uint8_t)constrain(v, 0, 100);
      applyAudio();
      cfgTouch();
    }
    if (sc || lg) editing = false;
    return;
  }
  if (d) homeSel = (homeSel + d + 4) & 3;
  if (sc) {
    switch (homeSel) {
      case 0: screen = SCR_INPUT_L; subSel = cfg.inputL; break;
      case 1: screen = SCR_INPUT_R; subSel = cfg.inputR; break;
      case 2: screen = SCR_SETTINGS; buildVis(); setSel = 0; setTop = 0; break;
      case 3: editing = true; break;
    }
  }
}

static void handleInput(bool isLeft, int8_t d, bool sc, bool lg) {
  if (d) subSel = (subSel + d + 3) % 3;
  if (lg) { screen = SCR_HOME; return; }
  if (sc) {
    if (subSel == 2) { screen = SCR_HOME; return; }
    uint8_t v = (subSel == 0) ? IN_MIC : IN_INST;
    if (isLeft) cfg.inputL = v; else cfg.inputR = v;
    cfgTouch();
    applyRouting(true);
    screen = SCR_HOME;
  }
}

static void handleSettings(int8_t d, bool sc, bool lg) {
  int item = vis[setSel];

  if (editing) {
    if (d) {
      int v = setGet(item) + d * setStep(item);
      setPut(item, constrain(v, setMin(item), setMax(item)));
      if (item == S_METER) {          // scope rows appear/vanish under you
        buildVis();
        setSel = visPos(S_METER);
        setTop = constrain(setTop, 0, nVis > 4 ? nVis - 4 : 0);
      }
    }
    if (sc || lg) editing = false;
    return;
  }

  if (lg) { screen = SCR_HOME; return; }
  if (d) setSel = constrain(setSel + d, 0, nVis - 1);

  if (sc) {
    item = vis[setSel];
    if (item == S_BACK) { screen = SCR_HOME; }
    else if (item == S_DEFAULTS) {
      uint8_t kl = cfg.inputL, kr = cfg.inputR;
      cfgDefaults();
      cfg.inputL = kl; cfg.inputR = kr;   // don't yank routing out from under them
      cfgTouch();
      applyRouting(false);
      laneL.valid = laneR.valid = false;
      buildVis();
      setSel = constrain(setSel, 0, nVis - 1);
    } else editing = true;
  }
}

/* ---------------------------------- main --------------------------------- */

void setup() {
  pinMode(PIN_ENC_CLK, INPUT_PULLUP);
  pinMode(PIN_ENC_DT,  INPUT_PULLUP);
  pinMode(PIN_ENC_SW,  INPUT_PULLUP);
  pinMode(PIN_SEL_L,   OUTPUT);
  pinMode(PIN_SEL_R,   OUTPUT);

  Wire.begin();
  Wire.setClock(1000000);   // 1 MHz halves the flush blackout; drop to 400000
                            // if the panel glitches on long/unshielded leads

  AudioMemory(60);
  sgtl.enable();
  sgtl.inputSelect(AUDIO_INPUT_LINEIN);
  testTone.frequency(TONE_HZ);
  testTone.amplitude(TONE_LEVEL);
  qL.begin();
  qR.begin();

  cfgLoad();
  buildVis();
  applyRouting(false);

  dispInit();
  clearFb();
  dispFlush();

  encState = (digitalReadFast(PIN_ENC_CLK) << 1) | digitalReadFast(PIN_ENC_DT);
  inputTimer.begin(inputPoll, 500);        // 2 kHz, independent of loop()
  inputTimer.priority(64);                 // above default, below the audio DMA

  bootStart = millis();
  screen = cfg.bootAnim ? SCR_BOOT : SCR_HOME;
}

void loop() {
  scopeDrain();
  meterUpdate();
  cfgSaveIfIdle();

  if (screen == SCR_BOOT) {
    uint32_t t = millis() - bootStart;
    if (t > 2900 || tookShort() || tookLong()) { screen = SCR_HOME; takeDelta(); }
    else {
      static uint32_t lastBoot = 0;
      if (millis() - lastBoot >= 33) { lastBoot = millis(); drawBoot(t); }
      return;
    }
  }

  int8_t d  = takeDelta();
  bool   sc = tookShort();
  bool   lg = tookLong();

  switch (screen) {
    case SCR_HOME:     handleHome(d, sc, lg);            break;
    case SCR_INPUT_L:  handleInput(true,  d, sc, lg);    break;
    case SCR_INPUT_R:  handleInput(false, d, sc, lg);    break;
    case SCR_SETTINGS: handleSettings(d, sc, lg);        break;
    default: break;
  }

  static uint32_t lastDraw = 0;
  if (millis() - lastDraw < 33) return;      // ~30 fps
  lastDraw = millis();

  clearFb();
  switch (screen) {
    case SCR_HOME:     drawHome();             break;
    case SCR_INPUT_L:  drawInputMenu(true);    break;
    case SCR_INPUT_R:  drawInputMenu(false);   break;
    case SCR_SETTINGS: drawSettings();         break;
    default: break;
  }
  dispFlush();
}

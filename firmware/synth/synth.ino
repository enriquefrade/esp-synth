// ESP32-S3-RLCD-4.2 4-oscillator synth.
// Controls, sync, and display behavior are described in docs/controls-and-sync.md
// and the project description in description.md.
// Serial (USB CDC, 115200): type "help" for test commands.

#include <Arduino.h>
#include <math.h>
#include <stdio.h>

#include "audio.h"
#include "cmd.h"
#include "config.h"
#include "controls.h"
#include "display.h"
#include "engine.h"
#include "logic.h"

namespace {

synth::Engine g_engine(SAMPLE_RATE);

logic::Button g_btnKey;
logic::Button g_btnBoot;
logic::PotFilter g_potFilter[3];

// Pot memory per voice: [voice][0 = frequency, 1 = level, 2 = detune], 12-bit raw.
uint16_t g_potMem[synth::kVoices][3];
// A pot only controls the selected voice after it has moved away from the
// voice's stored value (soft takeover). Avoids jumps when switching voices.
bool g_armed[3] = {false, false, false};
constexpr uint16_t kTakeoverDelta = 80;
constexpr uint16_t kApplyDelta = 3;

const int kPotPins[3] = {PIN_POT_FREQ, PIN_POT_LEVEL, PIN_POT_DETUNE};

// Serial overrides. When set, the pot for that parameter is ignored for the voice
// until the pot moves (or `free` is sent).
struct Override {
  bool hz = false;
  float hzValue = 0.0f;
  bool level = false;
  float levelValue = 0.0f;
  bool detune = false;
  float cents = 0.0f;
};
Override g_ovr[synth::kVoices];

int g_selected = 0;
bool g_hzMode = false;
bool g_dirty = true;

void updateVoice(int v) {
  const Override &o = g_ovr[v];
  float hz;
  if (o.hz) {
    hz = o.hzValue;
  } else if (controls::syncMode() == controls::SyncMode::Tempo) {
    float ratio = logic::kTempoRatios[logic::potToRatioIndex(g_potMem[v][0])];
    hz = controls::clockHz() * ratio;
  } else if (g_hzMode) {
    hz = logic::potToHz(g_potMem[v][0]);
  } else {
    hz = synth::Engine::noteToHz(logic::potToNote(g_potMem[v][0]));
  }

  float cents = o.detune ? o.cents : logic::potToDetune(g_potMem[v][2]);
  hz *= powf(2.0f, cents / 1200.0f);

  g_engine.setFrequency(v, hz);
  g_engine.setLevel(v, o.level ? o.levelValue : logic::potToLevel(g_potMem[v][1]));
}

void updateAllVoices() {
  for (int v = 0; v < synth::kVoices; v++) {
    updateVoice(v);
  }
}

// Reads the three pots and applies changes to the selected voice.
void applyPots() {
  for (int p = 0; p < 3; p++) {
    uint16_t raw = g_potFilter[p].update(controls::readPot(kPotPins[p]));
    uint16_t &stored = g_potMem[g_selected][p];

    // Pickup: the pot takes over once it comes within kTakeoverDelta of the
    // stored value. Until then the voice keeps its current value.
    if (!g_armed[p]) {
      int diff = (int)raw - (int)stored;
      if (diff < 0) diff = -diff;
      if (diff >= kTakeoverDelta) continue;
      g_armed[p] = true;
    }

    int diff = (int)raw - (int)stored;
    if (diff < 0) diff = -diff;
    if (diff >= kApplyDelta) {
      stored = raw;
      // Moving the pot takes control back from a serial override.
      Override &o = g_ovr[g_selected];
      if (p == 0) o.hz = false;
      if (p == 1) o.level = false;
      if (p == 2) o.detune = false;
      g_dirty = true;
    }
  }
}

void disarmPots() {
  for (int p = 0; p < 3; p++) {
    g_armed[p] = false;
  }
}

void selectVoice(int v) {
  g_selected = v;
  disarmPots();
  g_dirty = true;
}

void selectNextVoice() {
  selectVoice((g_selected + 1) % synth::kVoices);
}

void toggleActive() {
  bool on = !g_engine.voice(g_selected).active;
  g_engine.setActive(g_selected, on);
  g_dirty = true;
}

void cycleWave() {
  int next = ((int)g_engine.voice(g_selected).wave + 1) % (int)synth::Wave::Count;
  g_engine.setWave(g_selected, (synth::Wave)next);
  g_dirty = true;
}

void toggleSyncMode() {
  controls::SyncMode next = controls::syncMode() == controls::SyncMode::Phase
                                ? controls::SyncMode::Tempo
                                : controls::SyncMode::Phase;
  controls::setSyncMode(next);
  g_dirty = true;
}

void handleButtons(uint32_t now) {
  logic::Button::Event bootEv = g_btnBoot.update(controls::readButton(PIN_BTN_BOOT), now);
  logic::Button::Event keyEv = g_btnKey.update(controls::readButton(PIN_BTN_KEY), now);

  if (bootEv == logic::Button::Event::Click) {
    if (g_btnKey.isDown()) {
      // KEY held + BOOT tap = sync mode. Suppress the KEY click on release.
      toggleSyncMode();
      g_btnKey.suppressClick();
    } else {
      cycleWave();
    }
  } else if (bootEv == logic::Button::Event::Long) {
    g_hzMode = !g_hzMode;
    g_dirty = true;
  }

  // Ignore KEY long-press while BOOT is held, so combos don't toggle the voice.
  if (keyEv == logic::Button::Event::Click) {
    selectNextVoice();
  } else if (keyEv == logic::Button::Event::Long && !g_btnBoot.isDown()) {
    toggleActive();
  }
}

// ---- Serial test interface -------------------------------------------------

void printHelp() {
  Serial.println("commands (voices 1-4):");
  Serial.println("  status                      show all voices and modes");
  Serial.println("  pots                        raw pot readings (0-4095)");
  Serial.println("  audio                       codec output stats");
  Serial.println("  codec                       ES8311 registers and amp enable pin");
  Serial.println("  on <v> / off <v>            enable or silence a voice");
  Serial.println("  wave <v> saw|square|tri     set waveform");
  Serial.println("  freq <v> <hz>               set frequency, overrides pot 1");
  Serial.println("  note <v> <midi 0-127>       set note, overrides pot 1");
  Serial.println("  level <v> <0-1>             set level, overrides pot 2");
  Serial.println("  detune <v> <cents>          set detune, overrides pot 3");
  Serial.println("  free <v>                    clear overrides, pots control again");
  Serial.println("  sel <v>                     select voice (pots follow it)");
  Serial.println("  sync phase|tempo            sync mode");
  Serial.println("  pitch notes|hz              pot 1 unit in pot mode");
  Serial.println("  help");
}

void printStatus() {
  Serial.printf("selected=%d sync=%s pitch=%s clock=%.2fHz\n", g_selected + 1,
                controls::syncMode() == controls::SyncMode::Tempo ? "tempo" : "phase",
                g_hzMode ? "hz" : "notes", controls::clockHz());
  for (int v = 0; v < synth::kVoices; v++) {
    const synth::Voice &voice = g_engine.voice(v);
    const Override &o = g_ovr[v];
    Serial.printf("v%d %s %s hz=%.2f level=%.2f detune=%+.0fc%s%s%s\n", v + 1,
                  voice.active ? "on " : "off", synth::Engine::waveName(voice.wave), voice.hz,
                  voice.level,
                  o.detune ? o.cents : logic::potToDetune(g_potMem[v][2]),
                  o.hz ? " [freq ovr]" : "", o.level ? " [level ovr]" : "",
                  o.detune ? " [detune ovr]" : "");
  }
}

void runCommand(const char *line) {
  const char *error = nullptr;
  cmd::Command c = cmd::parse(line, &error);

  switch (c.type) {
    case cmd::Type::None:
      return;
    case cmd::Type::Invalid:
      Serial.printf("ERR %s\n", error ? error : "bad command");
      return;
    case cmd::Type::Help:
      printHelp();
      return;
    case cmd::Type::Status:
      printStatus();
      return;
    case cmd::Type::Audio: {
      AudioStats st = audioStats();
      Serial.printf("audio blocks=%lu lastWriteRet=%d peak=%d (of 32767)\n", (unsigned long)st.blocks,
                    st.lastWriteRet, st.peak);
      return;
    }
    case cmd::Type::Codec: {
      // PA_CTRL is GPIO46 on this board (schematic). Reading an output pin returns its level.
      Serial.printf("PA_CTRL GPIO%d level=%d\n", PIN_PA_CTRL, digitalRead(PIN_PA_CTRL));
      static const uint8_t regs[] = {0x00, 0x01, 0x02, 0x0D, 0x0E, 0x12, 0x14, 0x17,
                                     0x31, 0x32, 0x37, 0x45};
      for (uint8_t r : regs) {
        uint8_t val = 0;
        if (audioReadCodecReg(r, &val)) {
          Serial.printf("ES8311 reg 0x%02X = 0x%02X\n", r, val);
        } else {
          Serial.printf("ES8311 reg 0x%02X read failed\n", r);
        }
      }
      return;
    }
    case cmd::Type::Pots:
      Serial.printf("pot1(GPIO%d)=%u pot2(GPIO%d)=%u pot3(GPIO%d)=%u\n", PIN_POT_FREQ,
                    controls::readPot(PIN_POT_FREQ), PIN_POT_LEVEL,
                    controls::readPot(PIN_POT_LEVEL), PIN_POT_DETUNE,
                    controls::readPot(PIN_POT_DETUNE));
      return;
    case cmd::Type::On:
      g_engine.setActive(c.voice, true);
      break;
    case cmd::Type::Off:
      g_engine.setActive(c.voice, false);
      break;
    case cmd::Type::Wave:
      g_engine.setWave(c.voice, (synth::Wave)c.wave);
      break;
    case cmd::Type::Freq:
      g_ovr[c.voice].hz = true;
      g_ovr[c.voice].hzValue = c.value;
      break;
    case cmd::Type::Note:
      g_ovr[c.voice].hz = true;
      g_ovr[c.voice].hzValue = synth::Engine::noteToHz((int)c.value);
      break;
    case cmd::Type::Level:
      g_ovr[c.voice].level = true;
      g_ovr[c.voice].levelValue = c.value;
      break;
    case cmd::Type::Detune:
      g_ovr[c.voice].detune = true;
      g_ovr[c.voice].cents = c.value;
      break;
    case cmd::Type::Select:
      selectVoice(c.voice);
      break;
    case cmd::Type::Sync:
      controls::setSyncMode(c.flag ? controls::SyncMode::Tempo : controls::SyncMode::Phase);
      break;
    case cmd::Type::Pitch:
      g_hzMode = c.flag;
      break;
    case cmd::Type::Free:
      g_ovr[c.voice] = Override();
      break;
  }

  updateAllVoices();
  g_dirty = true;
  Serial.println("OK");
}

// Reads whatever has arrived on Serial and runs each complete line.
void serialPoll() {
  static char buf[96];
  static int len = 0;

  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      buf[len] = '\0';
      runCommand(buf);
      len = 0;
    } else if (len < (int)sizeof(buf) - 1) {
      buf[len++] = (char)c;
    }
  }
}

// ---- Setup, display, and main loop -----------------------------------------

void initVoices() {
  // Defaults: note C4, level 50%, detune centred.
  for (int v = 0; v < synth::kVoices; v++) {
    g_potMem[v][0] = 2048;
    g_potMem[v][1] = 2048;
    g_potMem[v][2] = 2048;
  }
  g_engine.setWave(0, synth::Wave::Saw);
  g_engine.setWave(1, synth::Wave::Square);
  g_engine.setWave(2, synth::Wave::Triangle);
  g_engine.setWave(3, synth::Wave::Saw);
  g_engine.setActive(0, true);
  updateAllVoices();
}

ui::State buildUiState() {
  ui::State s;
  s.selected = g_selected;
  s.hzMode = g_hzMode;
  s.tempoSync = controls::syncMode() == controls::SyncMode::Tempo;
  s.clockHz = controls::clockHz();

  for (int v = 0; v < synth::kVoices; v++) {
    const synth::Voice &voice = g_engine.voice(v);
    ui::VoiceView &w = s.voices[v];
    w.active = voice.active;
    w.wave = voice.wave;
    w.level = voice.level;
    w.hz = voice.hz;
    w.detuneCents = g_ovr[v].detune ? g_ovr[v].cents : logic::potToDetune(g_potMem[v][2]);

    if (s.tempoSync) {
      float ratio = logic::kTempoRatios[logic::potToRatioIndex(g_potMem[v][0])];
      snprintf(w.label, sizeof(w.label), "x%g", ratio);
    } else if (g_hzMode || g_ovr[v].hz) {
      snprintf(w.label, sizeof(w.label), "%.0f Hz", voice.hz);
    } else {
      char name[8];
      synth::Engine::noteName(logic::potToNote(g_potMem[v][0]), name, sizeof(name));
      snprintf(w.label, sizeof(w.label), "%s", name);
    }
  }
  return s;
}

}  // namespace

void setup() {
  Serial.begin(115200);

  ui::begin();
  controls::begin(&g_engine);
  initVoices();

  if (!audioBegin(&g_engine)) {
    Serial.println("audio init failed");
  }

  ui::draw(buildUiState());
  g_dirty = false;
  Serial.println("synth ready (type help)");
}

void loop() {
  static uint32_t lastControl = 0;
  static uint32_t lastDraw = 0;

  uint32_t now = millis();

  serialPoll();

  if (now - lastControl >= 10) {
    lastControl = now;
    handleButtons(now);
    applyPots();
    controls::tick(micros());
    updateAllVoices();
  }

  // The reflective LCD is slow, so redraw at most every 150 ms, and in tempo
  // mode also every 500 ms so the clock readout stays current.
  bool periodic = controls::syncMode() == controls::SyncMode::Tempo && now - lastDraw >= 500;
  if ((g_dirty && now - lastDraw >= 150) || periodic) {
    lastDraw = now;
    g_dirty = false;
    ui::draw(buildUiState());
  }

  vTaskDelay(pdMS_TO_TICKS(2));
}

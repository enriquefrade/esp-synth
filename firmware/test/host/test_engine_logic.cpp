// Host tests for the synth engine and input logic. No hardware needed.
// Build and run: firmware/test/host/run.sh

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"
#include "engine.h"
#include "logic.h"

static int g_failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      g_failures++;                                                    \
    }                                                                  \
  } while (0)

static void testNoteToHz() {
  CHECK(fabsf(synth::Engine::noteToHz(69) - 440.0f) < 0.01f);
  CHECK(fabsf(synth::Engine::noteToHz(81) - 880.0f) < 0.01f);
  CHECK(fabsf(synth::Engine::noteToHz(60) - 261.63f) < 0.05f);
}

static void testNoteName() {
  char buf[8];
  synth::Engine::noteName(69, buf, sizeof(buf));
  CHECK(strcmp(buf, "A4") == 0);
  synth::Engine::noteName(60, buf, sizeof(buf));
  CHECK(strcmp(buf, "C4") == 0);
  synth::Engine::noteName(61, buf, sizeof(buf));
  CHECK(strcmp(buf, "C#4") == 0);
}

static void testWaveShapes() {
  // Saw: -1 at start, approaching +1 at the end.
  CHECK(fabsf(synth::Engine::waveShape(synth::Wave::Saw, 0.0f) + 1.0f) < 1e-5f);
  CHECK(synth::Engine::waveShape(synth::Wave::Saw, 0.999f) > 0.99f);
  // Square: +1 first half, -1 second half.
  CHECK(synth::Engine::waveShape(synth::Wave::Square, 0.25f) == 1.0f);
  CHECK(synth::Engine::waveShape(synth::Wave::Square, 0.75f) == -1.0f);
  // Triangle: -1 at 0, +1 at 0.5, back toward -1 at 1.
  CHECK(fabsf(synth::Engine::waveShape(synth::Wave::Triangle, 0.0f) + 1.0f) < 1e-5f);
  CHECK(fabsf(synth::Engine::waveShape(synth::Wave::Triangle, 0.5f) - 1.0f) < 1e-5f);
  CHECK(synth::Engine::waveShape(synth::Wave::Triangle, 0.9999f) < -0.99f);
}

static void testSilentWhenInactive() {
  synth::Engine e(24000.0f);
  e.setFrequency(0, 440.0f);
  e.setActive(0, false);
  int16_t buf[256];
  e.render(buf, 256);
  bool allZero = true;
  for (int i = 0; i < 256; i++) {
    if (buf[i] != 0) allZero = false;
  }
  CHECK(allZero);
}

static void testOutputBounded() {
  synth::Engine e(24000.0f);
  for (int v = 0; v < synth::kVoices; v++) {
    e.setFrequency(v, 100.0f + 37.0f * v);
    e.setLevel(v, 1.0f);
    e.setActive(v, true);
    e.setWave(v, (synth::Wave)(v % 3));
  }
  int16_t buf[4096];
  e.render(buf, 4096);
  for (int i = 0; i < 4096; i++) {
    CHECK(buf[i] >= -32767 && buf[i] <= 32767);
  }
}

static void testFrequencyPeriod() {
  // A 1000 Hz square at 24 kHz should flip sign about every 12 samples.
  synth::Engine e(24000.0f);
  e.setWave(0, synth::Wave::Square);
  e.setFrequency(0, 1000.0f);
  e.setLevel(0, 1.0f);
  e.setActive(0, true);
  int16_t buf[480];
  e.render(buf, 480);
  int flips = 0;
  for (int i = 1; i < 480; i++) {
    if ((buf[i] > 0) != (buf[i - 1] > 0)) flips++;
  }
  // 480 samples = 20 ms = 20 cycles = 40 sign flips (allow +-2 for phase start).
  CHECK(flips >= 38 && flips <= 42);
}

static void testResetPhases() {
  synth::Engine e(24000.0f);
  e.setFrequency(0, 300.0f);
  e.setActive(0, true);
  int16_t buf[100];
  e.render(buf, 100);
  CHECK(e.voice(0).phase != 0);
  e.resetPhases();
  CHECK(e.voice(0).phase == 0);
}

static void testSetFrequencyClamps() {
  synth::Engine e(24000.0f);
  e.setFrequency(0, 50000.0f);
  CHECK(e.voice(0).hz <= 12000.0f);
  e.setFrequency(0, -5.0f);
  CHECK(e.voice(0).hz == 0.0f);
  CHECK(e.voice(0).inc == 0);
}

static void testButton() {
  logic::Button b;
  // Short tap: click on release.
  CHECK(b.update(true, 0) == logic::Button::Event::None);
  CHECK(b.update(true, 50) == logic::Button::Event::None);
  CHECK(b.update(false, 100) == logic::Button::Event::None);
  CHECK(b.update(false, 130) == logic::Button::Event::Click);

  // Long hold: long event fires once, no click on release.
  logic::Button c;
  CHECK(c.update(true, 0) == logic::Button::Event::None);
  CHECK(c.update(true, 30) == logic::Button::Event::None);
  CHECK(c.update(true, 800) == logic::Button::Event::Long);
  CHECK(c.update(true, 900) == logic::Button::Event::None);
  CHECK(c.update(false, 950) == logic::Button::Event::None);
  CHECK(c.update(false, 1000) == logic::Button::Event::None);

  // Bounce shorter than the debounce time is ignored.
  logic::Button d;
  CHECK(d.update(true, 0) == logic::Button::Event::None);
  CHECK(d.update(false, 5) == logic::Button::Event::None);
  CHECK(d.update(false, 100) == logic::Button::Event::None);
  CHECK(!d.isDown());

  // Suppressed click (used by KEY+BOOT combo) does not fire.
  logic::Button e;
  CHECK(e.update(true, 0) == logic::Button::Event::None);
  CHECK(e.update(true, 50) == logic::Button::Event::None);
  e.suppressClick();
  CHECK(e.update(false, 100) == logic::Button::Event::None);
  CHECK(e.update(false, 130) == logic::Button::Event::None);
}

static void testPotMappings() {
  CHECK(logic::potToNote(0) == 24);
  CHECK(logic::potToNote(4095) == 96);
  CHECK(logic::potToNote(2048) == 60);  // C4 default

  CHECK(fabsf(logic::potToHz(0) - 20.0f) < 0.01f);
  CHECK(fabsf(logic::potToHz(4095) - 2000.0f) < 0.5f);

  CHECK(logic::potToLevel(0) == 0.0f);
  CHECK(fabsf(logic::potToLevel(4095) - 1.0f) < 1e-5f);

  CHECK(logic::potToDetune(2048) == 0.0f);      // centre dead zone
  CHECK(fabsf(logic::potToDetune(4095) - 50.0f) < 1e-3f);
  CHECK(fabsf(logic::potToDetune(0) + 50.0f) < 1e-3f);

  CHECK(logic::potToRatioIndex(0) == 0);
  CHECK(logic::potToRatioIndex(4095) == 4);
}

static void testClockTracker() {
  logic::ClockTracker t;
  CHECK(!t.onEdge(0));                  // first edge only sets the reference
  CHECK(t.onEdge(500000));              // 0.5 s period -> 2 Hz
  CHECK(fabsf(t.hz() - 2.0f) < 1e-3f);
  CHECK(!t.onEdge(500100 + 100));       // 100 us period: too fast, rejected
  CHECK(fabsf(t.hz() - 2.0f) < 1e-3f);
  t.checkTimeout(500000 + 3000000);     // no edges for 3 s
  CHECK(t.hz() == 0.0f);
}

static void testPotFilter() {
  logic::PotFilter f;
  CHECK(f.update(1000) == 1000);
  for (int i = 0; i < 50; i++) f.update(2000);
  CHECK(f.update(2000) >= 1995);
}

static void testCommandParser() {
  using cmd::Type;
  CHECK(cmd::parse("").type == Type::None);
  CHECK(cmd::parse("   ").type == Type::None);
  CHECK(cmd::parse("HELP").type == Type::Help);
  CHECK(cmd::parse("status").type == Type::Status);
  CHECK(cmd::parse("pots").type == Type::Pots);
  CHECK(cmd::parse("audio").type == Type::Audio);
  CHECK(cmd::parse("codec").type == Type::Codec);

  cmd::Command c = cmd::parse("on 2");
  CHECK(c.type == Type::On && c.voice == 1);
  CHECK(cmd::parse("off 4").voice == 3);

  c = cmd::parse("wave 3 square");
  CHECK(c.type == Type::Wave && c.voice == 2 && c.wave == 1);
  CHECK(cmd::parse("wave 1 tri").wave == 2);
  CHECK(cmd::parse("wave 1 saw").wave == 0);

  c = cmd::parse("freq 1 440.5");
  CHECK(c.type == Type::Freq && c.voice == 0 && fabsf(c.value - 440.5f) < 1e-3f);

  c = cmd::parse("note 2 69");
  CHECK(c.type == Type::Note && c.voice == 1 && c.value == 69.0f);

  CHECK(cmd::parse("level 1 0.25").type == Type::Level);
  CHECK(cmd::parse("detune 1 -12").value == -12.0f);
  CHECK(cmd::parse("sel 3").voice == 2);
  CHECK(cmd::parse("free 4").type == Type::Free);

  CHECK(cmd::parse("sync tempo").flag == true);
  CHECK(cmd::parse("sync phase").flag == false);
  CHECK(cmd::parse("pitch hz").flag == true);
}

static void testCommandParserRejects() {
  const char *err = nullptr;
  CHECK(cmd::parse("on 5", &err).type == cmd::Type::Invalid && err != nullptr);
  CHECK(cmd::parse("on 0").type == cmd::Type::Invalid);
  CHECK(cmd::parse("on", nullptr).type == cmd::Type::Invalid);
  CHECK(cmd::parse("on 1.5").type == cmd::Type::Invalid);
  CHECK(cmd::parse("wave 1 noise").type == cmd::Type::Invalid);
  CHECK(cmd::parse("freq 1 0").type == cmd::Type::Invalid);
  CHECK(cmd::parse("freq 1 20000").type == cmd::Type::Invalid);
  CHECK(cmd::parse("freq 1 abc").type == cmd::Type::Invalid);
  CHECK(cmd::parse("note 1 128").type == cmd::Type::Invalid);
  CHECK(cmd::parse("level 1 1.5").type == cmd::Type::Invalid);
  CHECK(cmd::parse("detune 1 200").type == cmd::Type::Invalid);
  CHECK(cmd::parse("sync maybe").type == cmd::Type::Invalid);
  CHECK(cmd::parse("dance 1").type == cmd::Type::Invalid);
}

int main() {
  testNoteToHz();
  testNoteName();
  testWaveShapes();
  testSilentWhenInactive();
  testOutputBounded();
  testFrequencyPeriod();
  testResetPhases();
  testSetFrequencyClamps();
  testButton();
  testPotMappings();
  testClockTracker();
  testPotFilter();
  testCommandParser();
  testCommandParserRejects();

  if (g_failures) {
    fprintf(stderr, "%d check(s) failed\n", g_failures);
    return 1;
  }
  printf("all host tests passed\n");
  return 0;
}

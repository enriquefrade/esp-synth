#include "controls.h"

#include <Arduino.h>

#include "config.h"
#include "logic.h"

namespace controls {

namespace {

synth::Engine* g_engine = nullptr;
logic::ClockTracker g_clock;
volatile SyncMode g_mode = SyncMode::Phase;

// Runs on every rising edge of the sync input.
void IRAM_ATTR onSyncEdge() {
  uint32_t now = micros();
  if (g_mode == SyncMode::Phase) {
    if (g_engine) g_engine->resetPhases();
  } else {
    // ClockTracker is only written here, so the race with tick() is limited to
    // a single float read.
    g_clock.onEdge(now);
  }
}

}  // namespace

void begin(synth::Engine* engine) {
  g_engine = engine;

  pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
  pinMode(PIN_BTN_KEY, INPUT_PULLUP);

  analogSetPinAttenuation(PIN_POT_FREQ, ADC_11db);
  analogSetPinAttenuation(PIN_POT_LEVEL, ADC_11db);
  analogSetPinAttenuation(PIN_POT_DETUNE, ADC_11db);

  pinMode(PIN_SYNC_IN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_SYNC_IN), onSyncEdge, RISING);
}

uint16_t readPot(int pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 4; i++) {
    sum += analogRead(pin);
  }
  return (uint16_t)(sum / 4);
}

bool readButton(int pin) {
  return digitalRead(pin) == LOW;
}

void setSyncMode(SyncMode mode) {
  g_mode = mode;
}

SyncMode syncMode() {
  return g_mode;
}

float clockHz() {
  return g_clock.hz();
}

void tick(uint32_t nowUs) {
  g_clock.checkTimeout(nowUs);
}

}  // namespace controls

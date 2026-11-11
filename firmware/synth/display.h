#pragma once

#include "engine.h"

// Screen layout and drawing for the 400 x 300 reflective LCD.
namespace ui {

struct VoiceView {
  bool active = false;
  synth::Wave wave = synth::Wave::Saw;
  float level = 0.0f;         // 0..1
  float detuneCents = 0.0f;
  float hz = 0.0f;
  char label[12] = "";        // note name, Hz, or tempo ratio
};

struct State {
  VoiceView voices[synth::kVoices];
  int selected = 0;
  bool hzMode = false;        // false = notes
  bool tempoSync = false;
  float clockHz = 0.0f;
};

void begin();
void draw(const State& state);

}  // namespace ui

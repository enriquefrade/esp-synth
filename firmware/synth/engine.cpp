#include "engine.h"

#include <math.h>
#include <stdio.h>

namespace synth {

namespace {
constexpr double kTwoPow32 = 4294967296.0;
constexpr float kMixScale = 0.5f;  // 4 voices at full level clip, which the clamp handles
}  // namespace

Engine::Engine(float sampleRate) : sr_(sampleRate) {}

void Engine::setWave(int v, Wave w) {
  voices_[v].wave = w;
}

void Engine::setActive(int v, bool on) {
  voices_[v].active = on;
}

void Engine::setLevel(int v, float level) {
  if (level < 0.0f) level = 0.0f;
  if (level > 1.0f) level = 1.0f;
  voices_[v].level = level;
}

void Engine::setFrequency(int v, float hz) {
  Voice& voice = voices_[v];
  if (hz < 0.0f) hz = 0.0f;
  if (hz > sr_ * 0.5f) hz = sr_ * 0.5f;
  voice.hz = hz;
  voice.inc = (uint32_t)((double)hz * kTwoPow32 / (double)sr_);
}

void Engine::resetPhases() {
  for (int i = 0; i < kVoices; i++) {
    voices_[i].phase = 0;
  }
}

float Engine::waveShape(Wave w, float u) {
  switch (w) {
    case Wave::Saw:
      return 2.0f * u - 1.0f;
    case Wave::Square:
      return u < 0.5f ? 1.0f : -1.0f;
    case Wave::Triangle:
      return u < 0.5f ? (4.0f * u - 1.0f) : (3.0f - 4.0f * u);
    default:
      return 0.0f;
  }
}

void Engine::render(int16_t* out, int frames) {
  for (int n = 0; n < frames; n++) {
    float sum = 0.0f;
    for (int v = 0; v < kVoices; v++) {
      Voice& voice = voices_[v];
      if (!voice.active || voice.inc == 0) {
        voice.phase += voice.inc;
        continue;
      }
      // Top 24 bits of the phase give u in [0, 1).
      float u = (float)(voice.phase >> 8) * (1.0f / 16777216.0f);
      sum += waveShape(voice.wave, u) * voice.level;
      voice.phase += voice.inc;
    }
    float y = sum * kMixScale;
    if (y > 1.0f) y = 1.0f;
    if (y < -1.0f) y = -1.0f;
    out[n] = (int16_t)(y * 32767.0f);
  }
}

float Engine::noteToHz(int midi) {
  return 440.0f * powf(2.0f, (float)(midi - 69) / 12.0f);
}

void Engine::noteName(int midi, char* buf, size_t len) {
  static const char* kNames[12] = {"C", "C#", "D", "D#", "E", "F",
                                   "F#", "G", "G#", "A", "A#", "B"};
  int octave = midi / 12 - 1;
  snprintf(buf, len, "%s%d", kNames[midi % 12], octave);
}

const char* Engine::waveName(Wave w) {
  switch (w) {
    case Wave::Saw:      return "SAW";
    case Wave::Square:   return "SQR";
    case Wave::Triangle: return "TRI";
    default:             return "?";
  }
}

}  // namespace synth

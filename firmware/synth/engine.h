#pragma once

// Audio engine: up to 4 band-limited-free (naive) oscillators, mixed to mono int16.
// No Arduino dependencies, so it can be tested on the host.

#include <stddef.h>
#include <stdint.h>

namespace synth {

constexpr int kVoices = 4;

enum class Wave : uint8_t { Saw = 0, Square, Triangle, Count };

struct Voice {
  Wave wave = Wave::Saw;
  float hz = 0.0f;
  float level = 0.5f;   // 0..1
  bool active = false;
  uint32_t phase = 0;   // 32-bit phase accumulator, wraps at 2^32
  uint32_t inc = 0;     // phase increment per sample
};

class Engine {
public:
  explicit Engine(float sampleRate);

  void setWave(int v, Wave w);
  void setActive(int v, bool on);
  void setLevel(int v, float level);
  void setFrequency(int v, float hz);   // 0 Hz = silent

  // Hard sync: restart every oscillator at phase 0.
  // Called from an ISR; each write is a single 32-bit store.
  void resetPhases();

  const Voice& voice(int v) const { return voices_[v]; }

  // Fills `frames` mono samples. Called from the audio task only.
  void render(int16_t* out, int frames);

  // Waveform value in [-1, 1] for a phase position u in [0, 1).
  static float waveShape(Wave w, float u);
  static float noteToHz(int midi);
  static void noteName(int midi, char* buf, size_t len);
  static const char* waveName(Wave w);

private:
  float sr_;
  Voice voices_[kVoices];
};

}  // namespace synth

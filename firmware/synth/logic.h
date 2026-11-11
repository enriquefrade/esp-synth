#pragma once

// Input logic with no Arduino dependencies, so it can be tested on the host.

#include <math.h>
#include <stdint.h>

namespace logic {

// Debounced button with click and long-press events.
// Call update() on every poll with the raw level (true = pressed).
class Button {
public:
  enum class Event : uint8_t { None, Click, Long };

  Event update(bool pressed, uint32_t nowMs) {
    if (pressed != lastRaw_) {
      lastRaw_ = pressed;
      lastChange_ = nowMs;
    }

    Event ev = Event::None;
    if (pressed != down_ && nowMs - lastChange_ >= kDebounceMs) {
      down_ = pressed;
      if (down_) {
        pressedAt_ = nowMs;
        longFired_ = false;
        suppressed_ = false;
      } else if (!longFired_ && !suppressed_) {
        ev = Event::Click;
      }
    }

    if (down_ && !longFired_ && nowMs - pressedAt_ >= kLongPressMs) {
      longFired_ = true;
      ev = Event::Long;
    }
    return ev;
  }

  bool isDown() const { return down_; }

  // Used when another button combo consumed this press.
  void suppressClick() { suppressed_ = true; }

private:
  static constexpr uint32_t kDebounceMs = 20;
  static constexpr uint32_t kLongPressMs = 700;

  bool lastRaw_ = false;
  bool down_ = false;
  bool longFired_ = false;
  bool suppressed_ = false;
  uint32_t lastChange_ = 0;
  uint32_t pressedAt_ = 0;
};

// Exponential smoothing for an ADC reading.
class PotFilter {
public:
  uint16_t update(uint16_t raw) {
    if (!started_) {
      value_ = raw;
      started_ = true;
    } else {
      value_ += (raw - value_) * 0.25f;
    }
    return (uint16_t)(value_ + 0.5f);
  }

private:
  float value_ = 0.0f;
  bool started_ = false;
};

// Clock period tracker for tempo sync. Call onEdge() with micros() on each rising edge.
class ClockTracker {
public:
  // Returns true if the edge produced a valid period.
  bool onEdge(uint32_t nowUs) {
    uint32_t period = nowUs - lastEdgeUs_;
    lastEdgeUs_ = nowUs;
    if (!hasEdge_) {
      hasEdge_ = true;
      return false;
    }
    if (period < kMinPeriodUs || period > kMaxPeriodUs) {
      return false;
    }
    float hz = 1000000.0f / (float)period;
    hz_ = (hz_ == 0.0f) ? hz : (hz_ * 0.7f + hz * 0.3f);
    return true;
  }

  // Called from the main loop. Clears the clock if no edge arrived recently.
  void checkTimeout(uint32_t nowUs) {
    if (hasEdge_ && nowUs - lastEdgeUs_ > kTimeoutUs) {
      hz_ = 0.0f;
    }
  }

  float hz() const { return hz_; }

private:
  static constexpr uint32_t kMinPeriodUs = 5000;      // 200 Hz max
  static constexpr uint32_t kMaxPeriodUs = 2000000;   // 0.5 Hz min
  static constexpr uint32_t kTimeoutUs = 2000000;

  uint32_t lastEdgeUs_ = 0;
  bool hasEdge_ = false;
  volatile float hz_ = 0.0f;
};

// Maps a 12-bit pot reading to a MIDI note (C1..C7).
inline int potToNote(uint16_t raw) {
  return 24 + (raw * 72 + 2047) / 4095;
}

// Maps a 12-bit pot reading to 20..2000 Hz on a log scale.
inline float potToHz(uint16_t raw) {
  float t = (float)raw / 4095.0f;
  return 20.0f * powf(100.0f, t);
}

// Maps a 12-bit pot reading to a level in 0..1.
inline float potToLevel(uint16_t raw) {
  return (float)raw / 4095.0f;
}

// Maps a 12-bit pot reading to -50..+50 cents, with a dead zone at centre.
inline float potToDetune(uint16_t raw) {
  float t = (float)raw / 4095.0f * 2.0f - 1.0f;
  if (fabsf(t) < 0.02f) return 0.0f;
  return t * 50.0f;
}

// Tempo sync ratios selected by pot 1.
constexpr float kTempoRatios[5] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};

inline int potToRatioIndex(uint16_t raw) {
  int idx = (raw * 5) / 4096;
  return idx > 4 ? 4 : idx;
}

}  // namespace logic

#pragma once

#include <stdint.h>

#include "engine.h"

// Hardware inputs: buttons, pots, and the sync input.
namespace controls {

enum class SyncMode : uint8_t { Phase = 0, Tempo };

// Configures pins and the sync interrupt. `engine` receives phase resets.
void begin(synth::Engine* engine);

// Averaged 12-bit ADC reading for one of the pot pins.
uint16_t readPot(int pin);

// Raw button level (true = pressed).
bool readButton(int pin);

void setSyncMode(SyncMode mode);
SyncMode syncMode();

// Measured clock rate in Hz for tempo sync; 0 if no clock is present.
float clockHz();

// Call from the main loop to clear the clock when it stops.
void tick(uint32_t nowUs);

}  // namespace controls

#pragma once

#include <stdint.h>

#include "engine.h"

// Starts the ES8311 codec and a task that streams engine output to the speaker.
bool audioBegin(synth::Engine* engine);

struct AudioStats {
  uint32_t blocks;      // blocks handed to the codec since boot
  int lastWriteRet;     // return value of the last esp_codec_dev_write
  int peak;             // largest absolute sample seen since boot (0..32767)
};
AudioStats audioStats();

// Diagnostics: read one ES8311 register over I2C (returns false on bus error).
bool audioReadCodecReg(uint8_t reg, uint8_t *value);

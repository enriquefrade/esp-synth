#pragma once

#include <waveshare_rlcd.h>   // makes the vendored drivers visible to the build

// Pin map. Sources: docs/pinout.md, docs/controls-and-sync.md
// Buttons are active low (pulled up on the board).

// Potentiometers (ADC1, header P1)
constexpr int PIN_POT_FREQ   = 1;   // pot 1: frequency / ratio
constexpr int PIN_POT_LEVEL  = 2;   // pot 2: level
constexpr int PIN_POT_DETUNE = 3;   // pot 3: detune

// Onboard buttons
constexpr int PIN_BTN_BOOT = 0;     // Key1: waveform / pitch mode / sync combo
constexpr int PIN_BTN_KEY  = 18;    // Key4: select / toggle active

// Sync input (header P1, 5 V signal conditioned to 3.3 V)
constexpr int PIN_SYNC_IN = 17;

// Display (ST7305, same wiring as the Waveshare demo)
constexpr int PIN_RLCD_SCK  = 11;
constexpr int PIN_RLCD_MOSI = 12;
constexpr int PIN_RLCD_DC   = 5;
constexpr int PIN_RLCD_CS   = 40;
constexpr int PIN_RLCD_RST  = 41;

// Audio
constexpr int PIN_PA_CTRL = 46;   // speaker amplifier enable (NS4150B CTRL), active high
constexpr int SAMPLE_RATE  = 24000;
constexpr int BLOCK_FRAMES = 256;
constexpr int NUM_VOICES   = 4;

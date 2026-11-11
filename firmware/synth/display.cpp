#include "display.h"

#include <Arduino.h>
#include <stdio.h>

#include "config.h"
#include "display/ST7305_U8g2.h"

namespace ui {

namespace {

constexpr int kWidth = 400;
constexpr int kTileWidth = 100;
constexpr int kTileTop = 24;
constexpr int kTileHeight = 240;

ST7305_U8g2 g_lcd(PIN_RLCD_SCK, PIN_RLCD_MOSI, PIN_RLCD_DC, PIN_RLCD_CS, PIN_RLCD_RST);
U8G2 *g_u8g2 = nullptr;

void drawWave(U8G2 *u8g2, int x, int y, int w, int h, synth::Wave wave) {
  const int samples = w;
  int prevX = x;
  int prevY = y + h / 2 - (int)(synth::Engine::waveShape(wave, 0.0f) * (h / 2));
  for (int i = 1; i < samples; i++) {
    float u = (float)i / (float)samples;
    int px = x + i;
    int py = y + h / 2 - (int)(synth::Engine::waveShape(wave, u) * (h / 2));
    u8g2->drawLine(prevX, prevY, px, py);
    prevX = px;
    prevY = py;
  }
}

void drawTile(U8G2 *u8g2, int index, const VoiceView &v, bool selected) {
  int x = index * kTileWidth;

  u8g2->setDrawColor(1);
  if (selected) {
    u8g2->drawBox(x + 2, kTileTop, kTileWidth - 4, kTileHeight);
    u8g2->setDrawColor(0);
  } else {
    u8g2->drawFrame(x + 2, kTileTop, kTileWidth - 4, kTileHeight);
    u8g2->setDrawColor(1);
  }

  char text[24];
  u8g2->setFont(u8g2_font_6x13_tf);
  snprintf(text, sizeof(text), "OSC %d", index + 1);
  u8g2->drawStr(x + 10, kTileTop + 18, text);

  // Active / inactive indicator
  if (v.active) {
    u8g2->drawDisc(x + kTileWidth - 16, kTileTop + 13, 5);
  } else {
    u8g2->drawCircle(x + kTileWidth - 16, kTileTop + 13, 5);
  }

  // Waveform icon
  drawWave(u8g2, x + 10, kTileTop + 30, kTileWidth - 20, 40, v.wave);

  // Label: note, Hz, or tempo ratio
  u8g2->setFont(u8g2_font_helvB12_tf);
  u8g2->drawStr(x + 10, kTileTop + 92, v.label);

  // Detune
  u8g2->setFont(u8g2_font_6x13_tf);
  snprintf(text, sizeof(text), "%+.0f c", v.detuneCents);
  u8g2->drawStr(x + 10, kTileTop + 112, text);

  // Level bar
  u8g2->drawFrame(x + 10, kTileTop + 124, kTileWidth - 20, 10);
  int fill = (int)((kTileWidth - 22) * v.level);
  if (fill > 0) {
    u8g2->drawBox(x + 11, kTileTop + 125, fill, 8);
  }

  // Waveform name
  u8g2->drawStr(x + 10, kTileTop + 160, synth::Engine::waveName(v.wave));
}

}  // namespace

void begin() {
  g_lcd.begin(0, U8G2_R1);
  g_u8g2 = g_lcd.getU8g2();
}

void draw(const State &state) {
  if (!g_u8g2) return;
  U8G2 *u8g2 = g_u8g2;

  u8g2->clearBuffer();
  u8g2->setDrawColor(1);
  u8g2->setFont(u8g2_font_6x13_tf);

  char header[64];
  if (state.tempoSync) {
    snprintf(header, sizeof(header), "SYNC: TEMPO  clk %.1f Hz", state.clockHz);
  } else {
    snprintf(header, sizeof(header), "SYNC: PHASE");
  }
  u8g2->drawStr(8, 14, header);
  u8g2->drawStr(kWidth - 8 - u8g2->getStrWidth(state.hzMode ? "PITCH: HZ" : "PITCH: NOTE"), 14,
                state.hzMode ? "PITCH: HZ" : "PITCH: NOTE");

  for (int i = 0; i < synth::kVoices; i++) {
    drawTile(u8g2, i, state.voices[i], i == state.selected);
  }

  u8g2->setDrawColor(1);
  u8g2->drawStr(8, 290, "KEY sel/on-off  BOOT wave  hold BOOT pitch  KEY+BOOT sync");

  u8g2->sendBuffer();
}

}  // namespace ui

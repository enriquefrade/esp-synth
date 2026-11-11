#include "audio.h"

#include <Arduino.h>

#include "codec/codec_board/codec_board.h"
#include "codec/codec_board/codec_init.h"
#include "codec/esp_codec_dev/include/esp_codec_dev.h"
#include "codec/i2c_bsp.h"
#include "config.h"

#include <driver/i2c_master.h>

namespace {

// Same bus setup as Waveshare's 07_Audio_Test: SCL 14, SDA 13, port 0.
I2cMasterBus g_i2cBus(14, 13, 0);

esp_codec_dev_handle_t g_playback = nullptr;
i2c_master_dev_handle_t g_es8311Dev = nullptr;
constexpr uint8_t kEs8311Addr = 0x18;  // same address as Waveshare's CodecPort
synth::Engine *g_engine = nullptr;
volatile uint32_t g_blocks = 0;
volatile int g_lastRet = 0;
volatile int g_peak = 0;

void audioTask(void *arg) {
  (void)arg;
  static int16_t mono[BLOCK_FRAMES];
  static int16_t stereo[BLOCK_FRAMES * 2];

  for (;;) {
    g_engine->render(mono, BLOCK_FRAMES);
    int peak = g_peak;
    for (int i = 0; i < BLOCK_FRAMES; i++) {
      stereo[2 * i] = mono[i];
      stereo[2 * i + 1] = mono[i];
      int m = mono[i] < 0 ? -mono[i] : mono[i];
      if (m > peak) peak = m;
    }
    g_peak = peak;
    // Blocks until the codec has consumed the buffer, which paces the loop.
    g_lastRet = esp_codec_dev_write(g_playback, stereo, sizeof(stereo));
    g_blocks = g_blocks + 1;
  }
}

}  // namespace

bool audioBegin(synth::Engine *engine) {
  g_engine = engine;

  set_codec_board_type("S3_RLCD_4_2");

  codec_init_cfg_t cfg = {};
  cfg.in_mode = CODEC_I2S_MODE_TDM;
  cfg.out_mode = CODEC_I2S_MODE_TDM;
  cfg.in_use_tdm = false;
  cfg.reuse_dev = false;
  if (init_codec(&cfg) != ESP_OK) {
    return false;
  }

  g_playback = get_playback_handle();
  if (!g_playback) {
    return false;
  }

  esp_codec_dev_set_out_vol(g_playback, 100);

  esp_codec_dev_sample_info_t fs = {};
  fs.sample_rate = SAMPLE_RATE;
  fs.channel = 2;
  fs.bits_per_sample = 16;
  if (esp_codec_dev_open(g_playback, &fs) != ESP_CODEC_DEV_OK) {
    return false;
  }

  // The codec driver did not raise the amplifier enable on this board (PA_CTRL, GPIO46),
  // so the NS4150B stayed off. Enable it explicitly.
  pinMode(PIN_PA_CTRL, OUTPUT);
  digitalWrite(PIN_PA_CTRL, HIGH);

  // Separate handle for register reads (diagnostics). Playback does not use it.
  i2c_device_config_t devCfg = {};
  devCfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  devCfg.device_address = kEs8311Addr;
  devCfg.scl_speed_hz = 400000;
  i2c_master_bus_add_device(g_i2cBus.Get_I2cBusHandle(), &devCfg, &g_es8311Dev);

  BaseType_t ok = xTaskCreatePinnedToCore(audioTask, "audio", 6 * 1024, nullptr, 5, nullptr, 1);
  return ok == pdPASS;
}

AudioStats audioStats() {
  AudioStats s;
  s.blocks = g_blocks;
  s.lastWriteRet = g_lastRet;
  s.peak = g_peak;
  return s;
}

bool audioReadCodecReg(uint8_t reg, uint8_t *value) {
  if (!g_es8311Dev) return false;
  return i2c_master_transmit_receive(g_es8311Dev, &reg, 1, value, 1, 100) == ESP_OK;
}

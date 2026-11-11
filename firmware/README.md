# Firmware

4-oscillator synth for the ESP32-S3-RLCD-4.2. Arduino framework (ESP32 core 3.3.x).

## Layout

| Path | What it is |
|---|---|
| `synth/synth.ino` | Main sketch: button handling, pot takeover, UI state, loop |
| `synth/engine.{h,cpp}` | Oscillators (saw, square, triangle) and mixing. No Arduino deps. |
| `synth/logic.h` | Button gestures, pot filter, pot mappings, clock tracker. No Arduino deps. |
| `synth/controls.{h,cpp}` | Pins, ADC reads, sync interrupt |
| `synth/display.{h,cpp}` | 400 x 300 screen layout (U8g2 on the ST7305 driver) |
| `synth/audio.{h,cpp}` | ES8311 codec setup and the audio task |
| `synth/config.h` | Pin map and audio constants (see `docs/pinout.md`) |
| `libraries/waveshare_rlcd/` | Vendored Waveshare display and codec drivers (Apache-2.0, see its `LICENSE`) |
| `test/host/` | Host tests for the engine and logic. Run `test/host/run.sh`. |

## Controls

| Input | Action |
|---|---|
| KEY tap | Select next oscillator |
| KEY hold | Toggle selected oscillator on/off |
| BOOT tap | Cycle waveform (saw, square, triangle) |
| BOOT hold | Toggle pitch mode: notes or Hz |
| KEY held + BOOT tap | Toggle sync mode: phase reset or tempo |
| Pot 1 (GPIO1) | Frequency: note (notes mode), Hz (Hz mode), or ratio of the clock (tempo mode) |
| Pot 2 (GPIO2) | Level |
| Pot 3 (GPIO3) | Detune, -50 to +50 cents |

Pots use soft takeover. After you select an oscillator, a pot only controls it once it moves away from the stored value.

Sync input (GPIO17): rising edges reset oscillator phases (phase mode) or set the clock for tempo mode. Tempo ratios: x0.25, x0.5, x1, x2, x4.

## Build

Requires `arduino-cli` and the ESP32 core:

```sh
arduino-cli core install esp32:esp32@3.3.1
arduino-cli lib install U8g2
```

Compile (board options match the Waveshare Arduino setup: 16 MB flash, OPI PSRAM, USB CDC on boot):

```sh
arduino-cli compile --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi" \
  --libraries firmware/libraries firmware/synth
```

Upload (replace the port):

```sh
arduino-cli upload -p /dev/cu.usbmodemXXXX --fqbn "esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi" firmware/synth
```

Host tests (no hardware):

```sh
firmware/test/host/run.sh
```

## Known limits (v1)

- Saw and square are naive (not band-limited). They alias above a few kHz at 24 kHz sample rate.
- Pot and voice state are shared between the main loop and the audio task without a lock. Single 32-bit and float writes are used, so a glitch is at worst one block of audio.
- Build verified with arduino-cli 1.6.0, ESP32 core 3.3.1 (compiles cleanly, 14% of flash, 7% of RAM).
- Hardware not yet tested: display, codec, sync, and buttons have not been run on the board.

## Browser UI

`ui/index.html` is a single-file control panel that talks to the board over USB serial (Web Serial API).

1. Serve the folder (Web Serial needs localhost or https):
   ```sh
   cd firmware/ui && python3 -m http.server 8000
   ```
2. Open http://localhost:8000 in Chrome or Edge (Safari and Firefox do not support Web Serial).
3. Click **Connect board** and pick the board's USB port.

What it does:
- On-screen keyboard and computer keys (A W S E D F T G Y H U J K), up to 4 notes at once. Notes go to free voices, and the oldest note is stolen when all four are busy.
- Per voice: on/off, waveform, level, detune, and "Free" to give control back to the pots.
- Global: sync mode, pot 1 unit, read status, clear all overrides, all voices off.

The firmware has no effects yet (filter, delay, tremolo). The UI controls the parameters the synth has now.

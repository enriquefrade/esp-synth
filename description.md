i have this device https://www.waveshare.com/esp32-s3-rlcd-4.2.htm and i want to implement a synth on it, 
basically i want to have:
- up to 4 oscillators simultaneous
- sawtooth , square and triagle waveforms
- basic controls over oscillators , lets interact over it

interface: 
- controlled by buttons and potentiometers on gpio
- external sync option

graphical interface:
should show oscillators that are active inactive, have an option to select the type of waveform and basic controls over it parameters.

---
## Sketch (draft, to refine)

Sources: [Waveshare docs](https://docs.waveshare.com/ESP32-S3-RLCD-4.2), interface pin table (image in the same docs).

### Hardware facts (from the docs)
- ESP32-S3, 240 MHz, 512 KB SRAM, 8 MB PSRAM, 16 MB flash
- 4.2" reflective LCD, 300 x 400, no backlight
- Audio: ES8311 codec (I2S) + amplifier. Output options: speaker header (MX1.25 2-pin) now, line out later (not on the board, needs a jack and a buffer)
- Buttons: PWR (hardware power, not usable), BOOT (GPIO0), KEY (GPIO18, customizable)
- Pin table ends at GPIO46. GPIO47 and GPIO48 are not in it, so they are unverified.
- GPIO17 appears on the 2x8 header with no onboard function, so it is verified free.

### Audio output
- v1: speaker header, driven by the ES8311 on I2S (GPIO8/9/10/16/45), amp enable on GPIO46
- Later option: line out jack. Needs a DAC output path or an external I2S DAC, plus a line-level buffer. Not designed yet.

### Audio engine
- 4 oscillators, each with: waveform (saw / square / triangle), frequency, level, active on/off
- Mixed to one output, generated in a periodic task on I2S
- Phase accumulator per oscillator (no envelopes for v1)

### Controls (pin budget)
- Buttons (2, onboard):
  - KEY (GPIO18): short press = select next oscillator, long press = toggle active
  - BOOT (GPIO0): cycle waveform
  - Caveat: holding BOOT during reset enters download mode
- Pots (3, external): GPIO1, GPIO2, GPIO3 (all ADC1). Edit the selected oscillator: frequency, level, detune
- Frequency is set in musical notes, with raw Hz as an option. Both are supported.

### External sync (both modes)
- Input on GPIO17 (header, verified free)
- Signal is 5 V, so it needs conditioning before the ESP32-S3 (3.3 V max). Suggested: series resistor, resistor divider (e.g., 20k to GND, 10k from input, giving about 3.3 V at 5 V in), and a clamp diode to 3.3 V rail for protection. Values need checking against the actual circuit.
- Mode A (v1): reset oscillator phases on each rising edge
- Mode B: tempo sync, where frequency follows the incoming clock
- Mode is chosen in the menu

### Graphical interface
- 4 oscillator tiles: index, waveform shape icon, active/inactive state, frequency, level bar
- Selected tile highlighted
- Reflective LCD refreshes slowly: redraw only changed tiles

### Framework comparison

| | Arduino (ESP32 core) | ESP-IDF |
|---|---|---|
| Setup | Quick | More setup |
| I2S / audio timing | Usable, less control of DMA and task pinning | Full control of DMA buffers and task pinning |
| Display / ES8311 drivers | Likely available as libraries; not checked | Drivers must be written or ported |
| Language | C++ | C |
| Fit for v1 | Faster to a working prototype | Better for tight, glitch-free audio |

Recommendation: start with Arduino, since the ESP32 Arduino core runs on ESP-IDF underneath, and drop to ESP-IDF APIs for I2S only if audio glitches. I haven't checked which Waveshare sample code is available for Arduino or ESP-IDF.

### Pinout decisions (from the schematic, see [docs/](docs/README.md))
- Pots: GPIO1, GPIO2, GPIO3 (header P1, ADC1)
- Sync: GPIO17 (header P1), 5 V conditioned to 3.3 V
- Buttons: BOOT (GPIO0) cycles waveform, KEY (GPIO18) selects and toggles oscillator
- GPIO47/48 are not routed to the header, so they are not used

### Open questions
1. When do you want the line out? It needs a separate hardware step, so I'd leave it out of v1.

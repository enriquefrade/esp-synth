# Pinout

Source: schematic `hardware/ESP32-S3-RLCD-4.2-schematic.pdf` and the Waveshare pin table in the product docs. Status is what the schematic shows, not what the board's firmware uses.

## Free for the synth

| GPIO | Where it is | Use in this project |
|---|---|---|
| GPIO1 | Header P1, pin 7 | Pot 1 (ADC1) |
| GPIO2 | Header P1, pin 9 | Pot 2 (ADC1) |
| GPIO3 | Header P1, pin 11 | Pot 3 (ADC1) |
| GPIO17 | Header P1, pin 13 | Sync input (5 V, conditioned, see [controls-and-sync.md](controls-and-sync.md)) |

Pin numbers come from the schematic drawing of P1. Check them on the board before wiring.

## Used by the board (do not reuse)

| GPIO | Function | Notes |
|---|---|---|
| GPIO0 | BOOT button (Key1) | Pulled up. Strapping pin. Usable as an input after boot, but holding it during reset enters download mode. |
| GPIO4 | Battery ADC (BAT_ADC) | Not free |
| GPIO5 | LCD RS | Display |
| GPIO6 | LCD TE | Display tearing signal |
| GPIO7 | TP INT | Touch interrupt |
| GPIO8 | I2S DSDIN | ES8311 audio data in |
| GPIO9 | I2S SCLK | ES8311 bit clock |
| GPIO10 | I2S ASDOUT | ES8311 audio data out (ADC direction) |
| GPIO11 | LCD SCL | Display |
| GPIO12 | LCD SDA | Display |
| GPIO13 | ESP32 SDA (I2C) | Shared bus: RTC, IMU (QMI8658C), SHTC3, touch. Also on header P1 pin 14. |
| GPIO14 | ESP32 SCL (I2C) | Shared bus. Also on header P1 pin 16. |
| GPIO15 | RTC INT | PCF85063 interrupt |
| GPIO16 | I2S MCLK | ES8311 master clock |
| GPIO18 | KEY (Key4) | Button, pulled up by 10k (R17). Wired to the KEY net only. |
| GPIO19 | USB D- | USB |
| GPIO20 | USB D+ | USB |
| GPIO21 | SD MOSI | TF card |
| GPIO38 | SD SCK | TF card |
| GPIO39 | SD MISO | TF card |
| GPIO40 | LCD CS | Display |
| GPIO41 | LCD RESET | Display |
| GPIO42 | TP RESET | Touch reset |
| GPIO43 | U0TXD | Serial log on header P1 pin 10. Avoid unless serial is not needed. |
| GPIO44 | U0RXD | Serial log on header P1 pin 12. Avoid unless serial is not needed. |
| GPIO45 | I2S LRCK | ES8311 word select |
| GPIO46 | PA_CTRL | Speaker amplifier enable |

## Not usable

| GPIO | Reason |
|---|---|
| GPIO33-37 | Used internally by the octal PSRAM on the N16R8 module. Not in the schematic table. Not verified here. |
| GPIO47, GPIO48 | Routed to the module pads only. The schematic shows no other connection and they are not on header P1. Usable only by soldering to the module. |

## Header P1 (2x8, 2.54 mm)

Read from the schematic drawing:

| Pin | Left | Right |
|---|---|---|
| 1 | VCC3V3 | VBUS |
| 3 | GND | GND |
| 5 | GPIO0 | USB'_N |
| 7 | GPIO1 | USB'_P |
| 9 | GPIO2 | U0TXD |
| 11 | GPIO3 | U0RXD |
| 13 | GPIO17 | ESP32 SDA |
| 15 | GPIO18 | ESP32 SCL |

Odd pins are the left column, even pins are the right column. Pin 2 = VBUS, pin 4 = GND, pin 6 = USB'_N, pin 8 = USB'_P, pin 10 = U0TXD, pin 12 = U0RXD, pin 14 = ESP32 SDA, pin 16 = ESP32 SCL.

The 3.3 V rail (VCC3V3) and GND on P1 are the supply for pots and any sync conditioning. Keep the total draw small. The LDO is an RT9193-33 and its current limit is not checked here.

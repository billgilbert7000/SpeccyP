# SpeccyP 
<div align="left">
  
[![English](https://img.shields.io/badge/English-active-green.svg)](README_ENG.md)
[![Русский](https://img.shields.io/badge/Русский-README.md-blue.svg)](README.md)

</div>

**ZX Spectrum emulator based on Raspberry Pi RP2040, RP2350A, and RP2350B microcontrollers.**

This firmware emulates various Spectrum models with support for HDMI, VGA, TFT displays, I2S audio, PSRAM, USB mouse, and gamepads.

![RP2040](https://img.shields.io/badge/RP2040-supported-green)
![RP2350](https://img.shields.io/badge/RP2350-supported-green)
![License](https://img.shields.io/badge/license-GPLv3-blue.svg)

---

## 📄 License

This project is distributed under the **GNU General Public License v3.0** (GPL-3.0).

You are free to use, modify, and distribute this software under the terms of the license. The full license text is available in the `LICENSE` file or at [https://www.gnu.org/licenses/gpl-3.0.html](https://www.gnu.org/licenses/gpl-3.0.html).

---

## 📦 Firmware Contents

- **Supported boards**: m1p1, m1p2, m2p1, m2p2, z0p2 (RP2350B PiZero2).
- **Details about supported boards** are available at [murmulator.ru](https://murmulator.ru).

---

## 🎮 Features

### Supported ZX Spectrum Models
| Model | Features |
|-------|----------|
| **ZX Spectrum 48** | Original ZX Spectrum 48 ROM |
| **Pentagon 128** | Classic 128K model |
| **Pentagon 512** | Port #7FFD, bits 0,1,2,6,7 |
| **Pentagon 512CASH** | 32KB CASH, switching via IN ports (0xFB) — enable, IN (0x7B) — disable |
| **Pentagon 1024** | Port #7FFD, bits 0,1,2,5,6,7 (bit 5 does NOT lock 48K mode) |
| **Scorpion 256** | Ports #1FFD (bit 4) and #7FFD (bits 0,1,2) |
| **Scorpion GMX 2048** | Ports #DFFD (bits 0,1,2), #1FFD (bit 4), #7FFD (bits 0,1,2) |
| **Navigator 256** | Port #7FFD, bits 0,1,2,6 |
| **MurmoZavr 8Mb** | Port 0xAFF7 (bits 0-5) and #7FFD (bits 0,1,2) |

### Supported Video Outputs
| Type | Parameters |
|------|-----------|
| **HDMI** | 60 Hz with 1.5 divider, auto-detection |
| **VGA** | 60 Hz, auto-detection |
| **TFT** | ILI9341, ST7789 — configuration in Advanced Setup |

### Control Methods
| Device | Connection | Features |
|--------|------------|----------|
| **PS/2 Keyboard** | Via USB adapter or directly (if interface available) | Full Spectrum keyboard emulation |
| **USB Keyboard** | Via USB OTG on Pico | Support for standard USB HID keyboards |
| **USB Mouse** | Via USB OTG on Pico | Kemston Mouse mode, speed adjustment in Advanced Setup |
| **NES Gamepad** | Via appropriate adapter | Full support with additional functions (see "Controls" section) |
| **Xbox Gamepad** | Via USB OTG | Kempston joystick mode (buttons and D-pad) |
| **Wireless Gamepad** | Via USB OTG | Support for Chinese wireless gamepads in Kempston mode |

### Z80 Emulator
Uses a precise emulator by [Manuel Sainz](https://github.com/redcode/Z80), supporting all documented and undocumented processor features.

In the **Advanced Menu**, you can select the CPU type (switching on the fly):
- ZILOG NMOS
- ZILOG CMOS
- NEC NMOS
- ST CMOS
- UNREAL (all CPU-specific settings disabled)

### Screen and Video
- **First screen line adjustment** (useful for demos with border effects).
  - `[Ctrl] + [F7]` — decrease value
  - `[Ctrl] + [F8]` — increase value

### Audio
- Volume adjustment: `[F7]` (decrease), `[F8]` (increase) from 0 to 100%.
- Audio modes: Soft AY-3-8910, Soft TurboSound, I2S AY-3-8910, I2S TurboSound.
- **BUSTER I2S** (amplifier for TDA) from 0 to 7 (default 0 — no clipping).
- **Hard AY/TS** with AY clock generator (f=1773000Hz) on GPIO 29.

### Tape Audio Loading
- When loading a TAP file from the file manager, audio loading is **disabled**.
- To restore, perform a **Hard Reset**.
- Audio loading volume level can be adjusted in Advanced Setup (0-15).

---

## 💾 Memory (PSRAM)

Firmware is adapted for various boards:

| Board      | PSRAM Connection |
|------------|------------------|
| m1p2 (RP2350) | Sandwich PSRAM GPIO 19 (A) or GPIO 47 (B) / SPI GPIO 18-21 |
| m1p1 (RP2040) | SPI PSRAM GPIO 18-21 |
| m2p2 (RP2350) | Sandwich PSRAM CS GPIO 8 (A) or GPIO 47 (B) |
| m2p1 (RP2040) | Works without PSRAM (Spectrum 48/128 only) |

---

## 🕹️ Controls

### Hotkeys

| Key | Action |
|-----|--------|
| `[F1]` | Help |
| `[F2]` | Quick save |
| `[F3]` | Load save |
| `[F5]` | Save to slot 0 + save configuration |
| `[F6]` | Cycle through color palettes |
| `[F7]/[F8]` | Volume down/up |
| `[F9]` | NMI (Scorpion/Navigator/Pentagon 512CASH) |
| `[F10]` | Mode Normal/Turbo/Fast (3.5MHz/Int50Hz, Fast/Int100Hz) |
| `[F11]/[Ins]` | File menu |
| `[F12]/[Home]` | Settings menu |
| `[END]` | Disassembler |
| `[Ctrl]+[Alt]+[Del]` | Soft Reset |
| `[Shift]+[Alt]+[Del]` | Hard Reset |
| `[Ctrl]+[F7]/[F8]` | Screen start line adjustment |

**In the file menu:**
- `[ENTER]` — mount TRD/SCL images or connect TAP files
- `[SPACE]` — quick launch of TDS and SCL files

### 🎮 NES Gamepad
- `START + Down Arrow` — file browser
- `START + Up Arrow` — settings menu
- `START + Left Arrow` — SAVE menu
- `START + Right Arrow` — LOAD menu
- `[A]` — select / `[B]` — launch or exit

---

## ⚙️ Settings and Configuration

All settings are saved to the **`speccy_p.cnf`** file in the root of the SD card.

### AutoRUN
Configured in the `[F12]` menu:
- **File TR-DOS** — load TRD/SCL image from drive A
- **QuickSave Slot 0** — load state from slot 0
- **OFF** — disable autostart

---

## 🖱️ Additional Features

- **USB mouse** (Kemston Mouse) with speed adjustment.
- **Xbox gamepad** support (Kempston mode).
- **Chinese wireless gamepad** support.
- **Power OFF** option — shutdown with current configuration save.
- **Update mode** — enter firmware update mode (similar to pressing the BOOT button).

---

## 🧩 Resource Usage

### RP2350
| Memory | Used | Total | % |
|--------|------|-------|-----|
| FLASH  | 515884 B | 4 MB  | 12.30% |
| RAM    | 244860 B | 512 KB| 46.70% |

### RP2040
| Memory | Used | Total | % |
|--------|------|-------|-----|
| FLASH  | 411896 B | 2 MB  | 19.64% |
| RAM    | 241484 B | 256 KB| 92.12% |

---

## 📂 Firmware Files

Pre-compiled firmware for different boards:
- `m1p1`, `m1p2`, `m2p1`, `m2p2`
- `z0p2` (RP2350B PiZero2)

---

## 🙏 Acknowledgments

- **AlexEkb** — for the emulator idea, initial code, and HDMI driver ([AlexEkb](https://github.com/AlexEkb4ever))
- **murmulator.ru** — for support and feedback [ZX_MURMULATOR](https://t.me/ZX_MURMULATOR)
- **Fastbeta** — for support and help at the beginning of the journey (but then you blocked me, sorry if something was wrong) ([https://t.me/zxpipico](https://t.me/zxpipico))
- **Vitaliy Rudik** — for the TR-DOS emulator idea ([https://bitbucket.org/rudolff/fdcduino/src/master/](https://bitbucket.org/rudolff/fdcduino/src/master/))
- **Z80 Emulator** by [Manuel Sainz](https://github.com/redcode/Z80)
- **Derek Fountain** — for the idea and code for very fast communication between two Picos ([Derek Fountain](https://github.com/derekfountain/pico-pio-connect))
- **Firmware author** — for the hard work and the "added bugs" ;)

---

## 🔗 Useful Links

- [ZX_MURMULATOR](https://t.me/ZX_MURMULATOR) — Telegram channel
- [murmulator.ru](https://murmulator.ru) — details about supported boards and additional information
- [Z80 Emulator Repository](https://github.com/redcode/Z80)

---

*If you forgot how something works — there's always `[F1]` Help.*


<div align="center">
  <img src="images/Screenshot 2026-03-28 11-36-59.png" alt="Speccy P" width="600">
  <br>
  <em>SpeccyP start</em>
</div>

<div align="center">
  <img src="images/Screenshot 2026-03-28 11-37-10.png" alt="Speccy P" width="600">
  <br>
  <em>SpeccyP at work</em>
</div>

<div align="center">
  <img src="images/Screenshot 2026-03-28 11-37-32.png" alt="Speccy P" width="600">
  <br>
  <em>SpeccyP at work</em>
</div>

<div align="center">
  <img src="images/Screenshot 2026-03-28 11-37-58.png" alt="Speccy P" width="600">
  <br>
  <em>SpeccyP at work</em>
</div>

<div align="center">
  <img src="images/Screenshot 2026-03-02 08-39-51.png" alt="Speccy P" width="600">
  <br>
  <em>Speccy P at work</em>
</div>
<div align="center">
  <img src="images/Screenshot 2026-03-28 11-54-16.png" alt="Speccy P" width="600">
  <br>
  <em>SpeccyP file menu</em>
</div>

<div align="center">
  <img src="DOCS/Murmulator_M1.JPG.webp" alt="Murmulator M1" width="600">
  <br>
  <em>MURMULATOR M1</em>
</div>

<div align="center">
  <img src="DOCS/Murmulator_M1_TFT_PSR.JPG.webp" alt="Murmulator M1 TFT" width="600">
  <br>
  <em>MURMULATOR M1 TFT</em>
</div>

<div align="center">
  <img src="DOCS/Murmulator2_38NJU24_.JPG.webp" alt="Murmulator M2" width="600">
  <br>
  <em>MURMULATOR M2</em>
</div>

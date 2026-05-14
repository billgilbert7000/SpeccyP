/*
 * SpeccyP — пины HSTX для платы M2 (RP2350).
 *
 * Базовая раскладка взята из FRANK NES
 * (https://github.com/rh1tech/frank-nes), который, в свою очередь,
 * вырос из pico_hdmi от fliperama86
 * (https://github.com/fliperama86/pico_hdmi).
 * SPDX-License-Identifier: Unlicense
 */

#ifndef HSTX_PINS_H
#define HSTX_PINS_H

// =============================================================================
// Пины DVI/HSTX (GPIO 12-19) — дефолтная раскладка периферии HSTX RP2350.
// =============================================================================
#define PIN_HSTX_CLK 12 // База пары CLK (12 = CLK-, 13 = CLK+)
#define PIN_HSTX_D0 14  // База пары D0  (14 = D0-,  15 = D0+)
#define PIN_HSTX_D1 16  // База пары D1  (16 = D1-,  17 = D1+)
#define PIN_HSTX_D2 18  // База пары D2  (18 = D2-,  19 = D2+)

#endif // HSTX_PINS_H

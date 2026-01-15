#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include "audio_i2s.pio.h"

// Конфигурация через define
/* #ifndef I2S_FREQ
#define I2S_FREQ 44100
#endif

#ifndef I2S_DATA_PIN
#define I2S_DATA_PIN 9
#endif

#ifndef I2S_CLK_BASE_PIN
#define I2S_CLK_BASE_PIN 10
#endif

#ifndef PIO_I2S
#define PIO_I2S pio1
#endif

#ifndef SM_I2S
#define SM_I2S 0
#endif

#ifndef TRANSFER_COUNT
#define TRANSFER_COUNT (1 << 31)
#endif */

// Упрощенный интерфейс
void i2s_init(void);
void i2s_deinit(void);
void i2s_out(uint16_t left, uint16_t right);

#ifdef __cplusplus
}
#endif
/*
 * SpeccyP — структуры и API для кодирования HDMI-пакетов под HSTX.
 *
 * Базовая реализация взята из FRANK NES
 * (https://github.com/rh1tech/frank-nes), который, в свою очередь,
 * вырос из pico_hdmi от fliperama86
 * (https://github.com/fliperama86/pico_hdmi).
 * SPDX-License-Identifier: Unlicense
 */

#ifndef HSTX_PACKET_H
#define HSTX_PACKET_H

#include <stdbool.h>
#include <stdint.h>

// Тайминги Data Island, всё в пиксельклоках.
#define W_GUARDBAND 2                                             // Guard band — 2 пиксельклока.
#define W_PREAMBLE 8                                              // Преамбула — 8 пиксельклоков.
#define W_DATA_PACKET 32                                          // Полезная нагрузка — 32 пиксельклока.
#define W_DATA_ISLAND (W_GUARDBAND + W_DATA_PACKET + W_GUARDBAND) // Итого 36.

// HSTX выдаёт по одному символу за слово.
#define HSTX_DATA_ISLAND_WORDS W_DATA_ISLAND // 36 слов на остров.

// Структура пакета — как в DVI/HSTX-спеке.
typedef struct {
    uint8_t header[4];       // 3 байта заголовка + 1 байт BCH-чётности.
    uint8_t subpacket[4][8]; // 4 субпакета по 7 байт + 1 байт BCH-чётности.
} hstx_packet_t;

// Готовый закодированный Data Island для HSTX (36 слов).
typedef struct {
    uint32_t words[HSTX_DATA_ISLAND_WORDS];
} hstx_data_island_t;

// Стерео-сэмпл.
typedef struct {
    int16_t left;
    int16_t right;
} audio_sample_t;

// ============================================================================
// Сборка пакетов
// ============================================================================

void hstx_packet_init(hstx_packet_t *packet);
void hstx_packet_set_acr(hstx_packet_t *packet, uint32_t n, uint32_t cts);
void hstx_packet_set_audio_infoframe(hstx_packet_t *packet, uint32_t sample_rate, uint8_t channels,
                                     uint8_t bits_per_sample);
void hstx_packet_set_avi_infoframe(hstx_packet_t *packet, uint8_t vic, uint8_t pixel_repetition);
int hstx_packet_set_audio_samples(hstx_packet_t *packet, const audio_sample_t *samples, int num_samples,
                                  int frame_count);
void hstx_packet_set_null(hstx_packet_t *packet);

// ============================================================================
// Кодирование TERC4 для HSTX
// ============================================================================

void hstx_encode_data_island(hstx_data_island_t *out, const hstx_packet_t *packet, bool vsync, bool hsync);
const uint32_t *hstx_get_null_data_island(bool vsync, bool hsync);

#endif // HSTX_PACKET_H

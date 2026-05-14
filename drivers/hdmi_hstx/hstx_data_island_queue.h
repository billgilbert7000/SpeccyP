/*
 * SpeccyP — публичный интерфейс очереди HDMI Data Islands.
 *
 * Базовая реализация взята из FRANK NES
 * (https://github.com/rh1tech/frank-nes), который, в свою очередь,
 * вырос из pico_hdmi от fliperama86
 * (https://github.com/fliperama86/pico_hdmi).
 * SPDX-License-Identifier: Unlicense
 */

#ifndef HSTX_DATA_ISLAND_QUEUE_H
#define HSTX_DATA_ISLAND_QUEUE_H

#include "hstx_packet.h"

#include <stdbool.h>
#include <stdint.h>

/** Поднимает очередь Data Islands и планировщик. */
void hstx_di_queue_init(void);

/**
 * Задаёт частоту аудио-сэмплов — нужна для расчёта тайминга пакетов.
 * @param sample_rate частота в Гц (например, 44100, 48000).
 */
void hstx_di_queue_set_sample_rate(uint32_t sample_rate);

/**
 * Кладёт уже закодированный Data Island в очередь. Возвращает true
 * при успехе и false, если очередь полна.
 */
bool hstx_di_queue_push(const hstx_data_island_t *island);

/** Сколько элементов сейчас в очереди. */
uint32_t hstx_di_queue_get_level(void);

/**
 * Сообщает планировщику число строк в кадре. Нужно вызывать при
 * смене видеорежима (525 для 480p, 262 для 240p и т.д.).
 */
void hstx_di_queue_set_v_total(uint32_t v_total);

/**
 * Прямо задаёт samples_per_line в формате 16.16 fixed-point — для
 * случаев, когда частота кадров не ровно 60 Гц и
 * hstx_di_queue_set_sample_rate() не подходит.
 */
void hstx_di_queue_set_samples_per_line_fp(uint32_t value);

/**
 * Тикает планировщик на один сканлайн. В DMA ISR должен вызываться
 * ровно раз на строку.
 */
void hstx_di_queue_tick(void);

/**
 * Возвращает следующий аудиопакет, если планировщик решил, что
 * пора. Иначе — NULL. Указатель — на массив из 36 слов HSTX data
 * island.
 */
const uint32_t *hstx_di_queue_get_audio_packet(void);

/**
 * Обновляет frame_counter в "тишинном" пакете. Полезно вызывать из
 * фоновой задачи на ядре 1 — пока очередь не успела опустеть, B-frame
 * последовательность остаётся валидной.
 */
void hstx_di_queue_update_silence(int frame_counter);

/**
 * Сколько раз scanout-ISR пришлось подсунуть пакет тишины, потому
 * что очередь оказалась пустой. Счётчик только растёт. Полезен для
 * диагностики back-pressure и просадок декодера.
 */
uint32_t hstx_di_queue_get_underrun_count(void);

#endif // HSTX_DATA_ISLAND_QUEUE_H

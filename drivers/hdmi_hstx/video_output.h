/*
 * SpeccyP — публичный интерфейс HSTX scanout-драйвера.
 *
 * Базовая реализация взята из FRANK NES
 * (https://github.com/rh1tech/frank-nes), который, в свою очередь,
 * вырос из pico_hdmi от fliperama86
 * (https://github.com/fliperama86/pico_hdmi).
 * SPDX-License-Identifier: Unlicense
 */

#ifndef VIDEO_OUTPUT_H
#define VIDEO_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

// ============================================================================
// Конфигурация видеорежима
// ============================================================================

// Альтернативный режим: 320x240. Включается -DVIDEO_MODE_320x240.
#ifdef VIDEO_MODE_320x240

// 1280x240 @ 60 Hz — настоящий 240p в "учетверённой" частоте.
// Пиксельклок 25.2 МГц (scan rate 15 кГц).
// H_TOTAL = 1600, V_TOTAL = 262.
// Active: 1280 (320×4), Front: 32 (8×4), Sync: 192 (48×4), Back: 96 (24×4).
#define MODE_H_FRONT_PORCH 32
#define MODE_H_SYNC_WIDTH 192
#define MODE_H_BACK_PORCH 96
#define MODE_H_ACTIVE_PIXELS 1280

#define MODE_V_FRONT_PORCH 4
#define MODE_V_SYNC_WIDTH 4
#define MODE_V_BACK_PORCH 14
#define MODE_V_ACTIVE_LINES 240

// Делитель clk_sys для clk_hstx: clk_sys/N = 126 МГц, дальше CSR_CLKDIV=5
// даёт пиксельклок 25.2 МГц. Если sys_clk не 126 МГц — задавайте свой
// MODE_HSTX_CLK_DIV через -D (например, для 252 МГц возьмите DIV=2).

//#ifndef MODE_HSTX_CLK_DIV
//#define MODE_HSTX_CLK_DIV 1
//#endif

#ifndef MODE_HSTX_CSR_CLKDIV
#define MODE_HSTX_CSR_CLKDIV 5
#endif

#else

// 640x480 @ 60 Hz (VIC 1) — режим по умолчанию.
// Пиксельклок 25.2 МГц.
#define MODE_H_FRONT_PORCH 16
#define MODE_H_SYNC_WIDTH 96
#define MODE_H_BACK_PORCH 48
#define MODE_H_ACTIVE_PIXELS 640

#define MODE_V_FRONT_PORCH 10
#define MODE_V_SYNC_WIDTH 2
#define MODE_V_BACK_PORCH 33
#define MODE_V_ACTIVE_LINES 480

// Делитель HSTX, переопределяется -D если sys_clk не 126 МГц.
//#ifndef MODE_HSTX_CLK_DIV
//#define MODE_HSTX_CLK_DIV 1
//#endif
#ifndef MODE_HSTX_CSR_CLKDIV
#define MODE_HSTX_CSR_CLKDIV 5
#endif

#endif

#define MODE_H_TOTAL_PIXELS (MODE_H_FRONT_PORCH + MODE_H_SYNC_WIDTH + MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS)
#define MODE_V_TOTAL_LINES (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + MODE_V_BACK_PORCH + MODE_V_ACTIVE_LINES)

// Размер кадра — выставляется в video_output_init().
extern uint16_t frame_width;
extern uint16_t frame_height;

// ============================================================================
// Глобальное состояние
// ============================================================================

extern volatile uint32_t video_frame_count;

// ============================================================================
// Публичный интерфейс
// ============================================================================

typedef void (*video_output_task_fn)(void);
typedef void (*video_output_vsync_cb_t)(void);

/**
 * Scanline-callback: вызывается, когда пора отдать пиксели очередной
 * строки.
 *
 * @param v_scanline   Номер сканлайна 0..MODE_V_TOTAL_LINES-1.
 * @param active_line  Активная строка 0..MODE_V_ACTIVE_LINES-1
 *                     (имеет смысл только в активной зоне).
 * @param line_buffer  Буфер под MODE_H_ACTIVE_PIXELS пикселей RGB565,
 *                     упакованных по два в uint32_t. Должен быть
 *                     заполнен ровно (MODE_H_ACTIVE_PIXELS/2) словами:
 *                       - 640x480: 640 пикселей = 320 слов
 *                       - 320x240: 1280 пикселей = 640 слов (×4 повтор)
 */
typedef void (*video_output_scanline_cb_t)(uint32_t v_scanline, uint32_t active_line, uint32_t *line_buffer);

/**
 * Поднимает HSTX и DMA. width/height — размер источникового кадра.
 */
void video_output_init(uint16_t width, uint16_t height);

/** Регистрирует scanline-callback. */
void video_output_set_scanline_callback(video_output_scanline_cb_t cb);

/** Регистрирует VSYNC-callback (вызывается раз в кадр в начале vsync). */
void video_output_set_vsync_callback(video_output_vsync_cb_t cb);

/**
 * Регистрирует фоновую задачу для цикла на ядре 1 (обычно сюда
 * подвешивают обработку звука).
 */
void video_output_set_background_task(video_output_task_fn task);

/**
 * @return true если включён режим DVI (без HDMI audio), иначе false.
 */
bool video_output_get_dvi_mode(void);

/**
 * Включает/выключает DVI. В DVI HDMI Data Islands не выдаются (значит,
 * нет и HDMI-audio). По умолчанию HSTX-драйвер стартует в DVI — часть
 * мониторов на Data Islands не цепляется.
 */
void video_output_set_dvi_mode(bool enabled);

/**
 * Точка входа на ядро 1. Не возвращается.
 */
void video_output_core1_run(void);

/**
 * Настраивает HSTX, пины, DMA-каналы и IRQ под scanout, затем стартует
 * DMA-цепочку и сразу возвращает управление. Дальше всё крутит DMA
 * IRQ.
 *
 * Этот вариант используется, когда ядро 1 нужно занять чем-то другим
 * (IRQ работает на том ядре, где он включён — обычно ядро 0).
 */
void video_output_setup(void);

/**
 * Меняет частоту HDMI audio. Перепаковывает ACR, Audio InfoFrame и
 * пересчитывает темп пакетов. Можно вызывать уже после
 * video_output_init() — там по умолчанию ставится 48 кГц.
 */
void pico_hdmi_set_audio_sample_rate(uint32_t sample_rate);

#endif // VIDEO_OUTPUT_H

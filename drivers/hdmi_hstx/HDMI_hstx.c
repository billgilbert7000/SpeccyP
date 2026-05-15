/*
 * SpeccyP — HDMI-драйвер поверх HSTX (родная периферия RP2350).
 * Замена для legacy PIO-варианта (src/HDMI.c), включается флагом
 * сборки HDMI_HSTX. Снаружи виден тот же набор graphics_*() /
 * startVIDEO(), что и у PIO-версии.
 *
 * Формат кадрового буфера эмулятора g_gbuf: 4 бита на пиксель,
 * упакованных по два в байт:
 *   байт b по адресу row*W/2 + x/2:
 *     младший ниббл — пиксель x   (чётный)
 *     старший ниббл — пиксель x+1 (нечётный)
 * Один байт — это два пикселя по горизонтали; шаг строки SCREEN_W/2.
 *
 * Режим — 640x480@60 Hz (HDMI VIC=1), такой же, как у legacy
 * PIO-драйвера. Источник 320x240 удваивается по горизонтали и
 * вертикали и аккуратно ложится в активную область 640x480.
 *
 * Звук в HDMI-режиме идёт через Data Islands (HDMI 1.3a §5.3),
 * подробности — в HDMI_hstx_audio.c. Эмулятор кладёт сэмплы по
 * одному через i2s_out(), шим набирает по 4 и упаковывает в один
 * Data Island. По умолчанию мы стартуем в DVI-режиме (см.
 * graphics_init()), и звук идёт по обычным PWM/I2S-пинам.
 */

#include "HDMI.h"
#include "video_output.h"
#include "hstx_data_island_queue.h"

#include "pico/stdlib.h"
#include "hardware/clocks.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SPECCY_HDMI_W 320
#define SPECCY_HDMI_H 240

/* RGB565-палитра, синхронная с RGB888-палитрой эмулятора. Держим её
 * в обычной SRAM, не в scratch_y: scratch_y привязан к ядру 1, а DMA
 * ISR HSTX мы запускаем на ядре 0 (ядро 1 занято Z80). */
static uint16_t palette565[256];

static uint8_t * volatile graphics_buffer = NULL;
static volatile uint32_t graphics_vsync_count = 0;

#define ACTIVE_PIXELS_PER_LINE 640
#define ACTIVE_WORDS_PER_LINE  (ACTIVE_PIXELS_PER_LINE / 2)

/* =========================================================================
 * Публичный API. Сигнатуры один в один с src/HDMI.c.
 * =========================================================================*/

void graphics_set_buffer(uint8_t *buffer)
{
    graphics_buffer = buffer;
}

uint8_t* graphics_get_buffer(void)
{
    return (uint8_t*)graphics_buffer;
}

static inline uint16_t rgb888_to_rgb565(uint32_t color888)
{
    uint8_t r = (color888 >> 16) & 0xFF;
    uint8_t g = (color888 >>  8) & 0xFF;
    uint8_t b = (color888      ) & 0xFF;
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void graphics_set_palette(uint8_t i, uint32_t color888)
{
    /* В PIO-пути индексы 240..243 шли под шаблоны sync. У нас sync
     * собирается отдельно, поэтому такие "служебные" индексы просто
     * игнорируем. */
    if ((i >= 240) && (i != 255)) return;
    palette565[i] = rgb888_to_rgb565(color888);
}

/* =========================================================================
 * Scanline-callback HSTX. Вызывается из DMA ISR, поэтому функция
 * должна лежать в SRAM, не во flash. На вход — 320 слов по 32 бита,
 * на выход — два пикселя RGB565 в каждом слове, итого 640 пикселей
 * активной строки.
 *
 * Источник упакован 4bpp (байт = 2 пикселя). Одна и та же строка
 * исходника идёт на две выходные подряд — это line doubling по
 * вертикали.
 * =========================================================================*/

static void __not_in_flash_func(speccy_scanline_cb)(uint32_t v_scanline,
                                                    uint32_t active_line,
                                                    uint32_t *line_buf)
{
    (void)v_scanline;

    const uint8_t *fb = (const uint8_t *)graphics_buffer;

    int src_y = (int)active_line >> 1;

    if (!fb || src_y >= SPECCY_HDMI_H) {
        for (int i = 0; i < ACTIVE_WORDS_PER_LINE; i++) line_buf[i] = 0;
        return;
    }

    const uint8_t *src = fb + src_y * (SPECCY_HDMI_W / 2);
    uint32_t *dst = line_buf;

    /* Каждый исходный байт превращается в два 32-битных слова: одно
     * слово — один исходный пиксель, дублированный в обе 16-битные
     * половины (это и есть удвоение по горизонтали). На выходе из
     * 320 пикселей получается 640. */
    for (int x = 0; x < SPECCY_HDMI_W / 2; x++) {
        uint8_t b = src[x];
        uint16_t c0 = palette565[b & 0x0F];
        uint16_t c1 = palette565[b >> 4];
        *dst++ = (uint32_t)c0 | ((uint32_t)c0 << 16);
        *dst++ = (uint32_t)c1 | ((uint32_t)c1 << 16);
    }
}

static void __not_in_flash_func(speccy_vsync_cb)(void)
{
    graphics_vsync_count++;
}

/* =========================================================================
 * Инициализация. clk_hstx настраивается через MODE_HSTX_CLK_DIV (его
 * задаёт CMake под текущий CPU_MHZ так, чтобы clk_sys/N = 126 МГц —
 * пиксельклок). Очередь Data Islands поднимаем до старта scanout —
 * иначе DMA ISR на старте не из чего будет даже "тишину" собрать.
 *
 * Ядро 1 у нас отдано эмулятору под ZXThread, поэтому
 * video_output_core1_run() звать нельзя — он не возвращается.
 * Используем video_output_setup(): он настраивает HSTX, цепочки DMA и
 * IRQ и отдаёт управление обратно. Дальше вся работа крутится в
 * обработчике прерывания на том ядре, где IRQ был включён, то есть
 * на ядре 0.
 * =========================================================================*/

void graphics_init(g_out out_mode)
{
    (void)out_mode;

    /* Дефолтная серая шкала. До первого set_palette() / явного
     * graphics_set_palette() в палитре уже что-то осмысленное. */
    for (int i = 0; i < 240; i++) {
        graphics_set_palette((uint8_t)i, (uint32_t)((i << 16) | (i << 8) | i));
    }

    hstx_di_queue_init();

    video_output_set_vsync_callback(speccy_vsync_cb);
    video_output_set_scanline_callback(speccy_scanline_cb);
    video_output_init(SPECCY_HDMI_W, SPECCY_HDMI_H);

    /* По умолчанию идёт полноценный HDMI: AVI InfoFrame, ACR, Audio
     * InfoFrame, аудио-сэмплы в Data Islands. Звук в этом режиме —
     * через тот же i2s_out(), что и у legacy PIO, только пакеты
     * упаковываются в HDMI и едут вместе с видео.
     *
     * Если приёмник не понимает Data Islands и теряет синхру —
     * соберите с -DHDMI_HSTX_DVI=1, тогда HSTX стартует в чистом
     * DVI-режиме (без HDMI audio), как старый PIO-драйвер. Звук в
     * этом случае идёт по обычным PWM/I2S-пинам. */
#ifdef HDMI_HSTX_DVI
    video_output_set_dvi_mode(true);
#endif

    video_output_setup();

    sleep_ms(50);
}

/* =========================================================================
 * startVIDEO / set_palette — то, что дёргает эмулятор, когда нужно
 * поднять выбранный видеовыход и палитру. Эта реализация поддерживает
 * только HDMI; запросы VGA/TFT молча игнорируем.
 * =========================================================================*/

void startVIDEO(uint8_t vol)
{
    extern uint8_t g_gbuf[];

    /* VIDEO_HDMI == 2 (см. src/config.h). Разворачиваем константу
     * прямо здесь, чтобы не тащить внутренние заголовки SpeccyP в
     * драйвер. */
    if (vol == 2) {
        graphics_init(g_out_HDMI);
        graphics_set_buffer(g_gbuf);
        set_palette(0);
    }
}

void set_palette(uint8_t n)
{
    if (n == 0) {
        for (int i = 0; i < 16; i++) {
            int I = (i >> 3) & 1;
            int G = (i >> 2) & 1;
            int R = (i >> 1) & 1;
            int B = (i >> 0) & 1;
            uint32_t RGB = ((R ? (I ? 255 : 170) : 0) << 16)
                         | ((G ? (I ? 255 : 170) : 0) << 8)
                         | ((B ? (I ? 255 : 170) : 0));
            graphics_set_palette((uint8_t)i, RGB);
        }
        return;
    }
    n--;
    for (int i = 0; i < 16; i++) {
        graphics_set_palette((uint8_t)i, tab_color[n][i]);
    }
}

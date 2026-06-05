/*
 * SpeccyP — HSTX HDMI driver, scanout.
 *
 * Основа взята из FRANK NES (https://github.com/rh1tech/frank-nes),
 * который, в свою очередь, вырос из pico_hdmi от fliperama86
 * (https://github.com/fliperama86/pico_hdmi).
 * SPDX-License-Identifier: Unlicense
 *
 * Режим — 640x480@60 Hz, выводится через HSTX RP2350. Два DMA-канала
 * (PING и PONG) гоняют команды в HSTX FIFO; sync, преамбулы, guard
 * band и Data Islands лежат в SRAM-буферах cmd-list и переключаются
 * по chain. На каждой строке scanline-callback дёргает клиентский
 * код, чтобы тот наполнил line_buffer пикселями.
 */

#include "video_output.h"

#include "hstx_data_island_queue.h"
#include "hstx_packet.h"
#include "hstx_pins.h"

#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"

#include <math.h>
#include <string.h>

// ============================================================================
// Константы DVI/HSTX
// ============================================================================

#define TMDS_CTRL_00 0x354u // vsync=0 hsync=0
#define TMDS_CTRL_01 0x0abu // vsync=0 hsync=1
#define TMDS_CTRL_10 0x154u // vsync=1 hsync=0
#define TMDS_CTRL_11 0x2abu // vsync=1 hsync=1

// Сигналы синхронизации: lane 0 несёт sync, lanes 1 и 2 всегда CTRL_00
#define SYNC_V0_H0 (TMDS_CTRL_00 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V0_H1 (TMDS_CTRL_01 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H0 (TMDS_CTRL_10 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))
#define SYNC_V1_H1 (TMDS_CTRL_11 | (TMDS_CTRL_00 << 10) | (TMDS_CTRL_00 << 20))

// Преамбула Data Island: lane 0 = sync, lanes 1 и 2 = шаблон CTRL_01.
// HDMI 1.3a Table 5-2: CTL0=1, CTL1=0, CTL2=1, CTL3=0.
#define PREAMBLE_V0_H0 (TMDS_CTRL_00 | (TMDS_CTRL_01 << 10) | (TMDS_CTRL_01 << 20))
#define PREAMBLE_V1_H0 (TMDS_CTRL_10 | (TMDS_CTRL_01 << 10) | (TMDS_CTRL_01 << 20))

// Преамбула видео: lane 0 = sync, lane 1 = CTRL_01, lane 2 = CTRL_00.
// HDMI 1.3a Table 5-2: CTL0=1, CTL1=0, CTL2=0, CTL3=0.
#define VIDEO_PREAMBLE_V0_H1 (TMDS_CTRL_01 | (TMDS_CTRL_01 << 10) | (TMDS_CTRL_00 << 20))
#define VIDEO_PREAMBLE_V1_H1 (TMDS_CTRL_11 | (TMDS_CTRL_01 << 10) | (TMDS_CTRL_00 << 20))

// Video guard band — по HDMI 1.3a Table 5-5.
// CH0 = 0b1011001100 (0x2CC), CH1 = 0b0100110011 (0x133), CH2 = 0b1011001100 (0x2CC).
#define VIDEO_GUARD_BAND (0x2CCu | (0x133u << 10) | (0x2CCu << 20))

#define HSTX_CMD_RAW (0x0u << 12)
#define HSTX_CMD_RAW_REPEAT (0x1u << 12)
#define HSTX_CMD_TMDS (0x2u << 12)
#define HSTX_CMD_TMDS_REPEAT (0x3u << 12)
#define HSTX_CMD_NOP (0xfu << 12)

#define SYNC_AFTER_DI (MODE_H_SYNC_WIDTH - W_PREAMBLE - W_DATA_ISLAND)

// Длительности video preamble и guard band (HDMI 1.3a §5.2.2).
#define W_VIDEO_PREAMBLE 8
#define W_VIDEO_GUARD_BAND 2

// ============================================================================
// Состояние аудио/видео
// ============================================================================

uint16_t frame_width = 0;
uint16_t frame_height = 0;
volatile uint32_t video_frame_count = 0;

/* DVI-режим. true — Data Islands не выдаются, на выходе чистый DVI без
 * звука. Для совместимости с legacy PIO-путём HSTX по умолчанию
 * стартует именно так (см. graphics_init() в HDMI_hstx.c) — часть
 * мониторов на Data Islands не цепляется. */
static bool dvi_mode = false;

/* Два буфера на сканлайн. Главный цикл на core0 может надолго
 * затормозить DMA IRQ (PS/2, USB CDC, меню), и если scanline_cb не
 * успеет добить line_buffer до того, как DMA начнёт его читать —
 * получим обрывки прошлого кадра пополам с нулями. Делаем ping/pong:
 * DMA читает уже готовый буфер, callback пишет следующий. */
static uint16_t line_buffer_a[MODE_H_ACTIVE_PIXELS] __attribute__((aligned(4)));
static uint16_t line_buffer_b[MODE_H_ACTIVE_PIXELS] __attribute__((aligned(4)));
static uint16_t *line_buffer_active = line_buffer_a; /* читается DMA */
static uint16_t *line_buffer_fill   = line_buffer_b; /* пишется callback */
static uint32_t v_scanline = 2;
static bool vactive_cmdlist_posted = false;
static bool dma_pong = false;

static video_output_task_fn background_task = NULL;
static video_output_scanline_cb_t scanline_callback = NULL;
static video_output_vsync_cb_t vsync_callback = NULL;

#define DMACH_PING 0
#define DMACH_PONG 1

// ============================================================================
// Списки команд DMA для HSTX
// ============================================================================

// Чистый DVI (без Data Islands).
static uint32_t vblank_line_vsync_off[] = {HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH,
                                           SYNC_V1_H1,
                                           HSTX_CMD_NOP,
                                           HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,
                                           SYNC_V1_H0,
                                           HSTX_CMD_NOP,
                                           HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS),
                                           SYNC_V1_H1,
                                           HSTX_CMD_NOP};

static uint32_t vblank_line_vsync_on[] = {HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH,
                                          SYNC_V0_H1,
                                          HSTX_CMD_NOP,
                                          HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,
                                          SYNC_V0_H0,
                                          HSTX_CMD_NOP,
                                          HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS),
                                          SYNC_V0_H1,
                                          HSTX_CMD_NOP};

// Активная строка в DVI-режиме (без Data Island, только sync + пиксели).
static uint32_t vactive_line_dvi[] = {
    HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH, SYNC_V1_H1, HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | MODE_H_SYNC_WIDTH,  SYNC_V1_H0, HSTX_CMD_NOP,
    HSTX_CMD_RAW_REPEAT | MODE_H_BACK_PORCH,  SYNC_V1_H1, HSTX_CMD_TMDS | MODE_H_ACTIVE_PIXELS};

static uint32_t vactive_di_ping[128], vactive_di_pong[128], vactive_di_null[128];
static uint32_t vactive_di_len, vactive_di_null_len;

static uint32_t vblank_di_ping[128], vblank_di_pong[128], vblank_di_null[128];
static uint32_t vblank_di_len, vblank_di_null_len;

static uint32_t vblank_acr_vsync_on[64], vblank_acr_vsync_on_len;
static uint32_t vblank_acr_vsync_off[64], vblank_acr_vsync_off_len;
static uint32_t vblank_infoframe_vsync_on[64], vblank_infoframe_vsync_on_len;
static uint32_t vblank_infoframe_vsync_off[64], vblank_infoframe_vsync_off_len;
static uint32_t vblank_avi_infoframe[64], vblank_avi_infoframe_len;

// ============================================================================
// Resync: гасим HSTX и заводим его с первой строки кадра заново.
// ============================================================================

static void __scratch_x("") hstx_resync(void)
{
    // Останавливаем обе цепочки DMA.
    dma_channel_abort(DMACH_PING);
    dma_channel_abort(DMACH_PONG);

    // Гасим HSTX. Сбрасываются shift-регистр, генератор тактов и FIFO.
    hstx_ctrl_hw->csr &= ~HSTX_CTRL_CSR_EN_BITS;

    // Несколько nop'ов, чтобы дать периферии действительно остановиться.
    __asm volatile("nop\nnop\nnop\nnop");

    // Состояние на начало кадра.
    v_scanline = 0;
    vactive_cmdlist_posted = false;
    dma_pong = false;

    // Сбрасываем висящие флаги DMA IRQ.
    dma_hw->ints0 = (1U << DMACH_PING) | (1U << DMACH_PONG);

    // PING — на line 0 нового кадра.
    dma_channel_hw_t *ch_ping = &dma_hw->ch[DMACH_PING];
    ch_ping->read_addr = (uintptr_t)vblank_line_vsync_off;
    ch_ping->transfer_count = count_of(vblank_line_vsync_off);

    // PONG — на line 1, чтобы chain после PING нашёл готовую цепочку.
    dma_channel_hw_t *ch_pong = &dma_hw->ch[DMACH_PONG];
    ch_pong->read_addr = (uintptr_t)vblank_line_vsync_off; // line 1 тоже пустая
    ch_pong->transfer_count = count_of(vblank_line_vsync_off);

    // Включаем HSTX и пускаем DMA.
    hstx_ctrl_hw->csr |= HSTX_CTRL_CSR_EN_BITS;
    dma_channel_start(DMACH_PING);
}

// ============================================================================
// Вспомогательные функции
// ============================================================================

static uint32_t __scratch_x("hdmi") build_line_with_di(uint32_t *buf, const uint32_t *di_words, bool vsync, bool active)
{
    uint32_t *p = buf;
    uint32_t sync_h0 = vsync ? SYNC_V0_H0 : SYNC_V1_H0;
    uint32_t sync_h1 = vsync ? SYNC_V0_H1 : SYNC_V1_H1;
    uint32_t preamble = vsync ? PREAMBLE_V0_H0 : PREAMBLE_V1_H0;

    *p++ = HSTX_CMD_RAW_REPEAT | MODE_H_FRONT_PORCH;
    *p++ = sync_h1;
    *p++ = HSTX_CMD_NOP;

    *p++ = HSTX_CMD_RAW_REPEAT | W_PREAMBLE;
    *p++ = preamble;
    *p++ = HSTX_CMD_NOP;

    *p++ = HSTX_CMD_RAW | W_DATA_ISLAND;
    for (int i = 0; i < W_DATA_ISLAND; i++)
        *p++ = di_words[i];
    *p++ = HSTX_CMD_NOP;

    *p++ = HSTX_CMD_RAW_REPEAT | SYNC_AFTER_DI;
    *p++ = sync_h0;
    *p++ = HSTX_CMD_NOP;

    if (active) {
        // HDMI 1.3a §5.2.2: перед Video Data Period идут preamble и guard band.
        uint32_t video_preamble = vsync ? VIDEO_PREAMBLE_V0_H1 : VIDEO_PREAMBLE_V1_H1;

        // Control-период: back porch без места под preamble/guard band.
        *p++ = HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH - W_VIDEO_PREAMBLE - W_VIDEO_GUARD_BAND);
        *p++ = sync_h1;
        *p++ = HSTX_CMD_NOP;

        // 8 пикселей video preamble.
        *p++ = HSTX_CMD_RAW_REPEAT | W_VIDEO_PREAMBLE;
        *p++ = video_preamble;
        *p++ = HSTX_CMD_NOP;

        // 2 пикселя guard band.
        *p++ = HSTX_CMD_RAW_REPEAT | W_VIDEO_GUARD_BAND;
        *p++ = VIDEO_GUARD_BAND;

        // Активные пиксели.
        *p++ = HSTX_CMD_TMDS | MODE_H_ACTIVE_PIXELS;
    } else {
        *p++ = HSTX_CMD_RAW_REPEAT | (MODE_H_BACK_PORCH + MODE_H_ACTIVE_PIXELS);
        *p++ = sync_h1;
        *p++ = HSTX_CMD_NOP;
    }
    return (uint32_t)(p - buf);
}

typedef struct {
    bool vsync_active;
    bool front_porch;
    bool back_porch;
    bool active_video;
    bool send_acr;
    uint32_t active_line;
} scanline_state_t;

static inline void __scratch_x("") get_scanline_state(uint32_t v_scanline, scanline_state_t *state)
{
    state->vsync_active = (v_scanline >= MODE_V_FRONT_PORCH && v_scanline < (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH));
    state->front_porch = (v_scanline < MODE_V_FRONT_PORCH);
    state->back_porch = (v_scanline >= MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH &&
                         v_scanline < MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH + MODE_V_BACK_PORCH);
    state->active_video = (!state->vsync_active && !state->front_porch && !state->back_porch);

    state->send_acr = (v_scanline >= (MODE_V_FRONT_PORCH + MODE_V_SYNC_WIDTH) &&
                       v_scanline < (MODE_V_TOTAL_LINES - MODE_V_ACTIVE_LINES) && (v_scanline % 4 == 0));

    if (state->active_video) {
        state->active_line = v_scanline - (MODE_V_TOTAL_LINES - MODE_V_ACTIVE_LINES);
    } else {
        state->active_line = 0;
    }
}

static inline void __scratch_x("") video_output_handle_vsync(dma_channel_hw_t *ch, uint32_t v_scanline)
{
    if (dvi_mode) {
        // Чистый DVI: vsync-строка без Data Islands.
        ch->read_addr = (uintptr_t)vblank_line_vsync_on;
        ch->transfer_count = count_of(vblank_line_vsync_on);
        if (v_scanline == MODE_V_FRONT_PORCH) {
            video_frame_count++;
            if (vsync_callback)
                vsync_callback();
        }
    } else {
        if (v_scanline == MODE_V_FRONT_PORCH) {
            ch->read_addr = (uintptr_t)vblank_acr_vsync_on;
            ch->transfer_count = vblank_acr_vsync_on_len;
            video_frame_count++;
            if (vsync_callback)
                vsync_callback();
        } else {
            ch->read_addr = (uintptr_t)vblank_infoframe_vsync_on;
            ch->transfer_count = vblank_infoframe_vsync_on_len;
        }
    }
}

static inline void __scratch_x("")
    video_output_handle_active_start(dma_channel_hw_t *ch, uint32_t v_scanline, uint32_t active_line, bool dma_pong)
{
    /* Заполняем буфер _fill пикселями следующей строки. DMA в этот
     * момент может всё ещё читать буфер _active (предыдущей строки) —
     * это безопасно, мы не пишем в _active. */
    uint32_t *dst32 = (uint32_t *)line_buffer_fill;

    if (scanline_callback) {
        scanline_callback(v_scanline, active_line, dst32);
    } else {
        // Callback не назначен — заливаем строку чёрным.
        for (uint32_t i = 0; i < MODE_H_ACTIVE_PIXELS / 2; i++) {
            dst32[i] = 0;
        }
    }

    if (dvi_mode) {
        // DVI без Data Islands.
        ch->read_addr = (uintptr_t)vactive_line_dvi;
        ch->transfer_count = count_of(vactive_line_dvi);
    } else {
        uint32_t *buf = dma_pong ? vactive_di_ping : vactive_di_pong;
        const uint32_t *di_words = hstx_di_queue_get_audio_packet();
        if (di_words) {
            /* Длину сначала считаем в локальную переменную, потом
             * одним стором кладём в transfer_count. vactive_di_len
             * шарится с другим ping/pong-слотом, и параллельная
             * DMA-цепочка не должна успеть прочитать её на полпути. */
            uint32_t len = build_line_with_di(buf, di_words, false, true);
            vactive_di_len = len;
            ch->read_addr = (uintptr_t)buf;
            ch->transfer_count = len;
        } else {
            ch->read_addr = (uintptr_t)vactive_di_null;
            ch->transfer_count = vactive_di_null_len;
        }
    }
}

static inline void __scratch_x("")
    video_output_handle_blanking(dma_channel_hw_t *ch, uint32_t v_scanline, bool send_acr, bool dma_pong)
{
    if (dvi_mode) {
        // DVI: пустая строка без Data Islands.
        (void)send_acr;
        (void)dma_pong;
        (void)v_scanline;
        ch->read_addr = (uintptr_t)vblank_line_vsync_off;
        ch->transfer_count = count_of(vblank_line_vsync_off);
    } else {
        if (send_acr) {
            ch->read_addr = (uintptr_t)vblank_acr_vsync_off;
            ch->transfer_count = vblank_acr_vsync_off_len;
        } else if (v_scanline == 0) {
            ch->read_addr = (uintptr_t)vblank_avi_infoframe;
            ch->transfer_count = vblank_avi_infoframe_len;
        } else {
            const uint32_t *di_words = hstx_di_queue_get_audio_packet();
            if (di_words) {
                uint32_t *buf = dma_pong ? vblank_di_ping : vblank_di_pong;
                vblank_di_len = build_line_with_di(buf, di_words, false, false);
                ch->read_addr = (uintptr_t)buf;
                ch->transfer_count = vblank_di_len;
            } else {
                ch->read_addr = (uintptr_t)vblank_di_null;
                ch->transfer_count = vblank_di_null_len;
            }
        }
    }
}

static inline void __scratch_x("") video_output_handle_active_data(dma_channel_hw_t *ch)
{
    /* Меняем указатели местами: _fill, который только что наполнил
     * callback, теперь идёт в DMA как _active, а прежний _active
     * освобождается под следующую строку. */
    uint16_t *swap = line_buffer_active;
    line_buffer_active = line_buffer_fill;
    line_buffer_fill   = swap;

    /* DMB перед сменой read_addr нужен, чтобы DMA увидел все записи
     * scanline_callback в line_buffer_fill (они пришли в active_start
     * IRQ, до этого момента). Без барьера на левом краю строки
     * периодически вылезали чёрные точки и обрывки прошлого кадра. */
    __asm volatile ("dmb sy" ::: "memory");

    ch->read_addr = (uintptr_t)line_buffer_active;
    ch->transfer_count = (MODE_H_ACTIVE_PIXELS * sizeof(uint16_t)) / sizeof(uint32_t);
}

// ============================================================================
// Обработчик DMA IRQ
// ============================================================================

void __scratch_x("") dma_irq_handler()
{
    uint32_t ch_num = dma_pong ? DMACH_PONG : DMACH_PING;
    dma_channel_hw_t *ch = &dma_hw->ch[ch_num];
    dma_hw->intr = 1U << ch_num;
    dma_pong = !dma_pong;

    scanline_state_t state;
    get_scanline_state(v_scanline, &state);

    if (state.vsync_active) {
        video_output_handle_vsync(ch, v_scanline);
    } else if (state.active_video && !vactive_cmdlist_posted) {
        video_output_handle_active_start(ch, v_scanline, state.active_line, dma_pong);
        vactive_cmdlist_posted = true;
    } else if (state.active_video && vactive_cmdlist_posted) {
        video_output_handle_active_data(ch);
        vactive_cmdlist_posted = false;
    } else {
        video_output_handle_blanking(ch, v_scanline, state.send_acr, dma_pong);
    }
    if (!vactive_cmdlist_posted) {
        // Audio/Data-island scheduler tick belongs at scanline boundary,
        // i.e. exactly once per v_scanline advance. Original FRANK-NES
        // gated by !vactive_cmdlist_posted alone, which incorrectly
        // ticked twice per vblank line (no active-cmdlist there) and
        // gave a consumer rate of ~104 kHz instead of nominal 96 kHz —
        // queue underran ~8% steady-state and produced periodic skips.
        if (!dvi_mode) {
            hstx_di_queue_tick();
        }
        v_scanline = (v_scanline + 1) % MODE_V_TOTAL_LINES;
    }
}

// ============================================================================
// Публичный интерфейс
// ============================================================================

// ACR-параметры N и CTS для пиксельклока 25.2 МГц. HDMI spec, Table 7-1/7-2.
static void get_acr_params(uint32_t sample_rate, uint32_t *n, uint32_t *cts)
{
    switch (sample_rate) {
        case 32000:
            *n = 4096;
            *cts = 25200;
            break;
        case 44100:
            *n = 6272;
            *cts = 28000;
            break;
        case 48000:
            *n = 6144;
            *cts = 25200;
            break;
        case 88200:
            *n = 12544;
            *cts = 28000;
            break;
        case 96000:
            *n = 12288;
            *cts = 25200;
            break;
        case 176400:
            *n = 25088;
            *cts = 28000;
            break;
        case 192000:
            *n = 24576;
            *cts = 25200;
            break;
        default:
            // Если частота не совпала ни с одной табличной — берём 48 кГц.
            *n = 6144;
            *cts = 25200;
            break;
    }
}

static void configure_audio_packets(uint32_t sample_rate)
{
    hstx_di_queue_set_sample_rate(sample_rate);

    hstx_packet_t packet;
    hstx_data_island_t island;

    uint32_t acr_n;
    uint32_t acr_cts;
    get_acr_params(sample_rate, &acr_n, &acr_cts);
    hstx_packet_set_acr(&packet, acr_n, acr_cts);
    hstx_encode_data_island(&island, &packet, true, true);
    vblank_acr_vsync_on_len = build_line_with_di(vblank_acr_vsync_on, island.words, true, false);
    hstx_encode_data_island(&island, &packet, false, true);
    vblank_acr_vsync_off_len = build_line_with_di(vblank_acr_vsync_off, island.words, false, false);

    hstx_packet_set_audio_infoframe(&packet, sample_rate, 2, 16);
    hstx_encode_data_island(&island, &packet, true, true);
    vblank_infoframe_vsync_on_len = build_line_with_di(vblank_infoframe_vsync_on, island.words, true, false);
    hstx_encode_data_island(&island, &packet, false, true);
    vblank_infoframe_vsync_off_len = build_line_with_di(vblank_infoframe_vsync_off, island.words, false, false);
}

void video_output_init(uint16_t width, uint16_t height)
{
    frame_width = width;
    frame_height = height;

    // clk_hstx настраиваем под текущий режим. set_sys_clock_khz()
    // ломает этот клок, так что после смены sys_clock сюда нужно
    // приходить ещё раз.
    uint32_t sys_freq = clock_get_hz(clk_sys);


 /* # HSTX хочет целочисленный делитель clk_sys в свой 126 МГц домен
    # пиксельклока. CPU_MHZ выставляется выше через SPECCY_CONFIG_DEFINITIONS;
    # пересчитываем его в MODE_HSTX_CLK_DIV (504/4=126, 252/2=126 и т.д.). */
    uint8_t hstx_div_clock = 1;
    if (sys_freq==504000000)      hstx_div_clock = 4;
    else if (sys_freq==378000000) hstx_div_clock = 3;
    else if (sys_freq==252000000) hstx_div_clock = 2;

    clock_configure_int_divider(clk_hstx,
                                0, // без glitchless mux
                                CLOCKS_CLK_HSTX_CTRL_AUXSRC_VALUE_CLK_SYS, sys_freq, hstx_div_clock );

    // Берём под HSTX каналы DMA 0 и 1.
    dma_channel_claim(DMACH_PING);
    dma_channel_claim(DMACH_PONG);

    // По v_total планировщик считает темп аудио-пакетов.
    hstx_di_queue_set_v_total(MODE_V_TOTAL_LINES);

    // Заранее собираем аудио-пакеты на 48 кГц.
    configure_audio_packets(48000);

    hstx_packet_t packet;
    hstx_data_island_t island;

    // 640x480 — это VIC=1, всё остальное (например 240p) идёт как VIC=0.
    uint8_t vic = (height == 480) ? 1 : 0;
    hstx_packet_set_avi_infoframe(&packet, vic, 0);
    hstx_encode_data_island(&island, &packet, false, true);
    vblank_avi_infoframe_len = build_line_with_di(vblank_avi_infoframe, island.words, false, false);

    vblank_di_null_len = build_line_with_di(vblank_di_null, hstx_get_null_data_island(false, true), false, false);
    vactive_di_null_len = build_line_with_di(vactive_di_null, hstx_get_null_data_island(false, true), false, true);

    vblank_di_len = build_line_with_di(vblank_di_ping, hstx_get_null_data_island(false, true), false, false);
    memcpy(vblank_di_pong, vblank_di_ping, sizeof(vblank_di_ping));
}

void video_output_set_background_task(video_output_task_fn task)
{
    background_task = task;
}

bool video_output_get_dvi_mode(void)
{
    return dvi_mode;
}

void video_output_set_dvi_mode(bool enabled)
{
    dvi_mode = enabled;
}

void video_output_set_scanline_callback(video_output_scanline_cb_t cb)
{
    scanline_callback = cb;
}

void video_output_set_vsync_callback(video_output_vsync_cb_t cb)
{
    vsync_callback = cb;
}

void video_output_setup(void)
{
    // Настройка HSTX: TMDS-кодеры на каждой полосе и сдвиговый регистр.
    hstx_ctrl_hw->expand_tmds = 4 << HSTX_CTRL_EXPAND_TMDS_L2_NBITS_LSB | 8 << HSTX_CTRL_EXPAND_TMDS_L2_ROT_LSB |
                                5 << HSTX_CTRL_EXPAND_TMDS_L1_NBITS_LSB | 3 << HSTX_CTRL_EXPAND_TMDS_L1_ROT_LSB |
                                4 << HSTX_CTRL_EXPAND_TMDS_L0_NBITS_LSB | 13 << HSTX_CTRL_EXPAND_TMDS_L0_ROT_LSB;

    hstx_ctrl_hw->expand_shift =
        2 << HSTX_CTRL_EXPAND_SHIFT_ENC_N_SHIFTS_LSB | 16 << HSTX_CTRL_EXPAND_SHIFT_ENC_SHIFT_LSB |
        1 << HSTX_CTRL_EXPAND_SHIFT_RAW_N_SHIFTS_LSB | 0 << HSTX_CTRL_EXPAND_SHIFT_RAW_SHIFT_LSB;

    /* На время настройки bit[] HSTX обязательно гасим. Если включить
     * EN_BITS до того, как назначены lane->GPIO, на дефолтных пинах
     * успевает вылететь короткий шлейф мусорного TMDS — некоторые
     * приёмники по нему цепляются и больше не отпускают. Сначала
     * bit[], EN_BITS — последним. */
    hstx_ctrl_hw->csr = 0;

#ifdef HSTX_INVERT_DIFFPAIRS
    // Плата M2: дифпары разведены инверсно — P на чётном GPIO, N на нечётном.
    hstx_ctrl_hw->bit[0] = HSTX_CTRL_BIT0_CLK_BITS;
    hstx_ctrl_hw->bit[1] = HSTX_CTRL_BIT0_CLK_BITS | HSTX_CTRL_BIT0_INV_BITS;
    for (uint lane = 0; lane < 3; ++lane) {
        int bit = 2 + (lane * 2);
        uint32_t lane_data_sel_bits = (lane * 10) << HSTX_CTRL_BIT0_SEL_P_LSB | (lane * 10 + 1)
                                                                                    << HSTX_CTRL_BIT0_SEL_N_LSB;
        hstx_ctrl_hw->bit[bit] = lane_data_sel_bits;
        hstx_ctrl_hw->bit[bit + 1] = lane_data_sel_bits | HSTX_CTRL_BIT0_INV_BITS;
    }
#else
    hstx_ctrl_hw->bit[0] = HSTX_CTRL_BIT0_CLK_BITS | HSTX_CTRL_BIT0_INV_BITS;
    hstx_ctrl_hw->bit[1] = HSTX_CTRL_BIT0_CLK_BITS;
    for (uint lane = 0; lane < 3; ++lane) {
        int bit = 2 + (lane * 2);
        uint32_t lane_data_sel_bits = (lane * 10) << HSTX_CTRL_BIT0_SEL_P_LSB | (lane * 10 + 1)
                                                                                    << HSTX_CTRL_BIT0_SEL_N_LSB;
        hstx_ctrl_hw->bit[bit] = lane_data_sel_bits | HSTX_CTRL_BIT0_INV_BITS;
        hstx_ctrl_hw->bit[bit + 1] = lane_data_sel_bits;
    }
#endif

    // GPIO 12-19 переводим на функцию HSTX (на RP2350 это function 0).
    // По даташиту (2.19.6.1.8) подтяжки переживают смену функции, а
    // video_autodetect() как раз оставляет pull-up'ы на этих пинах.
    // 50 кОм на TMDS-дифпарах достаточно, чтобы сигнал поплыл и
    // приёмник не поймал синхру — поэтому подтяжки снимаем сразу
    // после смены функции.
    for (int i = PIN_HSTX_CLK; i <= PIN_HSTX_D2 + 1; ++i) {
        gpio_set_function(i, 0);
        gpio_disable_pulls(i);
    }

    /* bit[] разложен — теперь можно поднимать HSTX. На первых же
     * тактах TMDS уйдёт на правильные пины. */
    hstx_ctrl_hw->csr = HSTX_CTRL_CSR_EXPAND_EN_BITS | (uint32_t)MODE_HSTX_CSR_CLKDIV << HSTX_CTRL_CSR_CLKDIV_LSB |
                        5U << HSTX_CTRL_CSR_N_SHIFTS_LSB | 2U << HSTX_CTRL_CSR_SHIFT_LSB | HSTX_CTRL_CSR_EN_BITS;

    // Две DMA-цепочки ping/pong кормят HSTX FIFO командами.
    dma_channel_config c = dma_channel_get_default_config(DMACH_PING);
    channel_config_set_chain_to(&c, DMACH_PONG);
    channel_config_set_dreq(&c, DREQ_HSTX);
    dma_channel_configure(DMACH_PING, &c, &hstx_fifo_hw->fifo, vblank_line_vsync_off, count_of(vblank_line_vsync_off),
                          false);

    c = dma_channel_get_default_config(DMACH_PONG);
    channel_config_set_chain_to(&c, DMACH_PING);
    channel_config_set_dreq(&c, DREQ_HSTX);
    dma_channel_configure(DMACH_PONG, &c, &hstx_fifo_hw->fifo, vblank_line_vsync_off, count_of(vblank_line_vsync_off),
                          false);

    dma_hw->ints0 = (1U << DMACH_PING) | (1U << DMACH_PONG);
    dma_hw->inte0 = (1U << DMACH_PING) | (1U << DMACH_PONG);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_priority(DMA_IRQ_0, 0);
    irq_set_enabled(DMA_IRQ_0, true);

    /* В bus_ctrl приоритет DMA не задираем. Если DMA получает
     * приоритет над CPU, на RP2350 затыкается QMI/XIP — а главный
     * цикл живёт во flash и через него же подгружает свой код. В
     * итоге цикл просто перестаёт продвигаться после первой
     * итерации. SpeccyP при штатном round-robin-арбитраже спокойно
     * успевает подавать данные в HSTX FIFO. */
    dma_channel_start(DMACH_PING);
}

void video_output_core1_run(void)
{
    video_output_setup();

    while (1) {
        if (background_task) {
            background_task();
        }
        tight_loop_contents();
    }
}

void pico_hdmi_set_audio_sample_rate(uint32_t sample_rate)
{
    configure_audio_packets(sample_rate);
}

/*
 * SpeccyP — шим звукового вывода через HDMI Data Islands (HSTX).
 *
 * Снаружи это та же тройка функций, что у audio_i2s.h:
 *   i2s_init()
 *   i2s_out(uint16_t left, uint16_t right)   — посэмпловый вызов
 *   i2s_deinit()
 *
 * Внутри вместо PIO+DMA на I2S-пины сэмплы упаковываются в HDMI Data
 * Island (HDMI 1.3a §5.3) и складываются в общую очередь. Оттуда их
 * забирает scanout-ISR HSTX и подсовывает по одному пакету примерно
 * раз в 4 строки — для 60 Hz / 525 строк темп получается ровно тот
 * что нужен, никакой явной синхронизации со стороны производителя
 * не требуется.
 *
 * Эмулятор вызывает i2s_out() по одному стереосэмплу. В один Data
 * Island влезает ровно 4 стереокадра, поэтому копим 4 сэмпла и одним
 * пакетом отправляем в очередь.
 */

#include "audio_i2s.h"

#include "hstx_packet.h"
#include "hstx_data_island_queue.h"
#include "video_output.h"

#include "pico/stdlib.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifndef I2S_FREQ
#define I2S_FREQ 111111
#endif

static int g_frame_counter = 0;
static bool g_inited = false;

/* Накопитель на 4 сэмпла — ровно полезная нагрузка одного Data
 * Island. i2s_out() пишет сюда, как только набралось 4 — сбрасываем
 * в очередь.
 *
 * Пятый слот в массиве — guard на случай гонки. На реальном железе
 * ловили запись в g_pending[count] при count==4 (i2s_out и
 * flush_pending пересекались на границе кадра): соседний по BSS
 * graphics_buffer затирался значением вида 0xC000C000, после чего
 * scanline_cb прыгал по битому указателю и убивал видеовывод. Сам
 * outflow мы теперь зажимаем индекс через if (idx >= 4) idx = 3;
 * запасной слот стоит на всякий случай. */
static audio_sample_t g_pending[5];
static volatile uint8_t g_pending_count = 0;

/* HDMI понимает только фиксированный набор частот: 32 / 44.1 / 48 /
 * 88.2 / 96 / 176.4 / 192 кГц. Producer (AY-таймер на ~111 кГц) на
 * деле часто не дотягивает до своей номинальной частоты: callback
 * тяжёлый (вычисление AY-сэмпла + box-filter), а сам таймер с
 * "exact period" режимом не компенсирует пропущенные тики. Если
 * взять HDMI-частоту в 96 кГц, потребитель сожрёт всё, что producer
 * способен дать, и при просадках producer'а очередь идёт всухую —
 * слышны периодические пропуски.
 *
 * Берём 48 кГц: для AY/beeper/TS этого с головой, а producer на
 * любой просадке всё равно успевает кормить consumer'а с запасом,
 * очередь стабильно полная, тишинных пакетов нет. */
static uint32_t pick_hdmi_audio_rate(uint32_t producer_rate)
{
    (void)producer_rate;
    return 48000;
}

/* Producer (i2s_out) и HDMI sample rate почти никогда не совпадают
 * ровно: producer обычно 111 кГц, ближайший HDMI — 96 кГц. Если
 * принимать каждый сэмпл как есть, очередь Data Islands
 * переполняется, лишнее теряется кусками — отсюда щелчки и
 * искажения. Простой decimator с выкидыванием тоже плохо: при
 * прореживании сигнала с 111 кГц на 96 кГц без low-pass фильтра
 * получим aliasing и дребезг.
 *
 * Box-filter downsampler: накапливаем входные сэмплы (сумма + счётчик).
 * Bresenham-style accum решает, когда пора отдать один выходной — в
 * этот момент берём среднее (sum/count) и сбрасываем накопитель.
 * Среднее по нескольким соседним сэмплам — это простейший low-pass,
 * для отношения 1.16x его более чем достаточно. */
static uint32_t g_decim_accum = 0;
static uint32_t g_decim_in_rate = 0;   // = I2S_FREQ
static uint32_t g_decim_out_rate = 0;  // = выбранная HDMI-частота
static int32_t  g_avg_left  = 0;
static int32_t  g_avg_right = 0;
static uint32_t g_avg_count = 0;

void i2s_init(void)
{
    /* Если i2s_init() зовут повторно (например, после смены режима
     * звука), планировщик Data Islands должен начать с чистого
     * состояния — иначе в очереди останется мусор от прошлого режима. */
    hstx_di_queue_init();

    uint32_t rate = pick_hdmi_audio_rate(I2S_FREQ);
    pico_hdmi_set_audio_sample_rate(rate);

    g_decim_in_rate  = I2S_FREQ;
    g_decim_out_rate = rate;
    g_decim_accum = 0;
    g_avg_left = 0;
    g_avg_right = 0;
    g_avg_count = 0;
    g_frame_counter = 0;
    g_pending_count = 0;
    g_inited = true;
}

void i2s_deinit(void)
{
    g_inited = false;
    hstx_di_queue_update_silence(0);
}

static inline void __not_in_flash_func(flush_pending)(void)
{
    hstx_packet_t packet;
    int new_fc = hstx_packet_set_audio_samples(&packet, g_pending, 4, g_frame_counter);
    hstx_data_island_t island;
    hstx_encode_data_island(&island, &packet, false, true);
    if (hstx_di_queue_push(&island)) {
        g_frame_counter = new_fc;
    }
    /* В "тишинном" пакете тоже держим актуальный frame_counter.
     * Если очередь временно опустеет, ISR подсунет тишину; счётчик
     * там должен совпадать с потоком, иначе приёмник теряет синхру. */
    hstx_di_queue_update_silence(g_frame_counter);
    g_pending_count = 0;
}

void __not_in_flash_func(i2s_out)(uint16_t left, uint16_t right)
{
    if (!g_inited) return;

#if I2S_FREQ == 48000
    /* В legacy I2S-пути SpeccyP сэмпл уже знаковый (int16_t), просто
     * прототип uint16_t — делаем битовый reinterpret без расширения
     * знака. */
    int16_t sample_left = (int16_t)left;
    int16_t sample_right = (int16_t)right;
#else
    /* В legacy I2S-пути SpeccyP сэмпл уже знаковый (int16_t), просто
     * прототип uint16_t — делаем битовый reinterpret без расширения
     * знака. Накапливаем входной сэмпл в усреднитель. */
    g_avg_left  += (int32_t)(int16_t)left;
    g_avg_right += (int32_t)(int16_t)right;
    g_avg_count++;

    /* Bresenham решает, когда пора отдать один выходной сэмпл наружу.
     * accum растёт на out_rate каждый входной, при превышении in_rate
     * наступает "выходная точка": берём среднее накопленных входов и
     * вычитаем in_rate. Получается равномерный темп out_rate сэмплов
     * на каждые in_rate входных, и каждый выходной — это среднее
     * нескольких соседних входных (простейший low-pass). */
    g_decim_accum += g_decim_out_rate;
    if (g_decim_accum < g_decim_in_rate) return;
    g_decim_accum -= g_decim_in_rate;

    int32_t out_left  = g_avg_left  / (int32_t)g_avg_count;
    int32_t out_right = g_avg_right / (int32_t)g_avg_count;
    g_avg_left = 0;
    g_avg_right = 0;
    g_avg_count = 0;

    int16_t sample_left = (int16_t)out_left;
    int16_t sample_right = (int16_t)out_right;
#endif

    /* Зажимаем индекс в [0..3]. Раньше при гонке i2s_out и
     * flush_pending count успевал стать 4 до сброса, и запись в
     * g_pending[4] уходила в соседний BSS-символ — graphics_buffer
     * из HDMI_hstx.c — что моментально клало HSTX scanout. */
    uint8_t idx = g_pending_count;
    if (idx >= 4) idx = 3;

    g_pending[idx].left  = sample_left;
    g_pending[idx].right = sample_right;
    g_pending_count = idx + 1;

    if (g_pending_count >= 4) {
        flush_pending();
    }
}
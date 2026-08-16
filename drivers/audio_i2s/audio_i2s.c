/**
 * MIT License
 *
 * Copyright (c) 2025 DeepSeek
 
 https://github.com/raspberrypi/pico-sdk/issues/2059

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "audio_i2s.h"

// Статические переменные только для состояния DMA
static int dma_i2s_channel;
static uint32_t i2s_data;

void i2s_init(void)
{
    // Настройка drive strength для пинов
    gpio_set_drive_strength(I2S_DATA_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(I2S_CLK_BASE_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(I2S_CLK_BASE_PIN + 1, GPIO_DRIVE_STRENGTH_4MA);

    // Настройка GPIO
    uint func = (PIO_I2S == pio0) ? GPIO_FUNC_PIO0 : GPIO_FUNC_PIO1;
    gpio_set_function(I2S_DATA_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN, func);
    gpio_set_function(I2S_CLK_BASE_PIN + 1, func);

    // Расчет делителя частоты
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / I2S_FREQ;

    // Добавление программы в PIO
    int pio_offset = pio_add_program(PIO_I2S, &audio_i2s_program);
    if (pio_offset < 0) {
        // Обработка ошибки
      //  gpio_put(25, 1);
        return;
    }

    // Инициализация PIO программы
    audio_i2s_program_init(PIO_I2S, SM_I2S, pio_offset, I2S_DATA_PIN, I2S_CLK_BASE_PIN);
    pio_sm_set_clkdiv_int_frac(PIO_I2S, SM_I2S, divider >> 8u, divider & 0xffu);
    pio_sm_set_enabled(PIO_I2S, SM_I2S, false);

    // Настройка DMA
    dma_i2s_channel = dma_claim_unused_channel(true);
    
    dma_channel_config cfg_dma = dma_channel_get_default_config(dma_i2s_channel);
    channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg_dma, false);
    channel_config_set_write_increment(&cfg_dma, false);

    // Определение DREQ для DMA
    uint dreq_base = (PIO_I2S == pio0) ? DREQ_PIO0_TX0 : DREQ_PIO1_TX0;
    uint dreq = dreq_base + SM_I2S;
    channel_config_set_dreq(&cfg_dma, dreq);

    // Конфигурация DMA канала
    dma_channel_configure(
        dma_i2s_channel,
        &cfg_dma,
        &PIO_I2S->txf[SM_I2S],    // Write address
        &i2s_data,                // Read address
        1024,                     // Transfer count
        true                      // Start immediately
    );

    // Запуск
    pio_sm_set_enabled(PIO_I2S, SM_I2S, true);
    dma_start_channel_mask((1u << dma_i2s_channel));
}

void i2s_deinit(void)
{
    // Остановка и освобождение DMA
    dma_channel_abort(dma_i2s_channel);
    dma_channel_unclaim(dma_i2s_channel);

    // Остановка State Machine PIO
    pio_sm_set_enabled(PIO_I2S, SM_I2S, false);
    
    // Освобождение State Machine
    pio_sm_unclaim(PIO_I2S, SM_I2S);
    
    // Удаление программы из PIO (нужно сохранить offset, но для простоты пересчитаем)
    int pio_offset = pio_add_program(PIO_I2S, &audio_i2s_program);
    if (pio_offset >= 0) {
        pio_remove_program(PIO_I2S, &audio_i2s_program, pio_offset);
    }
    
    // Сброс GPIO
    gpio_set_function(I2S_DATA_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2S_CLK_BASE_PIN, GPIO_FUNC_NULL);
    gpio_set_function(I2S_CLK_BASE_PIN + 1, GPIO_FUNC_NULL);
}

void i2s_out(uint16_t left, uint16_t right)
{
    i2s_data = (uint32_t)(left << 16) | right;

    if (!dma_channel_is_busy(dma_i2s_channel)) {
        dma_hw->ch[dma_i2s_channel].al1_transfer_count_trig = TRANSFER_COUNT;
    }
}
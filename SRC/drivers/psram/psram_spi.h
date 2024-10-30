/******************************************************************************

rp2040-psram

Copyright © 2023 Ian Scott

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

******************************************************************************/

/**
 * @file psram_spi.h
 *
 * \mainpage
 *
 * The interface to this file is defined in psram_spi.h. Please see the
 * documentation for this file.
 *
 * The following defines _MUST_ be defined:
 *
 * - @c PSRAM_PIN_CS - GPIO number of the chip select pin
 * - @c PSRAM_PIN_SCK - GPIO number of the clock pin
 * - @c PSRAM_PIN_MOSI - GPIO number of the MOSI pin
 * - @c PSRAM_PIN_MISO - GPIO number of the MISO pin
 *
 * Optional define:
 * - @c PSRAM_MUTEX - Define this to put PSRAM access behind a mutex. This must
 * be used if the PSRAM is to be used by multiple cores.
 *
 * Project homepage: https://github.com/polpo/rp2040-psram
 */

    #define    PSRAM_PIN_CS 18
    #define    PSRAM_PIN_SCK 19
    #define    PSRAM_PIN_MOSI 20
    #define    PSRAM_PIN_MISO 21

//#define PSRAM_MUTEX 




#pragma once

#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#if defined(PSRAM_MUTEX)
#include "pico/mutex.h"
#elif defined(PSRAM_SPINLOCK)
#include "hardware/sync.h"
#endif
#include <string.h>

#include "psram_spi.pio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief A struct that holds the configuration for the PSRAM interface.
 *
 * This struct is generated by psram_spi_init() and must be passed to all calls to
 * the psram access functions.
 */
typedef struct psram_spi_inst {
    PIO pio;
    int sm;
    uint offset;
#if defined(PSRAM_MUTEX)
    mutex_t mtx;
#elif defined(PSRAM_SPINLOCK)
    spin_lock_t* spinlock;
    uint32_t spin_irq_state;
#endif
    int write_dma_chan;
    dma_channel_config write_dma_chan_config;
    int read_dma_chan;
    dma_channel_config read_dma_chan_config;
#if defined(PSRAM_ASYNC)
    int async_dma_chan;
    dma_channel_config async_dma_chan_config;
#endif
} psram_spi_inst_t;

#if defined(PSRAM_ASYNC)
extern psram_spi_inst_t* async_spi_inst;
#endif

/**
 * @brief Write and read raw data to the PSRAM SPI PIO, driven by the CPU
 * without DMA. This can be used if DMA has not yet been initialized.
 *
 * Used to send raw commands and receive data from the PSRAM. Usually the @c
 * psram_write*() and @c psram_read*() commands should be used instead.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param src Pointer to the source data to write.
 * @param src_len Length of the source data in bytes.
 * @param dst Pointer to the destination for read data, if any. Set to 0 or NULL
 * if no data is to be read.
 * @param dst_len Length of the destination data in bytes. Set to 0 if no data
 * is to be read.
 */
__force_inline static void __time_critical_func(pio_spi_write_read_blocking)(
        psram_spi_inst_t* spi,
        const uint8_t* src, const size_t src_len,
        uint8_t* dst, const size_t dst_len
) {
    size_t tx_remain = src_len, rx_remain = dst_len;

#if defined(PSRAM_MUTEX)
    mutex_enter_blocking(&spi->mtx); 
#elif defined(PSRAM_SPINLOCK)
    spi->spin_irq_state = spin_lock_blocking(spi->spinlock);
#endif
    io_rw_8 *txfifo = (io_rw_8 *) &spi->pio->txf[spi->sm];
    while (tx_remain) {
        if (!pio_sm_is_tx_fifo_full(spi->pio, spi->sm)) {
            *txfifo = *src++;
            --tx_remain;
        }
    }

    io_rw_8 *rxfifo = (io_rw_8 *) &spi->pio->rxf[spi->sm];
    while (rx_remain) {
        if (!pio_sm_is_rx_fifo_empty(spi->pio, spi->sm)) {
            *dst++ = *rxfifo;
            --rx_remain;
        }
    }

#if defined(PSRAM_MUTEX)
    mutex_exit(&spi->mtx);
#elif defined(PSRAM_SPINLOCK)
    spin_unlock(spi->spinlock, spi->spin_irq_state);
#endif
}

/**
 * @brief Write raw data to the PSRAM SPI PIO, driven by DMA without CPU
 * involvement. 
 *
 * It's recommended to use DMA when possible as it's higher speed. Used to send
 * raw commands to the PSRAM. This function is faster than
 * pio_spi_write_read_dma_blocking() if no data is to be read.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param src Pointer to the source data to write.
 * @param src_len Length of the source data in bytes.
 */
__force_inline static void __time_critical_func(pio_spi_write_dma_blocking)(
        psram_spi_inst_t* spi,
        const uint8_t* src, const size_t src_len
) {
#ifdef PSRAM_MUTEX
    mutex_enter_blocking(&spi->mtx); 
#elif defined(PSRAM_SPINLOCK)
    spi->spin_irq_state = spin_lock_blocking(spi->spinlock);
#endif // PSRAM_SPINLOCK
#if defined(PSRAM_WAITDMA)
#if defined(PSRAM_ASYNC)
    dma_channel_wait_for_finish_blocking(spi->async_dma_chan);
#endif // PSRAM_ASYNC
    dma_channel_wait_for_finish_blocking(spi->write_dma_chan);
    dma_channel_wait_for_finish_blocking(spi->read_dma_chan);
#endif // PSRAM_WAITDMA
    dma_channel_transfer_from_buffer_now(spi->write_dma_chan, src, src_len);
    dma_channel_wait_for_finish_blocking(spi->write_dma_chan);
#ifdef PSRAM_MUTEX
    mutex_exit(&spi->mtx);
#elif defined(PSRAM_SPINLOCK)
    spin_unlock(spi->spinlock, spi->spin_irq_state);
#endif // PSRAM_SPINLOCK
}

/**
 * @brief Write and read raw data to the PSRAM SPI PIO, driven by DMA without CPU
 * involvement. 
 *
 * It's recommended to use DMA when possible as it's higher speed. Used to send
 * raw commands and receive data from the PSRAM. Usually the @c psram_write* and
 * @c psram_read* commands should be used instead.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param src Pointer to the source data to write.
 * @param src_len Length of the source data in bytes.
 * @param dst Pointer to the destination for read data, if any. Set to 0 or NULL
 * if no data is to be read.
 * @param dst_len Length of the destination data in bytes. Set to 0 if no data
 * is to be read.
 */
__force_inline static void __time_critical_func(pio_spi_write_read_dma_blocking)(
        psram_spi_inst_t* spi,
        const uint8_t* src, const size_t src_len,
        uint8_t* dst, const size_t dst_len
) {
#ifdef PSRAM_MUTEX
    mutex_enter_blocking(&spi->mtx); 
#elif defined(PSRAM_SPINLOCK)
    spi->spin_irq_state = spin_lock_blocking(spi->spinlock);
#endif // PSRAM_SPINLOCK
#if defined(PSRAM_WAITDMA)
#if defined(PSRAM_ASYNC)
    dma_channel_wait_for_finish_blocking(spi->async_dma_chan);
#endif // PSRAM_ASYNC
    dma_channel_wait_for_finish_blocking(spi->write_dma_chan);
    dma_channel_wait_for_finish_blocking(spi->read_dma_chan);
#endif // PSRAM_WAITDMA
    dma_channel_transfer_from_buffer_now(spi->write_dma_chan, src, src_len);
    dma_channel_transfer_to_buffer_now(spi->read_dma_chan, dst, dst_len);
    dma_channel_wait_for_finish_blocking(spi->write_dma_chan);
    dma_channel_wait_for_finish_blocking(spi->read_dma_chan);
#ifdef PSRAM_MUTEX
    mutex_exit(&spi->mtx);
#elif defined(PSRAM_SPINLOCK)
    spin_unlock(spi->spinlock, spi->spin_irq_state);
#endif // PSRAM_SPINLOCK
}

/**
 * @brief Write raw data asynchronously to the PSRAM SPI PIO, driven by DMA without CPU
 * involvement. 
 *
 * Used to send raw commands to the PSRAM. Usually the @c psram_write*_async()
 * command should be used instead.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param src Pointer to the source data to write.
 * @param src_len Length of the source data in bytes.
 */
#if defined(PSRAM_ASYNC)
__force_inline static void __time_critical_func(pio_spi_write_async)(
        psram_spi_inst_t* spi,
        const uint8_t* src, const size_t src_len
) {
#if defined(PSRAM_ASYNC_SYNCHRONIZE)
#ifdef PSRAM_MUTEX
    mutex_enter_blocking(&spi->mtx); 
#elif defined(PSRAM_SPINLOCK)
    spi->spin_irq_state = spin_lock_blocking(spi->spinlock);
#endif // PSRAM_SPINLOCK
#endif // defined(PSRAM_ASYNC_SYNCHRONIZE)
    // Wait for all DMA to PSRAM to complete
    dma_channel_wait_for_finish_blocking(spi->write_dma_chan);
    dma_channel_wait_for_finish_blocking(spi->read_dma_chan);
    dma_channel_wait_for_finish_blocking(spi->async_dma_chan);
    async_spi_inst = spi;

    dma_channel_transfer_from_buffer_now(spi->async_dma_chan, src, src_len);
}
#endif


/**
 * @brief Initialize the PSRAM over SPI. This function must be called before
 * accessing PSRAM.
 *
 * @param pio The PIO instance to use (PIO0 or PIO1).
 * @param sm The state machine number in the PIO module to use. If -1 is given,
 * will use the first available state machine.
 * @param clkdiv Clock divisor for the state machine. At RP2040 speeds greater
 * than 280MHz, a clkdiv >1.0 is needed. For example, at 400MHz, a clkdiv of
 * 1.6 is recommended.
 * @param fudge Whether to insert an extra "fudge factor" of one clock cycle
 * before reading from the PSRAM. Depending on your PCB layout or PSRAM type,
 * you may need to do this.
 *
 * @return The PSRAM configuration instance. This instance should be passed to
 * all PSRAM access functions.
 */
psram_spi_inst_t psram_spi_init_clkdiv(PIO pio, int sm, float clkdiv, bool fudge);

/**
 * @brief Initialize the PSRAM over SPI. This function must be called before
 * accessing PSRAM.
 *
 * Defaults to a clkdiv of 1.0. This function is provided for backwards
 * compatibility. Use psram_spi_init_clkdiv instead if you want a clkdiv other
 * than 1.0.
 *
 * @param pio The PIO instance to use (PIO0 or PIO1).
 * @param sm The state machine number in the PIO module to use. If -1 is given,
 * will use the first available state machine.
 *
 * @return The PSRAM configuration instance. This instance should be passed to
 * all PSRAM access functions.
 */

psram_spi_inst_t psram_spi_init(PIO pio, int sm);
int test_psram(psram_spi_inst_t* psram_spi, int increment);

void psram_spi_uninit(psram_spi_inst_t spi, bool fudge);

static uint8_t write8_command[] = {
    40,         // 40 bits write
    0,          // 0 bits read
    0x02u,      // Write command
    0, 0, 0,    // Address
    0           // 8 bits data
};
/**
 * @brief Write 8 bits of data to a given address asynchronously to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement.
 *
 * This function is optimized to write 8 bits as quickly as possible to the
 * PSRAM as opposed to the more general-purpose psram_write() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param val Value to write.
 */
#if defined(PSRAM_ASYNC)
__force_inline static void psram_write8_async(psram_spi_inst_t* spi, uint32_t addr, uint8_t val) {
    write8_command[3] = addr >> 16;
    write8_command[4] = addr >> 8;
    write8_command[5] = addr;
    write8_command[6] = val;

    pio_spi_write_async(spi, write8_command, sizeof(write8_command));
};
#endif


/**
 * @brief Write 8 bits of data to a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the write is
 * complete.
 *
 * This function is optimized to write 8 bits as quickly as possible to the
 * PSRAM as opposed to the more general-purpose psram_write() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param val Value to write.
 */
__force_inline static void psram_write8(psram_spi_inst_t* spi, uint32_t addr, uint8_t val) {
    write8_command[3] = addr >> 16;
    write8_command[4] = addr >> 8;
    write8_command[5] = addr;
    write8_command[6] = val;

    pio_spi_write_dma_blocking(spi, write8_command, sizeof(write8_command));
};


static uint8_t read8_command[] = {
    40,         // 40 bits write
    8,          // 8 bits read
    0x0bu,      // Fast read command
    0, 0, 0,    // Address
    0           // 8 delay cycles
};
/**
 * @brief Read 8 bits of data from a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the read is
 * complete.
 *
 * This function is optimized to read 8 bits as quickly as possible from the
 * PSRAM as opposed to the more general-purpose psram_read() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to read from.
 * @return The data at the specified address.
 */
__force_inline static uint8_t psram_read8(psram_spi_inst_t* spi, uint32_t addr) {
    read8_command[3] = addr >> 16;
    read8_command[4] = addr >> 8;
    read8_command[5] = addr;

    uint8_t val; 
    pio_spi_write_read_dma_blocking(spi, read8_command, sizeof(read8_command), &val, 1);
    return val;
};


static uint8_t write16_command[] = {
    48,         // 48 bits write
    0,          // 0 bits read
    0x02u,      // Write command
    0, 0, 0,    // Address
    0, 0        // 16 bits data
};
/**
 * @brief Write 16 bits of data to a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the write is
 * complete.
 *
 * This function is optimized to write 16 bits as quickly as possible to the
 * PSRAM as opposed to the more general-purpose psram_write() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param val Value to write.
 */
__force_inline static void psram_write16(psram_spi_inst_t* spi, uint32_t addr, uint16_t val) {
    write16_command[3] = addr >> 16;
    write16_command[4] = addr >> 8;
    write16_command[5] = addr;
    write16_command[6] = val;
    write16_command[7] = val >> 8;

    pio_spi_write_dma_blocking(spi, write16_command, sizeof(write16_command));
};


static uint8_t read16_command[] = {
    40,         // 40 bits write
    16,         // 16 bits read
    0x0bu,      // Fast read command
    0, 0, 0,    // Address
    0           // 8 delay cycles
};
/**
 * @brief Read 16 bits of data from a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the read is
 * complete.
 *
 * This function is optimized to read 16 bits as quickly as possible from the
 * PSRAM as opposed to the more general-purpose psram_read() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to read from.
 * @return The data at the specified address.
 */
__force_inline static uint16_t psram_read16(psram_spi_inst_t* spi, uint32_t addr) {
    read16_command[3] = addr >> 16;
    read16_command[4] = addr >> 8;
    read16_command[5] = addr;

    uint16_t val; 
    pio_spi_write_read_dma_blocking(spi, read16_command, sizeof(read16_command), (unsigned char*)&val, 2);
    return val;
};


static uint8_t write32_command[] = {
    64,         // 64 bits write
    0,          // 0 bits read
    0x02u,      // Write command
    0, 0, 0,    // Address
    0, 0, 0, 0  // 32 bits data
};
/**
 * @brief Write 32 bits of data to a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the write is
 * complete.
 *
 * This function is optimized to write 32 bits as quickly as possible to the
 * PSRAM as opposed to the more general-purpose psram_write() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param val Value to write.
 */
__force_inline static void psram_write32(psram_spi_inst_t* spi, uint32_t addr, uint32_t val) {
    // Break the address into three bytes and send read command
    write32_command[3] = addr >> 16;
    write32_command[4] = addr >> 8;
    write32_command[5] = addr;
    write32_command[6] = val;
    write32_command[7] = val >> 8;
    write32_command[8] = val >> 16;
    write32_command[9] = val >> 24;

    pio_spi_write_dma_blocking(spi, write32_command, sizeof(write32_command));
};


/**
 * @brief Write 32 bits of data to a given address asynchronously to the PSRAM
 * SPI PIO, driven by DMA without CPU involvement.
 *
 * This function is optimized to write 32 bits as quickly as possible to the
 * PSRAM as opposed to the more general-purpose psram_write() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param val Value to write.
 */
#if defined(PSRAM_ASYNC)
__force_inline static void psram_write32_async(psram_spi_inst_t* spi, uint32_t addr, uint32_t val) {
    // Break the address into three bytes and send read command
    write32_command[3] = addr >> 16;
    write32_command[4] = addr >> 8;
    write32_command[5] = addr;
    write32_command[6] = val;
    write32_command[7] = val >> 8;
    write32_command[8] = val >> 16;
    write32_command[9] = val >> 24;

    pio_spi_write_async(spi, write32_command, sizeof(write32_command));
};
#endif

static uint8_t read32_command[] = {
    40,         // 40 bits write
    32,         // 32 bits read
    0x0bu,      // Fast read command
    0, 0, 0,    // Address
    0           // 8 delay cycles
};
/**
 * @brief Read 32 bits of data from a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the read is
 * complete.
 *
 * This function is optimized to read 32 bits as quickly as possible from the
 * PSRAM as opposed to the more general-purpose psram_read() function.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to read from.
 * @return The data at the specified address.
 */
__force_inline static uint32_t psram_read32(psram_spi_inst_t* spi, uint32_t addr) {
    read32_command[3] = addr >> 16;
    read32_command[4] = addr >> 8;
    read32_command[5] = addr;

    uint32_t val;
    pio_spi_write_read_dma_blocking(spi, read32_command, sizeof(read32_command), (unsigned char*)&val, 4);
    return val;
};


static uint8_t write_command[] = {
    0,          // n bits write
    0,          // 0 bits read
    0x02u,      // Fast write command
    0, 0, 0     // Address
};
/**
 * @brief Write @c count bytes of data to a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the write is
 * complete.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param src Pointer to the source data to write.
 * @param count Number of bytes to write.
 */
__force_inline static void psram_write(psram_spi_inst_t* spi, const uint32_t addr, const uint8_t* src, const size_t count) {
    // Break the address into three bytes and send read command
    write_command[0] = (4 + count) * 8;
    write_command[3] = addr >> 16;
    write_command[4] = addr >> 8;
    write_command[5] = addr;

    pio_spi_write_dma_blocking(spi, write_command, sizeof(write_command));
    pio_spi_write_dma_blocking(spi, src, count);
};


static uint8_t read_command[] = {
    40,         // 40 bits write
    0,          // n bits read
    0x0bu,      // Fast read command
    0, 0, 0,    // Address
    0           // 8 delay cycles
};
/**
 * @brief Read @c count bits of data from a given address to the PSRAM SPI PIO,
 * driven by DMA without CPU involvement, blocking until the read is
 * complete.
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to read from.
 * @param dst Pointer to the destination for the read data.
 * @param count Number of bytes to read.
 */
__force_inline static void psram_read(psram_spi_inst_t* spi, const uint32_t addr, uint8_t* dst, const size_t count) {
    read_command[1] = count * 8;
    read_command[3] = addr >> 16;
    read_command[4] = addr >> 8;
    read_command[5] = addr;

    pio_spi_write_read_dma_blocking(spi, read_command, sizeof(read_command), dst, count);
};


static uint8_t write_async_fast_command[134] = {
    0,          // n bits write
    0,          // 0 bits read
    0x02u      // Fast write command
};
/**
 * @brief Write @c count bytes of data to a given address asynchronously to the
 * PSRAM SPI PIO, driven by DMA without CPU involvement. 
 *
 * @param spi The PSRAM configuration instance returned from psram_spi_init().
 * @param addr Address to write to.
 * @param src Pointer to the source data to write.
 * @param count Number of bytes to write.
 */
#if defined(PSRAM_ASYNC)
__force_inline static void psram_write_async_fast(psram_spi_inst_t* spi, uint32_t addr, uint8_t* val, const size_t count) {
    write_async_fast_command[0] = (4 + count) * 8;
    write_async_fast_command[3] = addr >> 16;
    write_async_fast_command[4] = addr >> 8;
    write_async_fast_command[5] = addr;

    memcpy(write_async_fast_command + 6, val, count);

    pio_spi_write_async(spi, write_async_fast_command, 6 + count);
};
#endif
void uninit_psram();
uint8_t init_psram();
void psram_cleanup();
void write8psram(uint32_t addr32, uint8_t v);
void write16psram(uint32_t addr32, uint16_t v);
uint8_t read8psram(uint32_t addr32);
uint16_t read16psram(uint32_t addr32);

#ifdef __cplusplus
}
#endif
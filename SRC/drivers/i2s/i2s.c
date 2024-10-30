#include "i2s.h"

#include "hardware/pio.h"
#include <hardware/clocks.h>
#include "hardware/dma.h"
#include "config.h"

// --------- //
// audio_i2s //
// --------- //

#define audio_i2s_wrap_target 0
#define audio_i2s_wrap 7

#define audio_i2s_offset_entry_point 7u

static const uint16_t audio_i2s_program_instructions[] = {
            //     .wrap_target
    0x7001, //  0: out    pins, 1         side 2     
    0x1840, //  1: jmp    x--, 0          side 3     
    0x6001, //  2: out    pins, 1         side 0     
    0xe82e, //  3: set    x, 14           side 1     
    0x6001, //  4: out    pins, 1         side 0     
    0x0844, //  5: jmp    x--, 4          side 1     
    0x7001, //  6: out    pins, 1         side 2     
    0xf82e, //  7: set    x, 14           side 3     
            //     .wrap
};

static const struct pio_program audio_i2s_program = {
    .instructions = audio_i2s_program_instructions,
    .length = 8,
    .origin = -1,
};

static inline pio_sm_config audio_i2s_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + audio_i2s_wrap_target, offset + audio_i2s_wrap);
    sm_config_set_sideset(&c, 2, false, false);
    return c;
};

static uint sm_i2s=1;//-1
static inline void audio_i2s_program_init(PIO pio,  uint offset, uint data_pin, uint clock_pin_base) {
 //   sm_i2s  = pio_claim_unused_sm(pio, true);//
    uint sm = sm_i2s;

    uint8_t func=(pio==pio0)?GPIO_FUNC_PIO0:GPIO_FUNC_PIO1;    // TODO: GPIO_FUNC_PIO0 for pio0 or GPIO_FUNC_PIO1 for pio1
    gpio_set_function(data_pin, func);
    gpio_set_function(clock_pin_base, func);
    gpio_set_function(clock_pin_base+1, func);

    pio_sm_config sm_config = audio_i2s_program_get_default_config(offset);
    sm_config_set_out_pins(&sm_config, data_pin, 1);
    sm_config_set_sideset_pins(&sm_config, clock_pin_base);
    sm_config_set_out_shift(&sm_config, false, true, 32);
    pio_sm_init(pio, sm, offset, &sm_config);
    uint pin_mask = (1u << data_pin) | (3u << clock_pin_base);
    pio_sm_set_pindirs_with_mask(pio, sm, pin_mask, pin_mask);
    pio_sm_set_pins(pio, sm, 0); // clear pins
    pio_sm_exec(pio, sm, pio_encode_jmp(offset + audio_i2s_offset_entry_point));


    uint32_t sample_freq=111111;//44100;//111111;//88200;//44100; 
    sample_freq*=8;// 2 для max dac all 4
    uint32_t system_clock_frequency = clock_get_hz(clk_sys);
    uint32_t divider = system_clock_frequency * 4 / sample_freq; // avoid arithmetic overflow

    pio_sm_set_clkdiv_int_frac(pio, sm , divider >> 8u, divider & 0xffu);

    pio_sm_set_enabled(pio, sm, true);
}

static uint32_t i2s_data;
static uint32_t trans_count_DMA=1<<30;

void i2s_init(){
    uint offset = pio_add_program(PIO_I2S, &audio_i2s_program);
    audio_i2s_program_init(PIO_I2S, offset, I2S_DATA_PIN , I2S_CLK_BASE_PIN);

    int dma_i2s=dma_claim_unused_channel(true);
	int dma_i2s_ctrl=dma_claim_unused_channel(true);

    //основной рабочий канал
	dma_channel_config cfg_dma = dma_channel_get_default_config(dma_i2s);
	channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
	channel_config_set_chain_to(&cfg_dma, dma_i2s_ctrl);// chain to other channel

	channel_config_set_read_increment(&cfg_dma, false);
	channel_config_set_write_increment(&cfg_dma, false);



	uint dreq=DREQ_PIO1_TX0+sm_i2s;
	if (PIO_I2S==pio0) dreq=DREQ_PIO0_TX0+sm_i2s;
	channel_config_set_dreq(&cfg_dma, dreq);

	dma_channel_configure(
		dma_i2s,
		&cfg_dma,
		&PIO_I2S->txf[sm_i2s],		// Write address
		&i2s_data,					// read address
		1<<10,					//
		false			 				// Don't start yet
	);


    //контрольный канал для основного(перезапуск)
	cfg_dma = dma_channel_get_default_config(dma_i2s_ctrl);
	channel_config_set_transfer_data_size(&cfg_dma, DMA_SIZE_32);
	//channel_config_set_chain_to(&cfg_dma, dma_i2s);// chain to other channel
	
	channel_config_set_read_increment(&cfg_dma, false);
	channel_config_set_write_increment(&cfg_dma, false);



	dma_channel_configure(
		dma_i2s_ctrl,
		&cfg_dma,
		&dma_hw->ch[dma_i2s].al1_transfer_count_trig,	// Write address
		// &dma_hw->ch[dma_chan].al2_transfer_count,
		&trans_count_DMA,					// read address
		1,									//
		false								// Don't start yet
	);
    dma_start_channel_mask((1u << dma_i2s)) ;
};

void i2s_deinit(){
  //  pio_sm_unclaim(PIO_I2S,  sm_i2s);
 //   pio_remove_program(PIO_I2S, program595.length);
}

inline void i2s_out(int16_t l_out,int16_t r_out){i2s_data=(((uint16_t)l_out)<<16)|(((uint16_t)r_out));};
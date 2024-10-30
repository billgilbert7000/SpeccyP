#pragma once

#include "inttypes.h"
#include "stdbool.h"
//#include "config.h"
#include "g_config.h"





#define beginVideo_PIN (6)

#define HDMI_PIN_invert_diffpairs (1)
#define HDMI_PIN_RGB_notBGR (1)


#define beginHDMI_PIN_data (beginVideo_PIN+2)
#define beginHDMI_PIN_clk (beginVideo_PIN)


typedef enum g_mode{
    g_mode_320x240x8bpp,g_mode_320x240x4bpp
}g_mode;

typedef enum g_out{
    g_out_VGA,g_out_HDMI
}g_out;

void graphics_init(g_out g_out);
void graphics_set_buffer(uint8_t *buffer);
void graphics_set_mode(g_mode mode);
void graphics_set_palette(uint8_t i, uint32_t color888);
void startVIDEO(void);




#ifdef __cplusplus
extern "C" {
#endif
#include "vga.h"


#include "stdbool.h"
#include "stdio.h"
#include "stdint.h"

/*
 #define VGA
#ifdef TFT
#include "st7789.h"
#endif
#ifdef HDMI
#include "hdmi.h"
#endif
#ifdef VGA
#include "vga.h"
#endif */

enum graphics_mode_t {
    TEXTMODE_DEFAULT,
    GRAPHICSMODE_DEFAULT,

    TEXTMODE_53x30,

    TEXTMODE_160x100,

    CGA_160x200x16,
    CGA_320x200x4,
    CGA_640x200x2,

    TGA_320x200x16,
    EGA_320x200x16x4,
    VGA_320x240x256,
    VGA_320x200x256x4,
    // planar VGA
};



#ifdef __cplusplus
}
#endif

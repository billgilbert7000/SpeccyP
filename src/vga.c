#include "graphics.h"
#include "hardware/clocks.h"
#include "stdbool.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/systick.h"

#include "hardware/dma.h"
#include "hardware/irq.h"
#include <string.h>
#include <stdio.h>
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "stdlib.h"
#include "fnt8x16.h"

uint16_t pio_program_VGA_instructions[] = {
    //     .wrap_target
    0x6008, //  0: out    pins, 8
    //     .wrap
};

const struct pio_program pio_program_VGA = {
    .instructions = pio_program_VGA_instructions,
    .length = 1,
    .origin = -1,
};


static uint32_t* lines_pattern[4];
static uint32_t* lines_pattern_data = NULL;
static int _SM_VGA = -1;


static int N_lines_total = 525;
static int N_lines_visible = 480;
static int line_VS_begin = 490;
static int line_VS_end = 491;
static int shift_picture = 0;

static int begin_line_index = 0;
static int visible_line_size = 320;


static int dma_chan_ctrl;
static int dma_chan;

static uint8_t* graphics_buffer;
uint8_t* text_buffer = NULL;
static uint graphics_buffer_width = 0;
static uint graphics_buffer_height = 0;
static int graphics_buffer_shift_x = 0;
static int graphics_buffer_shift_y = 0;

static bool is_flash_line = false;
static bool is_flash_frame = false;

//буфер 1к графической палитры
static uint16_t palette[2][256];

static uint32_t bg_color[2];
static uint16_t palette16_mask = 0;

static uint text_buffer_width = 0;
static uint text_buffer_height = 0;

static uint16_t txt_palette[16];

//буфер 2К текстовой палитры для быстрой работы
//static uint16_t* txt_palette_fast = NULL;
static uint16_t txt_palette_fast[256*4];

enum graphics_mode_t graphics_mode;


void __time_critical_func() dma_handler_VGA() {
    dma_hw->ints0 = 1u << dma_chan_ctrl;
    static uint32_t frame_number = 0;
    static uint32_t screen_line = 0;
    static uint8_t* input_buffer = NULL;
    screen_line++;

    if (screen_line == N_lines_total) {
        screen_line = 0;
        frame_number++;
        input_buffer = graphics_buffer;
    }

    if (screen_line >= N_lines_visible) {
        //заполнение цветом фона
        if ((screen_line == N_lines_visible) | (screen_line == (N_lines_visible + 3))) {
            uint32_t* output_buffer_32bit = lines_pattern[2 + ((screen_line) & 1)];
            output_buffer_32bit += shift_picture / 4;
            uint32_t p_i = ((screen_line & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }

        //синхросигналы
        if ((screen_line >= line_VS_begin) && (screen_line <= line_VS_end))
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[1], false); //VS SYNC
        else
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    }

    if (!(input_buffer)) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false);
        return;
    } //если нет видеобуфера - рисуем пустую строку

    int y, line_number;

    uint32_t* * output_buffer = &lines_pattern[2 + (screen_line & 1)];
    uint div_factor = 2;
    switch (graphics_mode) {
        case CGA_160x200x16:
        case CGA_320x200x4:
        case CGA_640x200x2:
        case TGA_320x200x16:
        case EGA_320x200x16x4:
        case VGA_320x200x256x4:
        case GRAPHICSMODE_DEFAULT:
            line_number = screen_line / 2;
            if (screen_line % 2) return;
            y = screen_line / 2 - graphics_buffer_shift_y;
            break;

        case TEXTMODE_160x100:
        case TEXTMODE_53x30:
        case TEXTMODE_DEFAULT: {
            uint16_t* output_buffer_16bit = (uint16_t *)*output_buffer;
            output_buffer_16bit += shift_picture / 2;
            const uint font_weight = 8;
            const uint font_height = 16;

            // "слой" символа
            uint32_t glyph_line = screen_line % font_height;

            //указатель откуда начать считывать символы
            uint8_t* text_buffer_line = &text_buffer[screen_line / font_height * text_buffer_width * 2];

            for (int x = 0; x < text_buffer_width; x++) {
                //из таблицы символов получаем "срез" текущего символа
                uint8_t glyph_pixels = font_8x16[(*text_buffer_line++) * font_height + glyph_line];
                //считываем из быстрой палитры начало таблицы быстрого преобразования 2-битных комбинаций цветов пикселей
                uint16_t* color = &txt_palette_fast[4 * (*text_buffer_line++)];
#if 0
                if (cursor_blink_state && !manager_started &&
                    (screen_line / 16 == CURSOR_Y && x == CURSOR_X && glyph_line >= 11 && glyph_line <= 13)) {
                    *output_buffer_16bit++ = color[3];
                    *output_buffer_16bit++ = color[3];
                    *output_buffer_16bit++ = color[3];
                    *output_buffer_16bit++ = color[3];
                    if (text_buffer_width == 40) {
                        *output_buffer_16bit++ = color[3];
                        *output_buffer_16bit++ = color[3];
                        *output_buffer_16bit++ = color[3];
                        *output_buffer_16bit++ = color[3];
                    }
                }
                else
#endif
                    {
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
                    glyph_pixels >>= 2;
                    *output_buffer_16bit++ = color[glyph_pixels & 3];
                    if (text_buffer_width == 40) *output_buffer_16bit++ = color[glyph_pixels & 3];
                }
            }
            dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
            return;
        }
        default: {
            dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
            return;
        }
    }

    if (y < 0) {
        dma_channel_set_read_addr(dma_chan_ctrl, &lines_pattern[0], false); // TODO: ensue it is required
        return;
    }
    if (y >= graphics_buffer_height) {
        // заполнение линии цветом фона
        if ((y == graphics_buffer_height) | (y == (graphics_buffer_height + 1)) |
            (y == (graphics_buffer_height + 2))) {
            uint32_t* output_buffer_32bit = (uint32_t *)*output_buffer;
            uint32_t p_i = ((line_number & is_flash_line) + (frame_number & is_flash_frame)) & 1;
            uint32_t color32 = bg_color[p_i];

            output_buffer_32bit += shift_picture / 4;
            for (int i = visible_line_size / 2; i--;) {
                *output_buffer_32bit++ = color32;
            }
        }
        dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
        return;
    };

    //зона прорисовки изображения
    //начальные точки буферов
    // uint8_t* vbuf8=vbuf+line*g_buf_width; //8bit buf
    // uint8_t* vbuf8=vbuf+(line*g_buf_width/2); //4bit buf
    //uint8_t* vbuf8=vbuf+(line*g_buf_width/4); //2bit buf
    //uint8_t* vbuf8=vbuf+((line&1)*8192+(line>>1)*g_buf_width/4);
    uint8_t* input_buffer_8bit = input_buffer + ((y / 2) * 80) + ((y & 1) * 8192);


    //output_buffer = &lines_pattern[2 + ((line_number) & 1)];

    uint16_t* output_buffer_16bit = (uint16_t *)(*output_buffer);
    output_buffer_16bit += shift_picture / 2; //смещение началы вывода на размер синхросигнала

    //    g_buf_shx&=0xfffffffe;//4bit buf
    if (graphics_mode == CGA_640x200x2) {
        graphics_buffer_shift_x &= 0xfffffff1; //1bit buf
    }
    else {
        graphics_buffer_shift_x &= 0xfffffff2; //2bit buf
    }

    //для div_factor 2
    uint max_width = graphics_buffer_width;
    if (graphics_buffer_shift_x < 0) {
        //vbuf8-=g_buf_shx; //8bit buf
        if (CGA_640x200x2 == graphics_mode) {
            input_buffer_8bit -= graphics_buffer_shift_x / 8; //1bit buf
        }
        else {
            input_buffer_8bit -= graphics_buffer_shift_x / 4; //2bit buf
        }
        max_width += graphics_buffer_shift_x;
    }
    else {
        output_buffer_16bit += graphics_buffer_shift_x * 2 / div_factor;
    }


    int width = MIN((visible_line_size - ((graphics_buffer_shift_x > 0) ? (graphics_buffer_shift_x) : 0)), max_width);
    if (width < 0) return; // TODO: detect a case

    // Индекс палитры в зависимости от настроек чередования строк и кадров
    uint16_t* current_palette = palette[((y & is_flash_line) + (frame_number & is_flash_frame)) & 1];

    uint8_t* output_buffer_8bit;
    switch (graphics_mode) {
        case CGA_640x200x2:
            output_buffer_8bit = (uint8_t *)output_buffer_16bit;
        //1bit buf
            for (int x = width / 4; x--;) {
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 7) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 6) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 5) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 4) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 3) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 2) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 1) & 1];
                *output_buffer_8bit++ = current_palette[(*input_buffer_8bit >> 0) & 1];
                input_buffer_8bit++;
            }
            break;
        case CGA_320x200x4:
            //2bit buf
            for (int x = width / 4; x--;) {
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 6) & 3];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 4) & 3];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 2) & 3];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 0) & 3];
                input_buffer_8bit++;
            }
            break;
        case CGA_160x200x16:
            //4bit buf
            for (int x = width / 4; x--;) {
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 4) & 15];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 4) & 15];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit) & 15];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit) & 15];
                input_buffer_8bit++;
            }
            break;
        case TGA_320x200x16:
            //4bit buf
            input_buffer_8bit = input_buffer + ((y & 3) * 8192) + ((y / 4) * 160);
            for (int x = width / 2; x--;) {
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit >> 4) & 15];
                *output_buffer_16bit++ = current_palette[(*input_buffer_8bit) & 15];
                input_buffer_8bit++;
            }
            break;
        case EGA_320x200x16x4: {
            input_buffer_8bit = input_buffer + y * 40;
            for (int x = 0; x < 40; x++) {
                for (int bit = 7; bit--;) {
                    uint8_t color = (*(input_buffer_8bit) >> bit) & 1;
                    color |= ((*(input_buffer_8bit + 16000) >> bit) & 1) << 1;
                    color |= ((*(input_buffer_8bit + 32000) >> bit) & 1) << 2;
                    color |= ((*(input_buffer_8bit + 48000) >> bit) & 1) << 3;
                    *output_buffer_16bit++ = current_palette[color];
                }
                input_buffer_8bit++;
            }
            break;
        }
        case GRAPHICSMODE_DEFAULT:
            input_buffer_8bit = input_buffer + y * width;
            for (int i = width; i--;) {
                //*output_buffer_16bit++=current_palette[*input_buffer_8bit++];
                *output_buffer_16bit++ = current_palette[*input_buffer_8bit++];
            }
            break;
        case VGA_320x200x256x4:
            input_buffer_8bit = input_buffer + y * (width / 4);
            for (int x = width / 2; x--;) {
                //*output_buffer_16bit++=current_palette[*input_buffer_8bit++];
                *output_buffer_16bit++ = current_palette[*input_buffer_8bit];
                *output_buffer_16bit++ = current_palette[*(input_buffer_8bit + 16000)];
                *output_buffer_16bit++ = current_palette[*(input_buffer_8bit + 32000)];
                *output_buffer_16bit++ = current_palette[*(input_buffer_8bit + 48000)];
                *input_buffer_8bit++;
            }
            break;
        default:
            break;
    }
    dma_channel_set_read_addr(dma_chan_ctrl, output_buffer, false);
}



void graphics_set_offset(int x, int y) {
    graphics_buffer_shift_x = x;
    graphics_buffer_shift_y = y;
};

void graphics_set_flashmode(bool flash_line, bool flash_frame) {
    is_flash_frame = flash_frame;
    is_flash_line = flash_line;
};

void graphics_set_textbuffer(uint8_t* buffer) {
    text_buffer = buffer;
};

void graphics_set_bgcolor(uint32_t color888) {
    uint8_t conv0[] = { 0b00, 0b00, 0b01, 0b10, 0b10, 0b10, 0b11, 0b11 };
    uint8_t conv1[] = { 0b00, 0b01, 0b01, 0b01, 0b10, 0b11, 0b11, 0b11 };

    uint8_t b = ((color888 & 0xff) / 42);

    uint8_t r = (((color888 >> 16) & 0xff) / 42);
    uint8_t g = (((color888 >> 8) & 0xff) / 42);

    uint8_t c_hi = (conv0[r] << 4) | (conv0[g] << 2) | conv0[b];
    uint8_t c_lo = (conv1[r] << 4) | (conv1[g] << 2) | conv1[b];
    bg_color[0] = (((((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask) << 16) |
                  ((((c_hi << 8) | c_lo) & 0x3f3f) | palette16_mask);
    bg_color[1] = (((((c_lo << 8) | c_hi) & 0x3f3f) | palette16_mask) << 16) |
                  ((((c_lo << 8) | c_hi) & 0x3f3f) | palette16_mask);
};






void clrScr(const uint8_t color) {
    uint16_t* t_buf = (uint16_t *)text_buffer;
    int size = TEXTMODE_COLS * TEXTMODE_ROWS;

    while (size--) *t_buf++ = color << 4 | ' ';
}

#pragma once
#include "inttypes.h"
#include "stdbool.h"

#include "g_config.h"
#include "config.h"  
#include "zx_emu/zx_machine.h"


//#include "font6x8revers_new.h"
#include "font6x8.h"

#define FONT   FONT_6x8_DATA
#define FONT_W (6)
#define FONT_H (9)


typedef uint8_t color_t;

bool draw_pixel(int x,int y,color_t color);
void draw_text(int x,int y,char* text,color_t colorText,color_t colorBg);
void draw_text_len(int x,int y,char* text,color_t colorText,color_t colorBg,int len);
void draw_text_file(int x,int y,char* text,color_t colorText,color_t colorBg,int len);
void draw_line(int x0,int y0, int x1, int y1,color_t color);
//void draw_circle(int x0,int y0,  int r,color_t color);
void init_screen(uint8_t* scr_buf,int scr_width,int scr_height);
void draw_rect(int x,int y,int w,int h,color_t color,bool filled);
void ShowScreenshot(uint8_t* buffer);
void draw_img(uint8_t xPos,uint8_t yPos);


//void MenuBox(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  char *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox_trd(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox_bw(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos,  char* m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul);
void MenuBox_lite(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text, uint8_t over_emul);
uint8_t MenuBox_help(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char* m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox_tft(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text, uint8_t over_emul);
uint8_t MenuBox_tft_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  char *m_text[], uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox_advanced_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  uint8_t Pos,uint8_t cPos,uint8_t over_emul);
uint8_t MenuBox_sound_setup(uint8_t xPos, uint8_t yPos,uint8_t lPos ,uint8_t hPos, char *text,  uint8_t Pos,uint8_t cPos,uint8_t over_emul);
//
//-------------------------------------------------------
void MessageBox(char *text,char *text1,uint8_t colorFG,uint8_t colorBG,uint8_t over_emul);
void MessageBox_off(char *text,char *text1,uint8_t colorFG,uint8_t colorBG,uint8_t over_emul);
void draw_file_window();
void draw_main_window();
//void draw_keyboard(uint8_t xPos,uint8_t yPos);
void wait_enter(void);
void wait_esc(void);
bool decode_key_joy();

//--------------------------------------------------------
//##############
void draw_symbol(int x,int y,int symb,color_t colorText,color_t colorBg);
void draw_logo (int x,int y,color_t colorInc,color_t colorPaper);
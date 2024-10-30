
#pragma once
#include "stdio.h"

//---------------------------------------------
int	null_printf(const char *str, ...);
void g_delay_ms(uint delay);
#define G_PRINTF  printf
#define G_PRINTF_INFO  printf
#define G_PRINTF_DEBUG  printf
#define G_PRINTF_ERROR  printf
//------------------------------------------------------------
#define KEY_SPACE kb_st_ps2.u[1]&KB_U1_SPACE
#define KEY_ENTER kb_st_ps2.u[1]&KB_U1_ENTER
#define HOME kb_st_ps2.u[2]&KB_U2_HOME 
#define PAUSE kb_st_ps2.u[2]&KB_U2_PAUSE_BREAK
#define LCTRL kb_st_ps2.u[1]&KB_U1_L_CTRL
#define RCTRL kb_st_ps2.u[1]&KB_U1_R_CTRL
//#define F11 kb_st_ps2.u[3]&KB_U3_F11
//------------------------------------------------------------------------------------------
#define F1  kb_st_ps2.u[3]&KB_U3_F1
#define F9  kb_st_ps2.u[3]&KB_U3_F9
#define F8  kb_st_ps2.u[3]&KB_U3_F8
#define F7  kb_st_ps2.u[3]&KB_U3_F7
#define F10  kb_st_ps2.u[3]&KB_U3_F10
//#define MENU (kb_st_ps2.u[2]&KB_U2_HOME)|(kb_st_ps2.u[1]&KB_U1_L_WIN)|(kb_st_ps2.u[1]&KB_U1_R_WIN) //кнопка перехода в меню по HOME или WIN
//кнопка перехода в меню по HOME или WIN или F11
#define MENU (kb_st_ps2.u[2]&KB_U2_INSERT)|(kb_st_ps2.u[1]&KB_U1_L_WIN)|(kb_st_ps2.u[1]&KB_U1_R_WIN)|(kb_st_ps2.u[3]&KB_U3_F11) 
#define MENU_D (kb_st_ps2.u[2]&KB_U2_PRT_SCR) //кнопка перехода в меню nmi по PrintScreen
#define ESC_EXIT (kb_st_ps2.u[1]&KB_U1_ESC) // 
//кнопка перехода в меню SetUp
#define MENU_SETUP (kb_st_ps2.u[2] & KB_U2_HOME)|(kb_st_ps2.u[3]&KB_U3_F12)
// CTRL+ALT+DEL reset zx spectrum
#define KEY_RESET_ZX ((kb_st_ps2.u[1] & KB_U1_L_CTRL) || (kb_st_ps2.u[1] & KB_U1_R_CTRL)) && ((kb_st_ps2.u[1] & KB_U1_L_ALT) || (kb_st_ps2.u[1] & KB_U1_R_ALT)) && (kb_st_ps2.u[2] & KB_U2_DELETE)
// SHIFT+ALT+DEL reset pico
#define KEY_RESET_PICO ((kb_st_ps2.u[1] & KB_U1_L_SHIFT) || (kb_st_ps2.u[1] & KB_U1_R_SHIFT)) && ((kb_st_ps2.u[1] & KB_U1_L_ALT) || (kb_st_ps2.u[1] & KB_U1_R_ALT)) && (kb_st_ps2.u[2] & KB_U2_DELETE)
//----------------------------------------------------------------------------
// имитация джоя
//#define JOY_FIRE1 kb_st_ps2.u[1] & KB_U1_R_ALT // правый альт
#define JOY_FIRE1 kb_st_ps2.u[1] & KB_U1_SPACE // SPACE
//#define JOY_FIRE2 kb_st_ps2.u[2]&KB_U2_DELETE // DELETE NUMpad
//#define JOY_FIRE2 kb_st_ps2.u[2]&KB_U2_NUM_ENTER//
//#define JOY_FIRE2 kb_st_ps2.u[2]&KB_U2_NUM_0 
#define JOY_FIRE2 kb_st_ps2.u[2]&KB_U2_NUM_PERIOD // .точка он же del на доп клавиатуре  DELETE NUMpad
#define JOY_FIRE  (JOY_FIRE1) || (JOY_FIRE2)

//----------------------------------------------------------------------------
uint8_t mode_kbms ; // режим работы клавиатуры 0  пс пополам и так далее
uint16_t cpu_pc;
//----------------------------------------------//
//#define FONT6X8   
#define FONT8X8  
///////////////// nmi
 uint8_t z80_pc;

//---------- PSRAM -----------------  
bool PSRAM_AVAILABLE;
//----------------------------------------------------

//#define CLEAR_INFO	 draw_rect(17+FONT_W*14,16,182,214,COLOR_PIC_BG,true) 
#define CLEAR_INFO	 draw_rect(17+FONT_W*14,16+20,182,214-20,COLOR_PIC_BG,true) 
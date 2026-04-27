#pragma once

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>



// ======= Дефайны ===========
#if defined  GENERAL_SOUND
#include "gs_picobus.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <reent.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "pico/mutex.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"  

#include "pico/bootrom.h"

#include "hardware/vreg.h"
/********************************* */
#include "tf_card.h"
#include "ff.h"
//******************************* */



#define  TEST // 
#define  TRDOS_1// версия TR-DOS

#include "wd1793.h"

extern uint8_t table_voltage[];
extern uint32_t dtcpu;

//////////////////////////////////////////
//#include "I2C_rp2040.h"// это добавить
#include "usb_key.h"// это добавить

#define OUT_SND_MASK (0b00011000)
#define SHOW_SCREEN_DELAY 50  //500  в милисекундах

//-----------------------------------------------------------------------------------------
// частота RP2040
//#define CPU_KHZ 378000//378000//315000//315000//378000//276000 //252000 264000 ////set_sys_clock_khz(300000, true); // main.h252000//
//#define DEL_Z80 CPU_KHZ/14000 //80 // main.h 3500 turbo 7000
#define Z80_3500 CPU_KHZ/3500 //   CPU_KHZ/3500
#define Z80_7000 CPU_KHZ/7000
#define Z80_14000 CPU_KHZ/14000

//#define VOLTAGE VREG_VOLTAGE_1_30 //VREG_VOLTAGE_1_20 //	vreg_set_voltage(VOLTAGE); // main.h
#ifdef PICO_RP2350 
#define VOLTAGE_MIN VREG_VOLTAGE_1_30
#define VOLTAGE VREG_VOLTAGE_1_40
#else
#define VOLTAGE VREG_VOLTAGE_1_30
#endif
//=======================================================================================================

// 126MHz SPI
//#define SERIAL_CLK_DIV 3.0f
#define MADCTL_BGR_PIXEL_ORDER (1<<3)
#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)// 0x20
#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
#ifdef ILI9341
#define   MADCTL (MADCTL_ROW_COLUMN_EXCHANGE | MADCTL_BGR_PIXEL_ORDER) // 0x28
#else
    // ST7789
#define    MADCTL (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_COLUMN_EXCHANGE) // 0x60  Set MADCTL
#endif
//-------------------------------------------------------------------------------------------------
#define SOFT_AY 0
#define SOFT_TS 1
#define I2S_AY  2
#define I2S_TS  3
#define HARD_AY 4
#define HARD_TS 5
#define TSFM    6
#define TS_SAA  7
#define AY_END  8
//-----------------------------------------------------------------------------------------------
// настройки Hard TurboSound

#define HTS_DEFAULT  // программа  стейт машины для работы с регистром сдвига 595 без задержки 
//#define HTS_SAGIN    // программа  стейт машины для работы с регистром сдвига 595 c задержкой

//-----------------------------------------------------------------------------------------------

#define FW_AUTHOR "Speccy_P"
 
//-----------------------------------------------------------------------------------------------
#define PENT128  0
#define SPEC48 1
#define PENT512  2
#define PENT1024 3
#define SCORP256 4
//#define PROFI1024 5// отключено
#define GMX2048  5
//#define ZX4096 7// отключенно
#define NOVA256 6
#define PENT8M 7
#define PENT_512CASH 8

//
#define CONF_MASHINE PENT128 
 			// 0 soft AY
			// 1 soft TS
			// 2 hard AY
			// 3 hard TS
//#define conf.type_sound 1 // hard ts


#define S_COMAND_PIN 22
#define S_DATA_PIN 18



//**********************************************
// TFT
/* #define TFT_CLK_PIN (13)
#define TFT_DATA_PIN (12)
#define TFT_RST_PIN (8)
#define TFT_DC_PIN (10)
#define TFT_CS_PIN (6) 
#define TFT_LED_PIN  (9) */
//#define SERIAL_CLK_TFT 0x28000 //fdiv32:163840 #28000  126 MHz
#define SERIAL_CLK_TFT 0x40000

//**********************************************


#define SCREEN_H (240)
#define SCREEN_W (320)
#define V_BUF_SZ ((SCREEN_H+1)*SCREEN_W/2)


#define VGA_DMA_IRQ (DMA_IRQ_0)

#define TEXTMODE_COLS 80
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

#ifdef NUM_V_BUF
    extern  bool is_show_frame[NUM_V_BUF];
    extern int draw_vbuf_inx;
    extern int show_vbuf_inx;
#endif

// ---------------- цвета интерфейса -----------------------------

/*
color:
    0x00 - black
    0x01 - blue
    0x02 - red
    0x03 - pink
    0x04 - green
    0x05 - cyan
    0x06 - yellow
    0x07 - gray
    0x08 - black
    0x09 - blue+
    0x0a - red+
    0x0b - pink+
    0x0c - green+
    0x0d - cyan+
    0x0e - yellow+
    0x0f - white
*/


#define CL_BLACK     0x00
#define CL_BLUE      0x01
#define CL_RED       0x02
#define CL_PINK      0x03
#define CL_GREEN     0x04
#define CL_CYAN      0x05
#define CL_YELLOW    0x06
#define CL_GRAY      0x07
#define CL_LT_BLACK  0x08
#define CL_LT_BLUE   0x09
#define CL_LT_RED    0x0a
#define CL_LT_PINK   0x0b
#define CL_LT_GREEN  0x0c
#define CL_LT_CYAN   0x0d
#define CL_LT_YELLOW 0x0e
#define CL_WHITE     0x0f
/*
#define COLOR_TEXT          0x00
#define COLOR_SELECT        0x0D
#define COLOR_SELECT_TEXT   0x00
#define COLOR_BACKGOUND     0x0F
#define COLOR_BORDER        0x00
#define COLOR_PIC_BG        0x07
#define COLOR_FULLSCREEN    0x07
*/
#define CL_TEST        CL_PINK
#define COLOR_TEXT         CL_GREEN
#define COLOR_SELECT        0x0D
#define COLOR_SELECT_TEXT   0x00
#define COLOR_BACKGOUND     CL_BLACK
#define COLOR_BORDER       CL_GRAY
#define COLOR_PIC_BG        CL_BLACK
#define COLOR_FULLSCREEN    CL_BLACK
#define COLOR_UP            CL_LT_BLACK
#define COLOR_SCROLL        CL_GRAY

#define COLOR_TR0           CL_GRAY // цвет меню выбора диска
#define COLOR_TR1           CL_BLACK


#define NUMBER_CHAR 14 // длина имени файла вместе с разрешением
// цвета меню
#define CL0

#if defined(CL0)
#define CL_INK CL_GRAY
#define CL_PAPER CL_BLACK
#else
#define CL_INK CL_BLACK
#define CL_PAPER CL_GRAY
#endif

// дефайны файлов
#define ZX_RAM_PAGE_SIZE 0x4000
#define DIRS_DEPTH (8)// глубина директорий 8
#define MAX_FILES  (500)// максимум 500 файлов в каталоге
#define SD_BUFFER_SIZE  0x4000  //Размер буфера для работы с файлами
#define LENF 22  // 22
#define LENF1 LENF-1

//if (file_type[DRV]==TRD)
#define TRD 0
#define SCL 1
#define TRDS 3 // укороченый TRD
#define FDI 4

extern uint8_t sectors_per_track;

// дефайны меню настроек
#define M_RAM 0
#define M_SOUND 1
#define M_TURBO  2
#define M_JOY 3
#define M_AUTORUN 4
#define M_PALLETE 5
#define M_TFT_BRIGHT 5
#define M_SOUND_SETUP 6
#define M_ADVANCED 7
#define M_EMPTY_2  8

#define M_SAVE_CONFIG 9
#define M_SOFT_RESET 10
#define M_HARD_RESET 11
#define M_POWER_OFF 12
#define M_UPDATE 13
#define M_EXIT 14

//======================================================
//========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====================
//======================================================
extern uint8_t g_gbuf[V_BUF_SZ];
extern uint8_t g_gbuf[];
extern uint8_t color_zx[16];
extern bool vbuf_en;// экран эмуляции
extern bool is_menu_mode;
//**********************************************
extern uint8_t RAM[]; //Реальная память RAM
//------------------------------------------------
// Joystick
extern uint8_t joy_key_ext;
extern bool joy_connected;
extern uint8_t data_joy;
extern uint8_t old_data_joy;
//--------------------------------------------------
// fdd & trdos
extern bool trdos;
extern uint32_t sclDataOffset;
//unsigned char* track0;
extern uint8_t track0 [0x0900];
//#define track0 sd_buffer
extern bool write_protected;
extern uint8_t DriveNum;
extern uint8_t sound_track;

extern uint8_t dirs[DIRS_DEPTH][LENF];
extern uint8_t sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
extern char files[MAX_FILES][LENF];
extern char dir_patch[DIRS_DEPTH*(LENF+16)];
extern int init_fs;

extern uint8_t NoDisk;
extern uint8_t file_type[5];
extern uint8_t file_attr[5];
//---------------------------------
// tape loader
extern uint32_t seekbuf;
extern char TapeName[17];
extern bool enable_tape;
 //-------------------------------
//переменные состояния спектрума
extern uint8_t zx_Border_color;
//uint8_t mybuf[256];
//---------------------------------
//---------------------------------
extern uint32_t size_psram;
extern bool psram_type;
extern uint8_t type_psram;
#define NOT_PSRAM 0x00
#define BOARD_PSRAM 1
#define BUTTER_PSRAM 2
#define NOT_PSRAM_PIN21 3
#define BOARD_PSRAM_NOSUPORT 4


//---------------------------------
// other
extern char temp_msg[80]; // Буфер для вывода строк
//Quorum
extern bool main_nmi_key ;
//
extern bool im_z80_stop;
//
#ifdef GENERAL_SOUND
extern bool picobus_busy;
#endif
//=====================================================
#define TFT_9345   0
#define TFT_9345I  2
#define TFT_7789   1

//char Disks[4][160] = {"", "", "", ""};
//char Disks[4][DIRS_DEPTH*(LENF+16)];//160 // 5*16
//char DiskName[4][LENF+1];

/* как на основе этого создать текстовый конфигурационный файл , 
с понятными именами каждой переменной. для записи его на microSD и восстановления этих переменных
 из этого файла. используется rp2040 sdk raspberry pico vscode windows 
 Использовать  INI-подобный формат
 Для работы с файлами используется FatFS

strncpy для безопасного копирования строк

Обработка числовых и строковых параметров

Автоматическое создание конфига по умолчанию при отсутствии файла
реализация записи конфигурационного файла с подробными комментариями */

// данные конфигурационного файла
extern struct data_config
{
   uint64_t version;// версия конф. файла  
   uint8_t voltage;// Possible voltage values that can be applied to the regulator

   uint8_t vol_ay; //громкость soft AY
   uint8_t vol_i2s; //громкость i2s AY
   uint8_t vol_load;// громкость аудио загрузки
   uint8_t type_sound;// конфигурация вывода звука

   uint8_t pullup_pin_video;// определение видеовыхода для tft

   
   uint8_t vout;// Видевыход 0-VGA 1-HDMI 2-TFT 
   uint8_t tft;// 0-ili9341 1-st7789 2-ili9341 ips
   uint8_t tft_invert;// инверсия tft дисплея
   uint8_t tft_rotate;//переворот TFT 0 - 0x00 180 -0xC0
   uint8_t tft_rgb;// Режим цвета BGR-0x01 RGB-0x00
   uint8_t tft_bright;
   uint8_t mashine;// конфигурация ZX Spectrum 
   uint8_t trdos_version;// версия TR-DOS
   uint8_t cpu_version; //версия cpu z80

   uint8_t autorun;// автозагрузка образа или диска
   uint8_t turbo;// normal/turbo//fast

   uint8_t joyMode;
   uint8_t mouse_dpi;
   uint8_t sound_fdd;
   uint8_t audio_buster;
   uint8_t spi_bd;
   uint8_t tape_mode; // 0=fast (ROM trap), 1=normal (real tape emulation)
   uint8_t beep_mode; // 0 - pwm  , 1 - gpio

   uint32_t shift_img;

   uint8_t pallete; //номер палитры
   uint8_t FileAutorunType;
   char DiskName[4][LENF+1];
   char Disks[4][DIRS_DEPTH*(LENF+16)];//110 // 5*22
   char activefilename[DIRS_DEPTH*(LENF+16)]; // 400 // 5*22
  
   float hdmi_fdiv; // 1.0-> 90Hz  (cpu=378MHz)  1.5->60Hz (cpu=378MHz)
}  conf;

void config_defain(void);// процедура конфигурации по умолчанию
void config_save(const char *filename);
void config_load(const char *filename);
//----------------------------------------------------------


extern	uint64_t    t2;// tr-dos
extern uint64_t tindex;
//===========================================================
// сообщения внизу и громкость
extern  uint16_t wait_msg;
extern  uint8_t msg_bar; 
extern  uint8_t vol_i2s; 
extern   uint8_t vol_ay; 
//-------------------------------------------
// usb устройства
extern uint8_t usb_device;// 1 клавиатура 2 мышь 3 клавиатура+мышь
extern uint8_t gamepad_hid;// джойстик
extern uint8_t gamepad_addr;
extern uint8_t gamepad_instance;

extern uint16_t vid, pid;
extern uint8_t joy_k ;//#1F - кемпстон джойстик 0001 1111
//uint8_t joy_ext ;//дополнительные кнопки геймпадов
#define SELECT_J 0x88
#define START_J  0x84
extern kb_u_state kb_st_ps2;
extern bool flag_usb_kb;
extern uint8_t mouse[8] ;
//--------------------------------------------
// изображение видеодрайвер
extern int T_per_line; 
extern char dir_patch_info [DIRS_DEPTH*(LENF+16)];
//---------------------------------------------
//u_int16_t keytime;// нажатие кнопки при автозапуске 
extern int ScreenShot_Y;// координата Y для вывода скриншота

//---------- PSRAM -----------------  
extern bool psram_avaiable;
#define PSRAM_BASE 0x11000000 // Базовый адрес PSRAM в адресном пространстве

//----------------------------------------------------
extern uint8_t  dt_cpu;
///////////////// nmi
extern uint8_t z80_pc;

extern uint8_t hardAY_on_off;
extern uint8_t beep_data_old;
extern uint8_t beep_data;
extern uint8_t beep_pin;
//////////////////////////////////


  // Автодетект видеовыхода
#define VIDEO_AUTO 0  
#define VIDEO_VGA  1
#define VIDEO_HDMI 2
#define VIDEO_TFT  3



 uint8_t video_autodetect(void);


 uint8_t video_autodetect(void);
//=====================================================================
// GS глобальные переменные
extern uint8_t tx_buffer[64];// буфер picobus

extern bool flag_gs;
#define SERVICE_COMMAND 0x00 // служебная команда 

#define GS_INFO         0x00 // информация о GS
#define GS_RESET        0x01 // Сброс pico GS
#define TS_VOLUME       0x02 // второй байт команды третий значение
#define TS_RESET        0x03 // второй байт команды
#define TS_BUSTER       0x04 // второй байт команды третий  значение 
#define MUTE_GLOBAL     0x05 // Полное отключение звука



#define PICOBUS_CONNECT    0x77    // команда инициализации
// дефайны эмуляции портов GS 
#define GS_READ_IN_B3      0x01     // in(0xB3)
#define GS_STATUS_IN_BB    0x02      // in(0xBB)
#define GS_WRITE_OUT_B3    0x03     // out(0xB3)
#define GS_COMMAND_OUT_BB  0x04   // out(0xBB)
// дефайны эмуляции портов Z-Controler
#define ZC_READ_IN_57      0x05
#define ZC_READ_IN_77      0x06
#define ZC_WRITE_OUT_57    0x07
#define ZC_WRITE_OUT_77    0x08
// дефайны эмуляции портов AY TS
#define TS_READ_IN_FFFD    0x09
#define TS_READ_IN_BFFD    0x0A // ;)
#define TS_WRITE_OUT_FFFD  0x0B
#define TS_WRITE_OUT_BFFD  0x0C
// дефайны эмуляции портов RTC версия для NOVA
#define RTC_READ_IN_89     0x0D
#define RTC_WRITE_OUT_89   0x0E
#define RTC_WRITE_OUT_88   0x0F
// дефайны эмуляции портов RTC версия для SMUC
#define RTC_READ_IN_DFBA   0x0D
#define RTC_WRITE_OUT_FFBA 0x0E
#define RTC_WRITE_OUT_DFBA 0x0F
// дефайны эмуляции портов MIDI
#define MIDI_IN            0x10
#define MIDI_OUT           0x11



extern uint8_t command_gs;    // Порт 1
extern uint8_t data_zx;
extern uint8_t data_gs;
extern uint8_t status_gs;     // Порт 4: D0 - флаг команд, D7 - флаг данных

extern uint8_t z_controler_cs;

#include "fdi_stream.h"
extern fdi_sector_info_t *sector_inf;

#ifdef LEDBLINK
void led_blink(void);
     #endif

//####################################################
bool config_ini_save(const char *filename); 
bool config_ini_load(const char *filename);
//####################################################
     #endif


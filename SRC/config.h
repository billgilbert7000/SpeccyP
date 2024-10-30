#pragma once
#include <stdio.h>

#include "drivers/video/vga.h"
#include "drivers/video/hdmi.h"
#include "drivers/ay/aySoft.h"
#include "drivers/ay/aySoft1.h"
#include "drivers/i2s/i2s.h"

#include "screen_util.h"
#include "joy.h"


//#define  WS2812// RGB
#define  TEST // 
#define  TRDOS_1// версия TR-DOS

#ifdef TRDOS_0
#include "drivers\trdos\trdos.h"
#else
#include "drivers\trdos\wd1793.h"
#endif

uint32_t dtcpu;
uint32_t dt_cpu;

#include "drivers\ws2812\ws2812.h"
//////////////////////////////////////////
#include "I2C_rp2040.h"// это добавить
#include "usb_key.h"// это добавить
//#define DELAY_KEY   50 // задержка в меню настроек
//#define DELAY_FM_KEY 50 // задержка в меню файлов
#define OUT_SND_MASK (0b00011000)
#define SHOW_SCREEN_DELAY 50  //500  в милисекундах
//**********************************************
//zx_mashine.c
//#define PSRAM_ALL_PAGES  // использование памяти PSRAM кроме страниц 5 и 7
#define PSRAM_TOP_PAGES // использование памяти PSRAM только верхних от 8 и выше
//-----------------------------------------------------------------------------------------
// частота RP2040
#define CPU_KHZ 378000//378000//276000 //252000 264000 ////set_sys_clock_khz(300000, true); // main.h252000//
//#define DEL_Z80 CPU_KHZ/14000 //80 // main.h 3500 turbo 7000
#define Z80_3500 CPU_KHZ/3500
#define Z80_7000 CPU_KHZ/7000
#define Z80_14000 CPU_KHZ/14000
uint32_t ticks_per_cycle;//
uint64_t ticks_per_frame;// 71680- Пентагон //70908 - 128 +2A
int inx_tick_int;
uint8_t ticks_per_int;
bool int_en;
#define VOLTAGE VREG_VOLTAGE_1_30 //VREG_VOLTAGE_1_20 //	vreg_set_voltage(VOLTAGE); // main.h
//-------------------------------------------------------------------------------------------------	
#define PSRAM_DIV  1.5//     1.4 // делитель частоты psram если 315 то 1.4 если меньше то 1.0
//======================================================================================================
// pio & sm RP2040
#define PIO_PS2 pio0 // клавиатура ps/2
#define SM_PS2  2    // клавиатура ps/2

#define PIO_PSRAM pio1 // PSRAM
#define SM_PSRAM  -1    // PSRAM

#define pioAY595 pio1 // hard AY 595
#define sm_AY595 1    // 1 hard AY 595

#define PIO_I2S pio1 // i2s Sound
// sm = -1
#define I2S_DATA_PIN 26
#define I2S_CLK_BASE_PIN 27




#define PIO_VIDEO pio0
#define PIO_VIDEO_ADDR pio0
#define SM_VIDEO -1
#define SM_CONV -1

//=======================================================================================================
// AY
// выбор пина 29 или 21
//#define NO_PSRAM_21 

#if defined(NO_PSRAM_21)   
#define CLK_AY_PIN (21)

#else
#define CLK_AY_PIN (29)
#endif


#define CLK_LATCH_595_BASE_PIN (26)
#define DATA_595_PIN (28)
//!!!!!!!!!!!!!!!!!!!!!!!!!!
//#define CLK_AY_PIN (29)
//#define CLK_AY_PIN (17)
//#define CLK_AY_PIN (21)
//!!!!!!!!!!!!!!!!!!!!!!!!!!
//======================================================================================================
  //  INVOFF 0x20  
  //  INVON 0x21 


#define   VGA// HDMI// VGA // HDMI TFT 
#define TFT_INV 0x20

#define ILI9341 
//#define ST7789

#if defined(VGA)
#define VIDEO_OUTPUT 0
#endif

#if defined(HDMI)
#define VIDEO_OUTPUT 1
#endif

#if defined(TFT)
#define VIDEO_OUTPUT 2
#endif

// 126MHz SPI
//#define SERIAL_CLK_DIV 3.0f
#define MADCTL_BGR_PIXEL_ORDER (1<<3)
#define MADCTL_ROW_COLUMN_EXCHANGE (1<<5)
#define MADCTL_COLUMN_ADDRESS_ORDER_SWAP (1<<6)
#ifdef ILI9341
#define   MADCTL (MADCTL_ROW_COLUMN_EXCHANGE | MADCTL_BGR_PIXEL_ORDER) // 0x28
#else
    // ST7789
#define    MADCTL (MADCTL_COLUMN_ADDRESS_ORDER_SWAP | MADCTL_ROW_COLUMN_EXCHANGE) // 0x60  Set MADCTL
#endif
 

//-----------------------------------------------------------------------------------------------
// настройки Hard TurboSound

#define HTS_DEFAULT  // программа  стейт машины для работы с регистром сдвига 595 без задержки 
//#define HTS_SAGIN    // программа  стейт машины для работы с регистром сдвига 595 c задержкой


// AY-3-8910 
#define volume 1
#define volumeI2S 16
#define AY_DELTA 2
#define AY_SAMPLE_RATE -9//(-1000000 / 88200)// -18//(-1000000 / 55000) //=-9 //30310  31250
//#define PWM_WRAP 2000//0x0fff//255// максимальное число заполнения ШИМ ИСПОЛЬЗУЕТСЯ ПРИ ИНИЦАЛИЗАЦИИ 
#define FRQ_UP 0
//#define FILTER_ON     // фильтр частот
//-----------------------------------------------------------------------------------------------
//#define PICO_FLASH_SPI_CLKDIV 4
// #define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
//PICO_FLASH_SPI_CLKDIV

#define FW_VERSION "v0.15.3"
#define FW_AUTHOR "Speccy_P"

//-----------------------------------------------------------------------------------------------
#define PENT128  0
#define PENT512  1
#define PENT1024 2
#define SCORP256 3
#define PROFI1024 4
#define GMX2048  5
#define ZX4096 6
#define NOVA256 7

//
#define CONF_MASHINE PENT128 
 			// 0 soft AY
			// 1 soft TS
			// 2 hard AY
			// 3 hard TS
//#define conf.ay 1 // hard ts


#define PIN_ZX_LOAD (22)
//функции вывода звука спектрума
#define ZX_AY_PWM_PIN0 (26)
#define ZX_AY_PWM_PIN1 (27)
#define ZX_BEEP_PIN (28)

#define LED_PIN 25 // индикатор trdos

//**********************************************
// TFT
#define TFT_CLK_PIN (13)
#define TFT_DATA_PIN (12)
#define TFT_RST_PIN (8)
#define TFT_DC_PIN (10)
#define TFT_CS_PIN (6) 
#define TFT_LED_PIN  (9)
//#define SERIAL_CLK_TFT 0x28000 //fdiv32:163840 #28000  126 MHz
#define SERIAL_CLK_TFT 0x40000


//uint8_t tft_type; // 0 - ili9341  1 -st7789


//**********************************************
#define PIO_VIDEO pio0
#define PIO_VIDEO_ADDR pio0

#define SCREEN_H (240)
#define SCREEN_W (320)
#define V_BUF_SZ ((SCREEN_H+1)*SCREEN_W/2)


#define VGA_BASE_PIN (6)

#define VGA_DMA_IRQ (DMA_IRQ_0)

#define TEXTMODE_COLS 80
#define TEXTMODE_ROWS 30

#define RGB888(r, g, b) ((r<<16) | (g << 8 ) | b )

#ifdef NUM_V_BUF
    extern  bool is_show_frame[NUM_V_BUF];
    extern int draw_vbuf_inx;
    extern int show_vbuf_inx;
#endif
       uint8_t g_gbuf[V_BUF_SZ];
extern uint8_t g_gbuf[];
extern uint8_t color_zx[16];
bool vbuf_en;// экран эмуляции
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
//======================================================





//------------------------------------------------
// Joystick
 uint8_t joy_key_ext;


//--------------------------------------------------
// fdd & trdos
uint32_t sclDataOffset;
//unsigned char* track0;
char track0 [0x0900];
bool write_protected;
uint8_t DriveNum;
uint8_t sound_track;

//---------------------------------
// tape loader
uint32_t seekbuf;
char TapeName[17];
bool enable_tape;
 //-------------------------------
//переменные состояния спектрума
uint8_t zx_Border_color;
//uint8_t mybuf[256];
//---------------------------------
//SPI
uint baudrate;

//---------------------------------
uint8_t psram_size;
//---------------------------------
uint8_t z80_freq; // частота z80

//---------------------------------
// other
char temp_msg[60]; // Буфер для вывода строк
//Quorum
bool main_nmi_key ;
//=====================================================
// дефайны файлов
#define ZX_RAM_PAGE_SIZE 0x4000
#define DIRS_DEPTH (5)// глубина директорий 8
#define MAX_FILES  (128)// максимум 128 файлов в каталоге
#define SD_BUFFER_SIZE 0x4000  //Размер буфера для работы с файлами
#define LENF 22  // 22
#define LENF1 LENF-1
//=====================================================
//char Disks[4][160] = {"", "", "", ""};
//char Disks[4][DIRS_DEPTH*(LENF+16)];//160 // 5*16
//char DiskName[4][LENF+1];


// данные конфигурационного файла
struct data_config
{
   uint16_t version;// версия конф. файла  
   uint8_t mashine;
   uint8_t ay;// конфигурация ay 
   uint8_t autorun;// автозагрузка образа или диска
  // uint8_t ay_filter;// sound filter
   uint8_t turbo;// turbo/normal
   uint8_t ampl_tab;
   u_int8_t joyMode;
   u_int8_t sound_ffd;
   u_int8_t spi_bd;
   u_int8_t tft_bright;
   u_int8_t vol_bar; //громкость
   bool sclConverted;
   char DiskName[4][LENF+1];
   char Disks[4][DIRS_DEPTH*(LENF+16)];//110 // 5*22
 //  char Disks[4][256];//110 // 5*22
  char activefilename[DIRS_DEPTH*(LENF+16)]; // 400 // 5*22
 // char activefilename[256]; // 400 // 5*22
} conf;

void config_defain(void);

//==========================================================
// дефайны меню настроек
#define M_RAM 0
#define M_SOUND 1
//#define M_SOUND_FILTER  2
#define M_TURBO  2
#define M_JOY 7
#define M_SPI 4
#define M_NOISE_FDD 5

#define M_AY_TABLE 3
#define M_TFT_BRIGHT 8
#define M_AUTORUN 6

#define M_SAVE_CONFIG 10
#define M_SOFT_RESET 11
#define M_HARD_RESET 12
#define M_EXIT 13


//----------------------------------------------------------
/* 
	0" Memory      ",
	1" Sound       ",
    2" Turbo       ",
    3" AY Table TEST",
    4" SPI baudrate",
    5" Sound FFD   ",
    6" AutoRUN   ",
    7" Joystick    ",
    8" Bright TFT  "
	10" Save config ",
	11" Z80  reset  ",
	12" Hard reset  ",
    13" Exit        ",
     */

    static	uint64_t    t2=0;// tr-dos
//static uint64_t tindex0 =0;
//static uint64_t tindex1 =0;
static uint64_t tindex =0;
uint8_t DR_trdos;
//===========================================================
// сообщения внизу и громкость
   uint16_t wait_msg;
   uint8_t msg_bar; 
  // uint8_t vol_bar; 
   uint8_t vol_i2s; 
   uint8_t vol_ay; 
//-------------------------------------------
// usb устройства
uint8_t usb_device;// 1 клавиатура 2 мышь 3 клавиатура+мышь
uint8_t gamepad_hid;// джойстик
uint8_t gamepad_addr;
uint8_t gamepad_instance;

uint16_t vid, pid;
uint8_t joy_k ;//#1F - кемпстон джойстик 0001 1111
//uint8_t joy_ext ;//дополнительные кнопки геймпадов
#define SELECT_J 0x88
#define START_J  0x84

//--------------------------------------------
// изображение видеодрайвер
int T_per_line; 
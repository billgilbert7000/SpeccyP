#include "config.h" 

#include <stdio.h>

#include "hardware/gpio.h"
//#include "hardware/adc.h"

#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "hardware/irq.h"
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"



//#######################################
#ifdef PICO_RP2350
#include <hardware/structs/qmi.h>
#include <hardware/structs/xip.h>
#include <hardware/regs/sysinfo.h>
#endif
//######################################



#include "screen_util.h"

#include "tf_card.h"
#include "ff.h"

#include "util_z80.h"
#include "util_sna.h"
#include "util_tap.h"
#include "string.h"
#include "util_sd.h"
#include "util_trd.h"

#include "kbd_img.h"

#include "utf_handle.h"

#ifdef MURM1
#include "psram_spi.h"
#endif

#include "HDMI.h"
#include "aySoft.h"
#include "g_config.h"

#include "tft_driver.h"

#include "ps2.h"

#ifdef  GENERAL_SOUND   
#include "gs_picobus.h"
#endif

#include "aySoft.h"

#include "zx_emu/zx_machine.h"
#include "zx_emu/Z80.h"

//////////////////////////////////////////
#include "tusb.h"

//#include "I2C_rp2040.h"// это добавить
#include "usb_key.h"// это добавить
#include "joy.h"
#include "zx_emu/zx_machine.h"
//#include "drivers\psram\psram_spi.h"
//#include "fatfs_sd.h"
#include "diskio.h"
extern Z80 cpu;
int real_psram_freq;
int real_flash_freq;
uint8_t vout_select;
/////////////////////////////////////////
// headers
void file_manager (void);   
void file_info (void);   
void file_select_trdos(void);
void setup_zx(void);
//bool save_config(void);
void  config_init(void);

void help_zx(void); //F1
void pause_zx(void);//  [Pause Break]
void help_keyboard(void); // F4
//void turbo_switch(void);
void led_trdos(void);
void load_slot(void);
void save_all(void);
void save_slot(void);
void disk_autorun (void);
void disasm(void);
void  fast (init_psram_board_all_version)(void);

#if PICO_RP2350
void __no_inline_not_in_flash_func(init_psram_butter)(uint cs_pin);
void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin);
static uint32_t __no_inline_not_in_flash_func(psram_b_size)(void);
uint32_t get_psram_size();
//bool is_psram_enabled();
#endif
uint8_t psram_pin_cs; // переопределение пина PSRAM для пиморони 
//---------------------------------------------------------------
// переменные
char afilename[LENF];
//===============================================================
void nmi_zx()
{
   if (main_nmi_key)  
   z80_nmi(&z1->cpu);
}
//--------------------------------------------------

extern bool im_ready_loading;

uint8_t saves[11];

static uint32_t last_action = 0;
//uint32_t scroll_action = 0;
//uint16_t scroll_pos =0;
bool is_menu_mode=false;

extern ZX_Input_t zx_input;


//---------------------------------------------------------
//инициализация переменных для меню
//	bool read_dir = true;
	int shift_file_index=0;
	int cur_dir_index=0;
	int cur_file_index=0;
	int cur_file_index_old=0;
	int N_files=0;
	char current_lfn[LENF];
	//uint64_t current_time = 0;
	//char icon[3];
//	uint8_t sound_reg_pause[18];

	int lineStart=0;	

	int tap_block_percent = 0;
	


	bool old_key_menu_state=false;
	bool key_menu_state=false;
	bool is_new_screen=false;
	
	bool old_key_help_state=false;
	bool key_help_state=false;
	bool help_mode_draw=false;

	bool need_reset_after_menu=false;

	bool old_key_pause_state=false;
	bool key_pause_state=false;

	bool is_osk_mode=false;
	uint8_t current_key=0;

	char save_file_name_image[25];


//---------------------------------------------------------
    uint8_t video_select(void)
    {
        if (conf.vout==VIDEO_AUTO) return video_autodetect(); 
        if (conf.vout>VIDEO_TFT) conf.vout=0;
        return conf.vout;
    }
//---------------------------------------------

//----------------------------
	// меню  initial
	 char __in_flash() *menu_initial[4]={
	//char*  menu_initial[1]={	
	" ",
	
};
 	// меню setup
	char __in_flash() *menu_setup_tft[16]={
        " Model & RAM   ",
        " Sound out     ",
        " Speed Mode    ",
        " Joystick      ",
        " AutoRun       ",
        " Setting TFT   ",
        " Sound setup   ", 
        " Advanced setup", 
        "               ",
        " Save config   ",
        " Z80  reset    ",
        " Hard reset    ",
        " Power OFF     ",
 #ifndef MOS2
        " Update mode   ",
 #else    
        " Murmulator OS ", 
 #endif       
        " Exit          ",
        "               ",
        
    };



	// меню setup
	 char __in_flash() *menu_setup[16]={
    " Model & RAM   ",
	" Sound out     ",
    " Speed Mode    ",
    " Joystick      ",
    " AutoRun       ",
    " Palette       ",
    " Sound setup   ", 
    " Advanced setup", 
    "               ",
	" Save config   ",
	" Z80  reset    ",
	" Hard reset    ",
    " Power OFF     ",
 #ifndef MOS2
    " Update mode   ",
 #else    
    " Murmulator OS ", 
  #endif  
    " Exit          ",
	"               ",
	
};

// меню help keyboard
	 char __in_flash() *menu_keyboard[1]={	
	"",
	
};
// меню help
	char __in_flash() *menu_help[17]={
	"[F11], [Insert] file browser",
	"[F12], [HOME] settings menu     ",
    "[F2] Save slots [F3] Load slots",
    "[F4] Help ZX Keyboard",
    "[F5] Save slots 0 and config",
    "[F7] Volume down [F8] Volume up",
    "[F10] Normal/Turbo/Fast",
    "[F9] NMI key ",
    "[ESC] exit almost all menus ",
    "[END] Dissasembler",
    "[END] USB Update mode only here",
    "  ",
    "Video driver, etc. @Alex_Eburg ",
    "https://t.me/ZX_MURMULATOR ",
    "  ",
    "The author of the compilation:",
	"https://t.me/const_bill",
	
};
	// меню ram
	char __in_flash() *menu_ram[9]={
	//char*  menu_ram[7]={	
	" Pentagon 128     ",
    " ZX Spectrum 48   ",
	" Pentagon 512     ",
	" Pentagon 1024    ",
	" Scorpion ZS 256  ",
	//" Profi       1024 ",
 #ifdef NO_GMX
     " No  ScorpionGMX ", 
 #else         
	" ScorpionGMX 2048 ",
 #endif
	//" Unreal      4096 ",
    " Navigator 256    ",
    " MurmoZavr 8000K  ",
    " Pentagon 512CASH ",
};


#ifdef RP2350_256K
	// меню menu_ram_48_128_256
	char __in_flash() *menu_ram_128_48[4]={
        //char*  menu_ram[7]={	
        " Pentagon 128     ",
        " ZX Spectrum 48   ",
        " Scorpion ZS 256  ",
        " Navigator 256    ",
       };
#else
	// меню menu_ram_128_48
	char __in_flash() *menu_ram_128_48[2]={
        //char*  menu_ram[7]={	
        " Pentagon 128     ",
        " ZX Spectrum 48   ",
       };
#endif


	// меню sound
	 char __in_flash() *menu_sound[8]={
	//char*  menu_sound[4]={	
	" Soft AY-3-8910  ",
	" Soft TurboSound ",
    " I2S AY-3-8910   ",
    " I2S TurboSound  ",
    " Hard AY-3-8910  ",
	" Hard TurboSound ",
    " Hard TSFM *     ",
    " Hard TS+SAA *   ",
};
	// меню joy
	char __in_flash() *menu_joy[6]={
	" Kempston  NES Joy ",
	" Kempston   Arrows ",
	" Interface2 Arrows ",
	" Cursor     Arrows ",
    " Q A O P M  Arrows ",
    " Kempston   WASDKL ",
    };
  //  char __in_flash() *menu_trdos[2];
/* extern	char __in_flash() *menu_trdos[2]={
    " TR-DOS 5.04T  ",
    " TR-DOS 5.05D  ",
    };    */
	// меню advanced
/* 	 char __in_flash() *menu_advanced[10]={
    " Volume       ",    
	" I2S  buster  ",
	" Noise FDD    ",
	" Volume LOAD  ",
    " Mouse Speed  ",
    " Video OUT    ",
    menu_trdos[conf.trdos_version]," TR-DOS       ",
    " Z80 model    ",
    " Save config  ",
    " Return       ",
    };
 */



    // menu_autorun
	 char __in_flash() *menu_autorun[3]={
    " OFF              ",    
	" File TR-DOS      ",
	" QuickSave Slot 0 ",
    };

    //menu_speed
	 char __in_flash() *menu_speed[2]={
    " NORMAL INT  50Hz ",    
	//" TURBO  INT  50Hz ",
	" FAST   INT 100Hz ",
    };



	// меню table of pallete
	char __in_flash() *menu_pallete[12]={
	" 0.default       ",
	" 1.spectaculator ",
	" 2.base-graph    ",
	" 3.sc gray       ",
    " 4.MARS1         ",
	" 5.OCEAN1        ",
	" 6.Unreal-Grey 1 ",
	" 7.alone 1       ",
	" 8.pulsar 1      ",
    " 9.HAH2          ",
	" 10.UNREAL       ",
	" 11. HAH         ",
    };

	// меню table of setup_tft
	char __in_flash() *submenu_setup_tft[11]={
       
        "  ili9341       ",
        "  ili9341 ips   ",
        "  st7789        ",
        "  Rotate        ",
        "  Inversion     ",
        "  Color         ",
        "  Save & reset  ",
        "              ",
        "    Bright",
        "  ",
        " <=                 => "
        };
    
//=========================================================
// Режимы джойстика
void (*joy_scan)(void);// определение указателя на функцию
//
void joy_mode_0(void) // Kempston  NES Joy  стрелки работают как и надо
{
    if ((kb_st_ps2.u[2] & KB_U2_UP))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 3;
    }; //|(data_joy==0x08)
    if ((kb_st_ps2.u[2] & KB_U2_DOWN))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 4;
    }; //|(data_joy==0x04)
    if ((kb_st_ps2.u[2] & KB_U2_LEFT))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[3] |= 1 << 4;
    }; //|(data_joy==0x02)
    if ((kb_st_ps2.u[2] & KB_U2_RIGHT))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 2;
    };
}
void joy_mode_1(void) // Kempston   Arrows
{
    data_joy = 0;
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
        data_joy |= 0b00000001;
        // printf("KBD Right\n");
    };
    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
        data_joy |= 0b00000010;
        // printf("KBD Left\n");
    };
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
        data_joy |= 0b00000100;
        // printf("KBD Down\n");
    };
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
        data_joy |= 0b00001000;
        // printf("KBD Up\n");
    };
    if (JOY_FIRE)
    {
        data_joy |= 0b00010000;
        // printf("KBD Alt\n");
    };
    zx_input.kempston = (uint8_t)(data_joy);
}
void joy_mode_2(void) // Interface2 Arrows
{
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
        zx_input.kb_data[4] |= 1 << 1;
    };
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
        zx_input.kb_data[4] |= 1 << 2;
    };
    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
        zx_input.kb_data[4] |= 1 << 4;
    };
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
        zx_input.kb_data[4] |= 1 << 3;
    };
    if (JOY_FIRE)
    {
        zx_input.kb_data[4] |= 1 << 0;
    };
}
void joy_mode_3(void) // Cursor     Arrows
{
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
        zx_input.kb_data[4] |= 1 << 3;
    };
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
        zx_input.kb_data[4] |= 1 << 4;
    };
    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
        zx_input.kb_data[3] |= 1 << 4;
    };
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
        zx_input.kb_data[4] |= 1 << 2;
    };
    if (JOY_FIRE)
    {
        zx_input.kb_data[4] |= 1 << 0;
    };
}
void joy_mode_4(void) // Q A O P M  Arrows
{
    if (kb_st_ps2.u[2] & KB_U2_UP)
    {
        zx_input.kb_data[2] |= 1 << 0; // Q
    };
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
        zx_input.kb_data[1] |= 1 << 0; // A
    };
    if (kb_st_ps2.u[2] & KB_U2_LEFT)
    {
        zx_input.kb_data[5] |= 1 << 1; // O
    };
    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
        zx_input.kb_data[5] |= 1 << 0; // P
    };
    if (JOY_FIRE)
    {
        zx_input.kb_data[7] |= 1 << 2; // M
    };
}

void joy_mode_5(void) // W A S D K L  to Kempston
{
    data_joy = 0;
    if (kb_st_ps2.u[0] & KB_U0_D) // D
    {
        data_joy |= 0b00000001; // right
    };
    if (kb_st_ps2.u[0] & KB_U0_A) // A
    {
        data_joy |= 0b00000010; // left
    };
    if (kb_st_ps2.u[0] & KB_U0_W) // W
    {
        data_joy |= 0b00001000; // up
    };
    if (kb_st_ps2.u[0] & KB_U0_S) // S
    {
        data_joy |= 0b00000100; // down
    };
    if (kb_st_ps2.u[0] & KB_U0_K) // K
    {
        data_joy |= 0b00010000; // fire 1
    };
    if (kb_st_ps2.u[0] & KB_U0_L) // L
    {
        data_joy |= 0b00100000; // fire 2
    };
    zx_input.kempston = (uint8_t)(data_joy);

    // Обработка стрелок
    if ((kb_st_ps2.u[2] & KB_U2_UP))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 3;
    }; //|(data_joy==0x08)
    if ((kb_st_ps2.u[2] & KB_U2_DOWN))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 4;
    }; //|(data_joy==0x04)
    if ((kb_st_ps2.u[2] & KB_U2_LEFT))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[3] |= 1 << 4;
    }; //|(data_joy==0x02)
    if ((kb_st_ps2.u[2] & KB_U2_RIGHT))
    {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 2;
    };
}
//
void joy_redirecting(void)
{
    switch (conf.joyMode)
    {
    case 0:
        joy_scan = joy_mode_0;
        break;
    case 1:
        joy_scan = joy_mode_1;
        break;
    case 2:
        joy_scan = joy_mode_2;
        break;
    case 3:
        joy_scan = joy_mode_3;
        break;
    case 4:
        joy_scan = joy_mode_4;
        break;
    case 5:
        joy_scan = joy_mode_5;
        break;
    }
}
//=========================================================
void pico_reset(){

    #ifdef GENERAL_SOUND
    sys_GS(GS_RESET);// принудителный сброс GS
 //   sleep_ms(1000);
    #endif
//#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))

//AIRCR_Register = 0x5FA0004;
	watchdog_enable(1, 1);// сброс watch dog
	while(1);


}
//-----------------------------------------------------
//bool b_beep;
//bool b_save;


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

void ZXThread(){
	zx_machine_init();
	zx_machine_main_loop_start(); // главный цикл выполнения команд Z80
	return ;
}

void gpio_in_init(uint gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

void gpio_out_init(uint8_t gpio)
{
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_disable_pulls(gpio); // Отключить подтяжки (по умолчанию)
    gpio_put(gpio, 0);
}

//=================================================================================================================================
#ifndef  GENERAL_SOUND     
bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt)
{
if (!is_menu_mode)          //(is_menu_mode == 0)
{
   audio_out();
    return true;
}
 return true;
}
#endif
//============================================================================
bool zx_flash_callback(repeating_timer_t *rt) {zx_machine_flashATTR();return true;};
//============================================================================
//bool zx_int(repeating_timer_t *rt) {zx_generator_int();return true;};
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifdef PICO_RP2350 

#if (FLASH_MAX_FREQ_MHZ!=166)
static void __no_inline_not_in_flash_func(set_flash_timings)(void) {
    const int clock_hz = CPU_MHZ * 1000000;
    const int max_flash_freq = FLASH_MAX_FREQ_MHZ * 1000000;//FLASH_MAX_FREQ_MHZ
    int divisor = (clock_hz + max_flash_freq - (max_flash_freq >> 4) - 1) / max_flash_freq;
    if (divisor == 1 && clock_hz >= 100000000) {
        divisor = 2;
    }
     real_flash_freq = CPU_MHZ/divisor;

    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }
    qmi_hw->m[0].timing = 0x60007000 |
                        rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                        divisor << QMI_M0_TIMING_CLKDIV_LSB;
}
#endif
//############################################################
void fast(init_pico)(void) // настройка и разгон для RP2350
{  
    volatile uint32_t *qmi_m0_timing=(uint32_t *)0x400d000c;
    vreg_disable_voltage_limit();
    vreg_set_voltage(VOLTAGE_MIN);

#if (FLASH_MAX_FREQ_MHZ==166)
    real_flash_freq = CPU_MHZ/3;

    *qmi_m0_timing = 0x60007204; //  ???
    set_sys_clock_khz(CPU_KHZ, 0);
    *qmi_m0_timing = 0x60007303;   // ???
#else 
     set_flash_timings();
     set_sys_clock_khz(CPU_KHZ, 0);
#endif
}

#else
void fast(init_pico)(void) // настройка и разгон для RP2040
{  
   // hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
   // sleep_ms(10);
    vreg_set_voltage(VREG_VOLTAGE_1_30);
   sleep_ms(500);
   // запуск на пониженной частоте 
   set_sys_clock_khz(CPU_KHZ , false);

}

#endif
//========================================================================
static bool  rp2350a;
static uint inx=0;		


void init_and_info()
{

/* //#else // RP2040 или RP2350A
   for (int gpio = 0; gpio < 30; gpio++) {
    gpio_init(gpio);          // Сброс в SIO, вход
    gpio_disable_pulls(gpio); // Отключить подтяжки (по умолчанию)
    gpio_set_dir(gpio, GPIO_IN); // Направление: вход
    }
//#endif     */

#if LED_BOARD != 255
    gpio_init(LED_BOARD);
    gpio_set_dir(LED_BOARD, GPIO_OUT);
//	g_delay_ms(100);
    gpio_put(LED_BOARD, 1);
#endif 

#ifdef PICO_RP2350 
  // определение RP2350 A или B  
     rp2350a = (*((io_ro_32*)(SYSINFO_BASE + SYSINFO_PACKAGE_SEL_OFFSET)) & 1);
      psram_pin_cs = rp2350a ? PSRAM_BUTTER_PIN_CS : 47;







    // для корректного запуска с бутербродом PSRAM  
    gpio_init(psram_pin_cs);
    gpio_set_dir((psram_pin_cs), GPIO_IN);
    gpio_pull_up(psram_pin_cs); // 
    gpio_put(psram_pin_cs, 1);

// Weact RP2350A v20 GPIO 23 
// MODE=0 (PFM — Pulse Frequency Modulation)
// MODE=1 (PWM — Pulse Width Modulation)
#if POWER_MODE_WEACT != 255
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, POWER_MODE_WEACT);
#endif 
//--------------------------------------
#endif

  #if PI_CARD // ???? это для того чтобы настроить выходы для PICARD 1 ИНАЧЕЕ МИКРОСХЕМЫ ОБВЯЗКИ ГРЕЮТСЯ
        pio_set_gpio_base(PIO_PS2, 0x10);//использование на pio0 GPIO 16...48
        gpio_init(40);
        gpio_set_dir(40, GPIO_OUT);
        gpio_put(41,1);

        gpio_init(41);
        gpio_set_dir(41, GPIO_OUT);
        gpio_put(41,1);   
  #endif

  #ifdef WS_ZERO2
        pio_set_gpio_base(PIO_VIDEO, 0x10);//использование на pio0 GPIO 16...48
        gpio_init(46);//DVI_CEC
        gpio_set_dir(46, GPIO_OUT);
        gpio_put(46,1);

        gpio_init(45);//DVI_SCL
        gpio_set_dir(45, GPIO_OUT);
        gpio_put(45,1);

        gpio_init(44);//DVI_SDA
        gpio_set_dir(44, GPIO_OUT);
        gpio_put(44,1); 

#endif  




#ifdef  GENERAL_SOUND    
   gpio_out_init(BEEP_PIN);// на выход  для реалиации звука бипера
#endif

	//пин ввода звука
	gpio_in_init(PIN_ZX_LOAD);
    

 init_psram_board_all_version();// инициализация всех видов psram

//---------------------------------------------------------------------------

  init_fs = disk_initialize(0);// инициализация SD
    DIR fs;
 init_fs  =init_filesystem();// монтирование и инициализация SD

    config_init();

#ifdef PICO_RP2350 
vreg_set_voltage(conf.voltage);
#endif
    gpio_put(LED_BOARD, 0);
//------------------------------------------------------------------
    turbo_switch(); // переключение режима turbo
 
    joy_redirecting();// установка режима работы kempston joy
//--------------------------------------------------------------

#ifdef  HDMI_ONLY
     //  conf.hdmi_fdiv = 1.5;// 1.5-> 60Hz  (cpu=378MHz)
        conf.hdmi_fdiv = HDMI_DIV;
         vout_select= VIDEO_HDMI;
         startVIDEO(VIDEO_HDMI);// только HDMI
#else 
     //  conf.vout=video_filedetect();// определение видеовыхода по наличию определенного файла OLD!
         conf.hdmi_fdiv = HDMI_DIV;
         vout_select= video_select();// автоопределение

       //   vout_select=VIDEO_TFT;
       //  conf.tft=2; // 0-ili9341 1-st7789 2-ili9341 ips
 
         startVIDEO(vout_select);     
#endif 
//

#ifdef DEVICE_HDMI90_I2S
        // conf.vout=VIDEO_HDMI; // только HDMI
         conf.type_sound=3; // только i2s
         conf.hdmi_fdiv = 1.0;// 1.0-> 90Hz  (cpu=378MHz)
         vout_select= VIDEO_HDMI;
         startVIDEO(VIDEO_HDMI);// только HDMI
#endif 

 
#ifdef  SOUND_I2S_ONLY
        conf.type_sound=3; // только i2s
#endif 

#ifdef  SOUND_PWM_ONLY
        conf.type_sound=1; // только pwm
#endif 

//#####################################################################
    	
	    convert_kb_u_to_kb_zx(&kb_st_ps2,zx_input.kb_data);
//#####################################################################        
// инициализация с выводом результата на дисплей
        zx_machine_enable_vbuf(false);
	    init_screen(g_gbuf,SCREEN_W,SCREEN_H);
        is_new_screen = true;
        #define YPOS FONT_H*2 
        #define XPOS FONT_W+2
        uint8_t y_info = YPOS; //координата по оси Y для вывода информации

    	draw_rect(0,0,SCREEN_W,SCREEN_H,CL_BLACK,true);//Заливаем экран 
	    draw_rect(9,9,SCREEN_W-20,SCREEN_H-100,CL_CYAN,false);//Основная рамка TEST

        draw_logo (156,160,CL_CYAN,CL_BLACK);

        draw_text(7+XPOS,0+YPOS,"ZX SpeccyP",CL_GRAY ,CL_BLACK);	
        draw_text_len(7+11*FONT_W+XPOS,YPOS,FW_VERSION,CL_GRAY ,CL_BLACK,16);

        #ifndef PICO_RP2040
        if (rp2350a) snprintf(temp_msg, sizeof temp_msg, "RP2350A %dMHz",CPU_MHZ);
        else snprintf(temp_msg, sizeof temp_msg, "RP2350B %dMHz",CPU_MHZ);
        #else
        snprintf(temp_msg, sizeof temp_msg, " RP2040 %dMHz",CPU_MHZ);    
        #endif
        draw_text(210+XPOS,YPOS,temp_msg,CL_GRAY ,CL_BLACK);



        draw_text((320-(34*FONT_W))/2,180+YPOS,"[F12]-Setup [F11]-Files [F1]-Help",CL_GREEN,CL_BLACK);
      //  draw_text((320-(20*FONT_W))/2,140+YPOS,"Forward to the past",CL_GRAY,CL_BLACK);

        y_info += 20;
        if (init_fs == FR_OK) 
        {
            tf_card_get_complete_info_string(temp_msg, sizeof(temp_msg));
            draw_text(10 + XPOS, y_info , temp_msg, CL_GREEN, CL_BLACK);
        }
        else
            draw_text(10 + XPOS, y_info , "SD card not detected", CL_RED, CL_BLACK);

        #if D_JOY_CLK_PIN!=255      
        d_joy_init();
	    decode_joy(); //????????


         #if !DEVICE_HDMI90_I2S
	      if(gpio_get(D_JOY_DATA_PIN))
          {
             y_info += 10;
          draw_text_len(10+XPOS,y_info,"NES Joy present",CL_GREEN,CL_BLACK,20);
          }
    /*      else 
           {
             y_info += 10;
          draw_text_len(10+XPOS,y_info,"NES Joy not found",CL_RED  ,CL_BLACK,20);
           } */
         #endif          
     #endif  
////////////////////////////////////////////////////////////////


	  //  это инициализация мыши ;)

	    mouse[0] = 0x00;
        mouse[1] = 0xff; 
        mouse[2] = 0xff; 
        mouse[3] = 0xff; 

start_PS2_capture(); //


    y_info += 10;
switch (type_psram)
{
case NOT_PSRAM:
    draw_text_len(10+XPOS,y_info,"PSRAM not found",CL_RED,CL_BLACK,16); 
    #ifdef RP2350_256K
	{
		if (conf.mashine==PENT128||conf.mashine==SPEC48||conf.mashine==SCORP256||conf.mashine==NOVA256) conf.mashine = conf.mashine;
        else conf.mashine = SCORP256;    
	}
    #else
	{
		if (conf.mashine > 1) conf.mashine = 0;
	}
    #endif
    psram_avaiable =0;
    break;


case BUTTER_PSRAM:
    snprintf(temp_msg, sizeof temp_msg, "PSRAM %d Mb QSPI CS:%d", size_psram, psram_pin_cs );//BUTTER  'butter'
	draw_text(10+XPOS,y_info,temp_msg,CL_GREEN,CL_BLACK);
    psram_type = 0;
    psram_avaiable =1;
    break;
case BOARD_PSRAM:
    snprintf(temp_msg, sizeof temp_msg, "PSRAM %d Mb SPI board", size_psram); //available
	draw_text(10+XPOS,y_info,temp_msg,CL_GREEN,CL_BLACK);
    psram_type = 1;
    psram_avaiable =1;
    break;
/* case NOT_PSRAM_PIN21:    
	draw_text(10+XPOS,y_info,"NO PSRAM / Clock AY on pin21",CL_LT_PINK,CL_BLACK);
    if (conf.mashine > 1) conf.mashine = 0;// only 128kB
    psram_avaiable = 0;
    break; */
case BOARD_PSRAM_NOSUPORT:
    snprintf(temp_msg, sizeof temp_msg, "PSRAM board is not supported");
	draw_text(10+XPOS,y_info,temp_msg,CL_BLUE,CL_BLACK);
    #ifdef RP2350_256K
	{
		if (conf.mashine==PENT128||conf.mashine==SPEC48||conf.mashine==SCORP256||conf.mashine==NOVA256) conf.mashine = conf.mashine;
        else conf.mashine = SCORP256;    
	}
    #else
	{
		if (conf.mashine > 1) conf.mashine = 0;
	}
    #endif
    psram_avaiable =0;
    break;
}

        #ifdef TEST_DEBUG

#ifdef GENERAL_SOUND
snprintf(temp_msg, sizeof temp_msg, "FLASH   %dMHz", real_flash_freq); 
        draw_text(210+XPOS,YPOS+10,temp_msg,CL_GRAY ,CL_BLACK); 
        snprintf(temp_msg, sizeof temp_msg, "PSRAM   N/A",real_psram_freq);
        if (type_psram==BUTTER_PSRAM) snprintf(temp_msg, sizeof temp_msg, "PSRAM   %dMHz",real_psram_freq); 
        else snprintf(temp_msg, sizeof temp_msg, "PSRAM   SPI");
      
        draw_text(210+XPOS,YPOS+20,temp_msg,CL_GRAY ,CL_BLACK); 

          snprintf(temp_msg, sizeof temp_msg, "  Ucpu   %.2f V ",table_voltage[conf.voltage]/ 100.0 ); 
        draw_text(210+XPOS,YPOS+30,temp_msg,CL_GRAY ,CL_BLACK); 
     //   snprintf(temp_msg, sizeof temp_msg, "PicoBus %dMbps",PICOBUS_SPEED); 
     //   draw_text(210+XPOS,YPOS+40,temp_msg,CL_GRAY ,CL_BLACK); 

        #else
//snprintf(temp_msg, sizeof temp_msg, " FLASH   %dMHz", real_flash_freq); 
    //    draw_text(204+XPOS,YPOS+10,temp_msg,CL_GRAY ,CL_BLACK); 
   //     snprintf(temp_msg, sizeof temp_msg, "PSRAM   N/A",real_psram_freq);
/*         if (type_psram==BUTTER_PSRAM) snprintf(temp_msg, sizeof temp_msg, "PSRAM   %dMHz",real_psram_freq); 
        else snprintf(temp_msg, sizeof temp_msg, " ");
        draw_text(210+XPOS,YPOS+20,temp_msg,CL_GRAY ,CL_BLACK);  */
    #ifdef PICO_RP2350 
        snprintf(temp_msg, sizeof temp_msg, "  Ucpu   %.2fV ",table_voltage[conf.voltage]/ 100.0 ); 
      // snprintf(temp_msg, sizeof temp_msg, "  Ucpu  %d mV ",conf.voltage );
        draw_text(204+XPOS,YPOS+10,temp_msg,CL_GRAY ,CL_BLACK); 
    #endif
#if POWER_MODE_WEACT == 0
      //  draw_text(210+XPOS,YPOS+30,"MODE    PFM",CL_GRAY ,CL_BLACK); 
#endif 
#if POWER_MODE_WEACT == 1
     //   draw_text(210+XPOS,YPOS+30,"MODE    PWM",CL_GRAY ,CL_BLACK); 
#endif 

        #endif
        #endif

// информация из setup
draw_text(6+FONT_W,75+YPOS,menu_ram[conf.mashine],CL_GRAY,CL_BLACK);

#ifndef  GENERAL_SOUND     
draw_text(6+FONT_W,85+YPOS,menu_sound[conf.type_sound],CL_GRAY,CL_BLACK);    
#else 
#ifdef Z_CONTROLER 
draw_text(11+FONT_W,85+YPOS,"TurboSound + Z-Controller SD",CL_GRAY,CL_BLACK);    
#else
draw_text(11+FONT_W,85+YPOS,"GeneralSound + TurboSound",CL_GRAY,CL_BLACK);   
#endif 
#endif


// дата и время компиляции
//printf("%s ", BUILD_DATE);
draw_text(12+FONT_W,110+YPOS,BUILD_DATE,CL_BLUE,CL_BLACK); 


#ifdef MOS2
#ifndef WS_ZERO2
draw_text(70+FONT_W,110+YPOS,"Murmulator OS2 Edition",CL_BLUE,CL_BLACK); 
#else
draw_text(70+FONT_W,110+YPOS,"RP2350-PiZero MOS2",CL_BLUE,CL_BLACK); 
#endif
#endif

#ifndef MOS2
#ifdef WS_ZERO2
draw_text(70+FONT_W,110+YPOS,"RP2350-PiZero",CL_BLUE,CL_BLACK); 
#endif
#endif

#ifndef  GENERAL_SOUND   
#ifndef  MOS2

#ifdef  MURM2
draw_text(70+FONT_W,110+YPOS,"Murmulator v2.x ",CL_BLUE,CL_BLACK); 
#endif

#ifdef  MURM1
draw_text(70+FONT_W,110+YPOS,"Murmulator v1.x ",CL_BLUE,CL_BLACK); 
#endif

#ifdef  PI_CARD
draw_text(70+FONT_W,110+YPOS,"PiCard v1.x ",CL_BLUE,CL_BLACK); 
#endif

#endif
#endif

if (vout_select==VIDEO_VGA)
        {
        snprintf(temp_msg, sizeof temp_msg, "VGA %dHz",60);   
       // snprintf(temp_msg, sizeof temp_msg, "VGA %dHz",(int) (CPU_MHZ*10/63)); 
       draw_text(246+XPOS,YPOS+110,temp_msg,CL_BLUE ,CL_BLACK);  	
        }

        if (vout_select==VIDEO_HDMI)
        {
        snprintf(temp_msg, sizeof temp_msg, "HDMI %dHz",(int) (CPU_MHZ*10/(42*conf.hdmi_fdiv)));  
        draw_text(240+XPOS,YPOS+110,temp_msg,CL_BLUE ,CL_BLACK);
        }

        if (vout_select==VIDEO_TFT)
        {
        if (conf.tft== TFT_9345) snprintf(temp_msg, sizeof temp_msg,  "ILI9345 TFT");
        if (conf.tft== TFT_9345I) snprintf(temp_msg, sizeof temp_msg, "ILI9345 IPS");
        if (conf.tft== TFT_7789) snprintf(temp_msg, sizeof temp_msg,  "ST7789");
		draw_text(228+XPOS,YPOS+110,temp_msg,CL_BLUE ,CL_BLACK);	
        }   
           
//------------------------------------------------------------------
// есле подключается плата с GS PicoBUS
	#if defined  GENERAL_SOUND
    // Первичная инициализация picobus
     draw_text(12+FONT_W,100+YPOS, "Connect PicoBus ....",CL_LT_BLUE,CL_BLACK);
      sleep_ms(1000);
       init_picobus();

       flag_gs=1;
        sys_GS(0x00);
        tx_buffer[60]=0;
       draw_text_len(12+FONT_W,100+YPOS,tx_buffer,CL_GREEN,CL_BLACK,32); 
       draw_text(12+FONT_W,110+YPOS,tx_buffer+32,CL_LT_BLUE,CL_BLACK); 
       select_audio(); // переключение режимов вывода звука 
    #endif
//-----------------------------------------------------------------    
     y_info += 10;
//  tuh_task(); // tinyusb host task
  switch (usb_device) 
    {
              case 0:
               snprintf(temp_msg, sizeof temp_msg, "No USB device");
            //   temp_msg[0]=0;
              break;
              case 1:
              snprintf(temp_msg, sizeof temp_msg, "USB keyboard        ");
              break;
              case 2:
              snprintf(temp_msg, sizeof temp_msg, "USB mouse           ");
              break;
              case 3:
              snprintf(temp_msg, sizeof temp_msg, "USB keyboard + mouse");
              break;
    }
    draw_text(10+XPOS,y_info,temp_msg,CL_GREEN,CL_BLACK); 

    
    //    snprintf(temp_msg, sizeof temp_msg, "#%x ",kb_st_ps2.u[2]);
	 //  	draw_text(254+XPOS,YPOS+20,temp_msg,CL_LT_CYAN,CL_BLACK);

flag_usb_kb = false;

// kb_st_ps2.u[2]= 0xff;
    // ожидание клавиши перед запуском
 /*     while (!((kb_st_ps2.u[3] == 0) & (kb_st_ps2.u[2] == 0) & (kb_st_ps2.u[1] == 0) & (kb_st_ps2.u[0] == 0)))
    {
        decode_key();
    } */
 

	//while (! ((decode_PS2()) | (decode_key(is_menu_mode)) | (decode_joy()))){}
 //   sleep_ms(9000);

}
//=========================================================================
void Message_Print()
        {
           wait_msg--;
           
        switch (msg_bar)
        {
            #define X_INFO 320-48
            #define Y_INFO 240-16
    //   case 0:
    //        draw_text((320-(16*FONT_W))/2,Y_INFO,"2026  ZX SPECCY P",CL_LT_CYAN ,CL_BLACK);//232   
    //        break;

        case 1:    
       
             if (conf.type_sound<I2S_AY) //soft ay
             {
            sprintf(temp_msg, "VOL %d ", conf.vol_ay);//%%
             draw_text(X_INFO,Y_INFO,temp_msg,CL_GREEN ,CL_BLACK);//232
             break;
             }
             else  // i2s ay
             {
             sprintf(temp_msg, "VOL %d ", conf.vol_i2s);//%%
               draw_text(X_INFO,Y_INFO,temp_msg, CL_GREEN ,CL_BLACK);//232
             break;
             }

        case 2:    
        if (conf.type_sound<I2S_AY) //soft ay
        {
        sprintf(temp_msg, "VOL %d ", conf.vol_ay);//%%
        draw_text(X_INFO,Y_INFO,temp_msg,CL_GREEN ,CL_BLACK);//232
        break;
        }
        else  // i2s ay
        {
        sprintf(temp_msg, "VOL %d ", conf.vol_i2s);//%%
            draw_text(X_INFO,Y_INFO,temp_msg, CL_GREEN ,CL_BLACK);//232
        break;
        }
        case 3:
            sprintf(temp_msg, "NORMAL ");
            draw_text(X_INFO,Y_INFO,temp_msg,CL_GREEN ,CL_BLACK);//232
/*             sprintf(temp_msg, "Z80 %d MHz",conf.z80_freq_hz);
            draw_text(5,X_INFO,temp_msg,CL_YELLOW,CL_BLACK);//232 */
            break;
        case 4:
            sprintf(temp_msg, "TURBO  ");
            draw_text(X_INFO,Y_INFO,temp_msg,CL_GREEN ,CL_BLACK);//232
/*             sprintf(temp_msg, "Z80 %d MHz",conf.z80_freq_hz);
            draw_text(5,X_INFO,temp_msg,CL_YELLOW,CL_BLACK);//232    */    
            break;
        case 5:
            sprintf(temp_msg, "FAST  ");
            draw_text(X_INFO,Y_INFO,temp_msg,CL_GREEN ,CL_BLACK);//232
/*             sprintf(temp_msg, "Z80 %d MHz",conf.z80_freq_hz);
            draw_text(5,X_INFO,temp_msg,CL_YELLOW,CL_BLACK);//232        */
            break;            

 /*        case 6:
            sprintf(temp_msg, " CONNECTED USB  %4X:%4X         ", vid,pid);//%%
            draw_text(0,X_INFO,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;    */

/*         case 7:
            sprintf(temp_msg, " %4X KEMPSTON", joy_k);//%%
            draw_text(0,X_INFO,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;   
       */

        case 8:
            sprintf(temp_msg, " GAMEPAD XBOX  %4X:%4X                  ", vid,pid);//%%
            draw_text(0,Y_INFO,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;   

      
/*         case 9:
         //  sprintf(temp_msg, "  ROM:  %4X    %4X       ", rom_n,rom_n1 );//%%zx_7ffd_lastOut
         sprintf(temp_msg, "7ffd:%2X 1ffd:%2X RAM:%2X ROM:%2X     ", 
         zx_machine_get_7ffd_lastOut(),zx_machine_get_1ffd_lastOut(), zx_machine_get_zx_RAM_bank_active(), zx_machine_get_rom() );//%%
            draw_text(0,X_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;   
 */
        case 10:
           sprintf(temp_msg, " Read Only! ");
            draw_text(0,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;  
      /*   case 11:
           sprintf(temp_msg, " TRD the file is non-standard Read Only ");
            draw_text(0,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;  
        case 12:
           sprintf(temp_msg, " This file is Read Only ");
            draw_text(0,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;  */ 


    /*     case 14:
          // sprintf(temp_msg, dir_patch_info);
           sprintf(temp_msg, " File ATTR:  %4X         ", file_attr[0]  );//%%zx_7ffd_lastOut
            draw_text(0,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;  
 */
/*  case 15:
          sprintf(temp_msg, dir_patch_info);
          sprintf(temp_msg, "WRITE TR:%2X Sc:%2X       ",WD1793.RealTrack ,WD1793.RealSector);//%%zx_7ffd_lastOut
            draw_text(8,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;   */
/*  case 16:
          // sprintf(temp_msg, dir_patch_info);
       //    sprintf(temp_msg, "READ TR:%2X Sc:%2X       ",WD1793.RealTrack ,WD1793.RealSector);//%%zx_7ffd_lastOut
       //     draw_text(8,Y_INFO,temp_msg,CL_WHITE ,CL_BLUE);//232
            break;   */
 case 18:
           sprintf(temp_msg, dir_patch_info);
           sprintf(temp_msg, "ERR TR:%2X Sc:%2X       ",WD1793.RealTrack ,WD1793.RealSector);//%%zx_7ffd_lastOut
           draw_text(8,Y_INFO,temp_msg,CL_WHITE ,CL_RED);//232
            break;  
  case 19:    
        sprintf(temp_msg, "S:%d ", conf.shift_img);//%%
        draw_text(X_INFO,Y_INFO,temp_msg,CL_LT_BLUE ,CL_BLACK);
        break;


  case 17:
            draw_text(8,Y_INFO,menu_pallete[conf.pallete],CL_WHITE ,CL_BLUE);//232
             break;

 case 77:
           sprintf(temp_msg, dir_patch_info);

           sprintf(temp_msg, "C:%2X H:%2X  R:%2X   " ,WD1793.RealTrack ,WD1793.Side, WD1793.RealSector);//%%zx_7ffd_lastOut
           draw_text(8,Y_INFO-10,temp_msg,CL_WHITE ,CL_BLUE);//232
           sprintf(temp_msg, "C:%2X H:%2X  R:%2X   N:%2X  SIZE:%d " ,sector_inf->c ,sector_inf->h, sector_inf->r,sector_inf->n ,sector_inf->size );//
           draw_text(8,Y_INFO,temp_msg,CL_WHITE ,CL_BLUE);//232
           wait_msg = 1000;
            break;  

        default:
            wait_msg = 0;
            break;
        }
        }
//=========================================================================
// MAIN
int fast(main)(void){  

    init_pico();
    sleep_ms(100);

 // Инициализация USB
   init_usb_hid(); // USB HID
    for(int i = 0; i < 500; i++)// время на определение USB устройств
{
     tuh_task(); // tinyusb host task
    //  tuh_task_ext(0, false);
     g_delay_ms(1);
  } 
// tuh_task(); // tinyusb host task
// Инициализация последовательного порта
  //  stdio_init_all(); // для автоматической загрузки прошивки RP2350

    init_and_info();

//-----------------------------------------------------------------    
// если одна плата без GS 
    #ifndef  GENERAL_SOUND     
    select_audio(); // переключение режимов вывода звука 
 	int hz = 96000;	//44000 //44100 //96000 //22050
	repeating_timer_t timer_audio;
	// negative timeout means exact delay (rather than delay between callbacks)
 	if (!add_repeating_timer_us(AY_SAMPLE_RATE, AY_timer_callback, NULL, &timer_audio)) return 1;// -10  частота ноты До 237Гц  нужно 240,0058 Гц
    #endif

	repeating_timer_t zx_flash_timer;
	//hz=2;
	if (!add_repeating_timer_us(-1000000 / 2/*Hz*/, zx_flash_callback, NULL, &zx_flash_timer)) {
	//	G_PRINTF_ERROR("Failed to add zx flash timer\n");
 //   gpio_put(25,1);//error
		return 1;
	}
//---------------------------------------------------------
// INT generator 50Hz
/*  	repeating_timer_t int_timer; 
     // hz=50;
	if (!add_repeating_timer_us(-1000000 / 50, zx_int, NULL, &int_timer))
     {
		//G_PRINTF_ERROR("Failed to add INT timer\n");
    //    gpio_put(25,1);//error
		return 1;
	}   
    */
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


 // is_new_screen = true;

    
	multicore_launch_core1(ZXThread);

    disk_autorun ();
 //   wait_msg = 5000;// сообщения внизу и громкость время вывода
 //   msg_bar = 0;
//######################
//   основной цикл
//######################


    while (1)
    {
        if (wait_msg !=0) Message_Print();
//---------------------------------------------
// опрос джоя
       if (conf.joyMode == 0)
        {
            if ((inx++ %5) == 0)
            {
                if (joy_connected) zx_input.kempston = (uint8_t)(data_joy);
                else  zx_input.kempston = 0;
            };
        }
        
  // ОПРОС КЛАВИАТУРЫ И ДЖОЙСТИКА 
 if ((decode_PS2()) | (decode_key(is_menu_mode)) | (decode_joy()) )
    {   
            //------------------------------------------------------
            // кнопка перехода в меню файлов
             key_menu_state =( (MENU) | (joy_key_ext  == 0x84)  );
   
            // кнопка выхода из меню файлов по [start] joy
             if ( (joy_key_ext  == 0x80) & (is_menu_mode))  key_menu_state = true;// exit [start] joy

            if (key_menu_state & !old_key_menu_state) // выход из файлового меню
            {
                data_joy =0;
                is_menu_mode ^= 1;
                is_new_screen = true;
                zx_machine_enable_vbuf(false);
                im_z80_stop = false;
                hardAY_on();
            }

            else // останов эмуляции и вход в файловое меню
            {
                is_new_screen = false;
                im_z80_stop = true;
            }
               old_key_menu_state = key_menu_state;

//########################################################################################           
                // меню если is_menu_mode =true файловое меню
                if ((is_menu_mode) && (!trdos))   { file_manager(); TAP_RestorePage(); }// файловое меню
//########################################################################################
    if ((!is_menu_mode)) // что нажимается вне файлового менеджера
    {
            if (((kb_st_ps2.u[3] & KB_U3_F2) | (joy_key_ext== 0x82)) )  { save_slot();  TAP_RestorePage(); }  // [START]+стрелка влево - вход в меню SAVE
            if (((kb_st_ps2.u[3] & KB_U3_F3) | (joy_key_ext == 0x81)) )  { load_slot();  TAP_RestorePage(); }  // [START]+стрелка вправо - вход в меню LOAD
            if (kb_st_ps2.u[3] & KB_U3_F5)   save_all(); // запись всей памяти и файла конфигурации
            if (END) disasm(); // Дизассемблер
            if (F10) // TURBO
                { conf.turbo++;
                  turbo_switch();
                  msg_bar=3+conf.turbo;
                  wait_msg = 2000; 
                }
            // кнопка перехода в меню SETUP
           if (((MENU_SETUP) | (joy_key_ext  == 0x88)) )  setup_zx();  // START+ [B] SetUp


           //
           if (F1)  help_zx();
           if (F4)  help_keyboard();
           if (F9)  nmi_zx();
           if (PAUSE) pause_zx(); //  [Pause Break]

           if (F6) // палитра
           {
               if (vout_select != VIDEO_TFT)
               {
                   msg_bar = 17;
                   wait_msg = 1000;
                   conf.pallete++;
                   if (conf.pallete == 12)
                       conf.pallete = 0;
                   set_palette(conf.pallete);
               }
           }

            if (KEY_RESET_PICO) pico_reset();  


//### длителность кадра в тактах
           if (KEY_CTRL_F7) // -
           {
            msg_bar = 19;
            wait_msg = 1000;
            conf.shift_img = conf.shift_img - 1;
            kb_st_ps2.u[3]=0;
           }

           if (KEY_CTRL_F8) // +
           {
             msg_bar = 19;
             wait_msg = 1000;
             conf.shift_img = conf.shift_img + 1;
             kb_st_ps2.u[3]=0;
           }
//###
#ifndef  GENERAL_SOUND         
           if (conf.type_sound < HARD_AY) // остальное железные AY
           {
               if (F7) // громкость -
               {
                   msg_bar = 1;
                   wait_msg = 1000;

                   // Soft AY
                   if ((conf.type_sound == SOFT_AY) | (conf.type_sound == SOFT_TS))
                   {
                       if (conf.vol_ay == 0)
                           conf.vol_ay = 0;
                       else
                           conf.vol_ay--;
                       vol_ay = conf.vol_ay;
                       init_vol_ay();
                   }
                   // I2S AY
                   if ((conf.type_sound == I2S_AY) | (conf.type_sound == I2S_TS))
                   {
                       if (conf.vol_i2s == 0)
                           conf.vol_i2s = 0;
                       else
                           conf.vol_i2s--;
                       vol_ay = conf.vol_i2s;
                       init_vol_ay();
                   }
               }

               if (F8) // громкость +
               {
                   msg_bar = 2;
                   wait_msg = 1000;
                   // Soft AY
                   if ((conf.type_sound == SOFT_AY) | (conf.type_sound == SOFT_TS))
                   {
                       if (conf.vol_ay == MAX_VOL_PWM)
                           conf.vol_ay = MAX_VOL_PWM;
                       else
                           conf.vol_ay++;
                       vol_ay = conf.vol_ay;
                       init_vol_ay();
                   }
                   // I2S AY
                   if ((conf.type_sound == I2S_AY) | (conf.type_sound == I2S_TS))
                   {
                       if (conf.vol_i2s == 100)
                           conf.vol_i2s = 100;
                       else
                           conf.vol_i2s++;
                       vol_ay = conf.vol_i2s;
                       init_vol_ay();
                   }
               }
           }

#endif

#ifdef  GENERAL_SOUND         
      //   if ((z_controler_cs & 0x02) == 0x00) // Z-Controller не активен
     /// if (gpio_get(PBUS_CS)) // Z-Controller не активен
           {
               if (F7) // громкость -
               {
                   msg_bar = 1;
                   wait_msg = 1000;

                       if (conf.vol_i2s == 0)
                           conf.vol_i2s = 0;
                       else
                           conf.vol_i2s--;
                       vol_ay = conf.vol_i2s;
                       init_vol_ay();
               }

               if (F8) // громкость +
               {
                   //  kb_st_ps2.u[3] = 0; // key F8
                   msg_bar = 2;
                   wait_msg = 1000;

                       if (conf.vol_i2s == 100)
                           conf.vol_i2s = 100;
                       else
                           conf.vol_i2s++;
                       vol_ay = conf.vol_i2s;

                        im_z80_stop = true;
                       init_vol_ay();
                   
               }
           }

#endif
           /*--Emulator reset--*/
           if (KEY_RESET_ZX)
           {

               im_z80_stop = true;
               is_menu_mode = true;
               is_new_screen = true;

               hardAY_off();
               MessageBox(" ZX SPECTRUM RESET ", "", CL_WHITE, CL_RED, 2);

               zx_machine_reset(3);
               im_z80_stop = false;
               is_menu_mode = false;
            //   is_new_screen = false;
           }
            }
//######################################################
// Работа эмулятора
//######################################################
          if (!is_menu_mode)
            { // Emulation mode
                zx_machine_enable_vbuf(true);
                if (im_z80_stop)
                {
                    im_z80_stop = false;
                }
                // zx_machine_set_vbuf(g_gbuf);
                if (need_reset_after_menu)
                {
                    zx_machine_reset(3);
                }

                convert_kb_u_to_kb_zx(&kb_st_ps2, zx_input.kb_data);

                joy_scan();// переопределление kempston joy на клавиши 

            } // Emulation mode end


        } 

           zx_machine_input_set(&zx_input);
      
           led_trdos();// мигаие led 


      } // while(1)
    pico_reset(); // аварийный сброс ;)
}
//==========================================================================
void file_select_trdos(void) // 
{
	is_menu_mode = true;
	
is_new_screen = true;
	//     MenuTRDOS(); // меню выбора и подключения образов trd
	uint8_t Drive = MenuBox_trd(64, 54, 22, 7, "Drive TR-DOS", 4, 0, 1);
	if (Drive < 5)
	{
		// Копируем строку длиною не более 10 символов из массива src в массив dst1.
		// strncpy (dst1, src,3);
        strncpy(conf.DiskName[Drive], files[cur_file_index], LENF);

          if (Drive == 0) conf.FileAutorunType=TRD; // к диску а подключен TRD образ
         // conf.FileAutorunType=TRD;
         file_type[Drive] = TRD;
          OpenTRDFile(conf.activefilename,Drive);

          write_protected = false; // защита записи отключена для TRD
	}

	draw_main_window(); // восстановление текста
	draw_file_window();

    g_delay_ms(200);
	last_action = time_us_32();


}
////// TRDOS end
//++++++++++++++++++++++++++++++++++++++++++
//================================================================
void  config_init(void)
{
	FIL f;
	//int fr =-1;

    enable_tape = false; // tap файл не подключен при запуске

    sprintf(temp_msg, "0:/speccy_p.cnf");
    int fd = f_open(&f, temp_msg, FA_READ);
    if (fd != FR_OK)
    {
        f_close(&f);
        config_defain();// если нет файла конфигурации       
        config_ini_load("0:/speccy_p.ini");//текстовый файл конфига
        // если его нет то загружается дефолтная конфигурация и файл записывается
        return;
    }
     UINT bytesRead;
      fd =  f_read(&f, &conf ,sizeof(conf),&bytesRead);//f_read(&f, *conf ,sizeof(conf) , &bytesRead);
    if (fd != FR_OK)  // если ошибка то конфиг по умолчанию
    {
      f_close(&f);
      config_defain(); 
      return ;
   
    }  
    if (conf.version!=CONFIG_VERSION) 
    {
        f_close(&f);
        config_defain();// если файл конфигурации неправильной версии
        config_ini_load("0:/speccy_p.ini");//текстовый файл конфига
        // если его нет то загружается дефолтная конфигурация и файл записывается
        return;
    }
 

    f_close(&f);

    config_ini_load("0:/speccy_p.ini");//текстовый файл конфига
    conf.turbo=0; // при включении TURBO OFF!
    return ;
}
//----------------------------------------------------
/*
#define PENT128  0
#define PENT512  1
#define PENT1024 2
#define SCORP256 3
#define PROFI1024 4
#define GMX2048  5
#define ZX4096 6 */

bool save_config(void)
{   
    
	FIL f;

/*     int fd = f_mkdir("0:/");

   if ((fd != FR_OK) && (fd != FR_EXIST))
    {
        MessageBox("      Error saving config      ", "", CL_LT_YELLOW, CL_RED, 1);
        return false;
    }   */

    MessageBox("       Saving config      ", "", CL_WHITE, CL_BLUE, 2);

    sprintf(temp_msg, "0:/speccy_p.cnf");//

   int fd = f_open(&f, temp_msg, FA_CREATE_ALWAYS | FA_WRITE);
    if (fd != FR_OK)
    {
         MessageBox("      Error saving config      ", "", CL_LT_YELLOW, CL_RED, 1);
        f_close(&f);
        return false;
    }
    UINT bytesWritten;
    fd = f_write(&f, &conf, sizeof(conf), &bytesWritten);
    if (bytesWritten != sizeof(conf))
    {
        f_close(&f);
        return false;
    }
    f_close(&f);

    config_ini_save("0:/speccy_p.ini");//текстовый файл конфига


    return true;
}
//=========================================================
void pause_zx(void) //  [Pause Break]
{
    im_z80_stop = true;
    is_menu_mode = true;
    hardAY_on_off = 0;
    hardAY_off(); // off hard AY   help

    draw_text(7, 228, " PAUSE PRESS [ESC] TO EXIT ", CL_WHITE, CL_BLUE);
   
  while (1)
  {
   // if (mode_kbms)  sleep_ms(DELAY_KEY); // задержка если это не ps/2
    if (!decode_key_joy()) continue;


    if (kb_st_ps2.u[1] & KB_U1_ESC) // ESC
    {
       wait_esc();

            im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on();
       return ;
    }
}
}
//=========================================================
//=========================================================
void help_zx(void)// F1
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_on_off=0;
    hardAY_off();// off hard AY   help 

	draw_rect(0, 20, 318, 200, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(0, 20, 318, 200, CL_GRAY, false);				   // рамка 1
	draw_rect(0 + 2, 20 + 2, 318 - 4, 200 - 4, CL_GRAY, false); // рамка 2
	draw_rect(0 + 3, 20 + 3, 318 - 6, 8, CL_GRAY, true);			 // шапка меню
	draw_text(0 + 10, 20 + 3, "ZX SpeccyP  Help", CL_BLACK, CL_GRAY); // шапка меню

	// меню помощи фактически ожидание ESC
     MenuBox_help(7, 24, 16, 17, menu_help, 17, 0, 1);
            im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on();// exit help
}
//=========================================================

extern uint16_t dis_adres;
void disasm(void) // [END] KEY
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_on_off=0;
    hardAY_off();// off hard AY help keyboard
    disassembler();
  //  address_pc =Z80_PC(z1->cpu);
     dis_adres = Z80_PC(z1->cpu);       // PC 16-битное значение
    bool dis_dump= true;
      if(dis_dump) list_disassm( );
        else  list_dump();
while (1)
  {

    if (!decode_key_joy()) continue;

    if (dis_dump)
    {
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      
       dis_adres+= OpcodeLen(dis_adres);
       list_disassm();
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
       dis_adres--;
      list_disassm();   
    }

    if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
      kb_st_ps2.u[2] = 0;
       dis_adres+=20;
      list_disassm();   
    }


    if (kb_st_ps2.u[2] & KB_U2_LEFT)

    {
      kb_st_ps2.u[2] = 0;
       dis_adres-=20;
      list_disassm();   
    }




    }
    else
    {
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      
       dis_adres+= 8;
       
        list_dump();
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
       dis_adres-=8;
      
        list_dump();
    }
if (kb_st_ps2.u[2] & KB_U2_RIGHT)
    {
      kb_st_ps2.u[2] = 0;
      
       dis_adres+= 8*20;
       
        list_dump();
    }

    if (kb_st_ps2.u[2] & KB_U2_LEFT)

    {
      kb_st_ps2.u[2] = 0;
       dis_adres-=8*20;
      
        list_dump();
    }




    }
    if (kb_st_ps2.u[1] & KB_U1_ENTER) // enter
    {
        wait_enter();
        draw_rect(248, 20, 50, 190, CL_BLACK, true);// clear
       dis_dump= !dis_dump;
        if(dis_dump) list_disassm( );
        else  list_dump();
    }

    if (kb_st_ps2.u[1] & KB_U1_ESC)
    {
     wait_esc();
    im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on(); // ON hard AY disassm
      return ; // ESC exit
    }
  }


     MenuBox_help(7, 24, 16, 1,menu_keyboard, 1, 0, 1);
 
            im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on(); // ON hard AY disassm
}
//===========================================
void help_keyboard(void) // F4
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_on_off=0;
    hardAY_off();// off hard AY help keyboard


	draw_rect(0, 10, 318, 200, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(0, 10, 318, 200, CL_GRAY, false);				   // рамка 1
	draw_rect(0 + 2, 10 + 2, 318 - 4, 200 - 4, CL_GRAY, false); // рамка 2

	draw_rect(0 + 3, 10 + 3, 318 - 6, 8, CL_GRAY, true);			 // шапка меню
	draw_text(0 + 10, 10 + 3, "ZX Keyboard  [ESC] Exit", CL_BLACK, CL_GRAY); // шапка меню


	// меню выбора setup

 //  void draw_keyboard(uint8_t xPos,uint8_t yPos){ //x:33 y:84
	for(uint8_t y=0;y<156;y++){
		for(uint8_t x=0;x<254;x++){
			uint8_t pixel = kbd_img[x+(y*254)];
			if (pixel<0xFF)	draw_pixel(35+x,35+y,pixel);
			//printf("X:%d Y:%d C:[%02X]\n",x,y,pixel);
		}
	}
     MenuBox_help(7, 24, 16, 1,menu_keyboard, 1, 0, 1);
 
            im_z80_stop = false;
            is_menu_mode = false;
        //    is_new_screen = false;
            hardAY_on(); // ON hard AY help keyboard

}
//=========================================================
void setup_zx(void)
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_on_off=0;
    hardAY_off();// off hard AY SETUP

#define w1 290
#define h1 180
#define x1 18
#define y1 20

	draw_rect(x1, y1, w1, h1, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(x1, y1, w1, h1, CL_GRAY, false);				   // рамка 1
	draw_rect(x1 + 2, y1 + 2, w1 - 4, h1 - 4, CL_GRAY, false); // рамка 2

	draw_rect(x1 + 3, y1 + 3, w1 - 6, 8, CL_GRAY, true);	   // шапка меню
    draw_text(x1 + 10, y1 + 3, FW_VERSION, CL_BLACK, CL_GRAY); // шапка меню
    
         if (vout_select==VIDEO_VGA)
		draw_text(x1 + 180, y1 + 3,"VGA",CL_BLACK, CL_GRAY);	
        if (vout_select==VIDEO_HDMI)
		draw_text(x1 + 180, y1 + 3,"HDMI",CL_BLACK, CL_GRAY);	
        if (vout_select==VIDEO_TFT)
		draw_text(x1 + 180, y1 + 3,"TFT",CL_BLACK, CL_GRAY);	  

	char str[10];
	snprintf(str, sizeof str, "%dMHz", (int)(clock_get_hz(clk_sys) / 1000000));
	draw_text(x1 + 240, y1 + 3, str, CL_BLACK, CL_GRAY); // CPU

	if (psram_avaiable)
    {
         snprintf(temp_msg, sizeof temp_msg, "%dMb", size_psram);
		draw_text(x1 + 206+12, y1 + 3, temp_msg, CL_BLACK, CL_GRAY);
    }
    else
    #ifdef RP2350_256K
	{
		if (conf.mashine==PENT128||conf.mashine==SPEC48||conf.mashine==SCORP256||conf.mashine==NOVA256) conf.mashine = conf.mashine;
        else conf.mashine = SCORP256;
        
	}
    #else
	{
		if (conf.mashine > 1) conf.mashine = 0;
	}
    #endif


	// меню выбора setup
    while (1)
    {  
        draw_rect(30, 40, 240,155, CL_BLACK, true);				   // рамка 3 фон
        draw_text(x1 + 120, y1 +20             , menu_ram[conf.mashine], CL_GRAY, CL_BLACK);

      #ifdef GENERAL_SOUND
        draw_text(x1 + 126, y1 + 20+ M_SOUND*10, "GeneralSound + TS", CL_GRAY, CL_BLACK);
      #else  
        draw_text(x1 + 120, y1 + 20+ M_SOUND*10, menu_sound[conf.type_sound], CL_GRAY, CL_BLACK);
      #endif

        draw_text(x1 + 120, y1 + 20+ M_TURBO*10,  menu_speed[conf.turbo], CL_GRAY, CL_BLACK);
        draw_text(x1 + 120, y1 + 20+ M_JOY*10, menu_joy[conf.joyMode], CL_GRAY, CL_BLACK);

       /// if (conf.vout!=VIDEO_TFT) 
        if (vout_select != VIDEO_TFT)
        draw_text(x1 + 120, y1 + 20+ M_PALLETE*10, menu_pallete[conf.pallete], CL_GRAY, CL_BLACK);

        draw_text(x1 + 120, y1 + 20+M_AUTORUN*10, menu_autorun[conf.autorun], CL_GRAY, CL_BLACK); 

       
     static  uint8_t numsetup = 14;
     if (vout_select == VIDEO_TFT)
     //if (conf.vout==VIDEO_TFT ) 
     numsetup = MenuBox_bw(30, 20, 18, 15, menu_setup_tft,15, numsetup, 1);// TFT
                else 
                       numsetup = MenuBox_bw(30, 20, 18, 15, menu_setup,15, numsetup, 1);

//---
        if (psram_avaiable)
        {
            if (numsetup == M_RAM)
            {
                uint8_t x = MenuBox(90, 52, 17, 9, "Model & RAM", menu_ram, 9, conf.mashine, 1);
               if (x==0xff) continue;
               #ifdef NO_GMX
               if (x==0x05) continue;
               #endif
                conf.mashine  = x;
                init_mashine_and_extram(conf.mashine);
                continue;
            }
        }

        #ifdef RP2350_256K
        else // только 256 128 48 машины
        {
            if (numsetup == M_RAM)
            {
            uint8_t x = MenuBox(90, 52, 17, 4, "RAM Seting", menu_ram_128_48, 4, 0, 1);
            if (x==0xff) continue;
            switch (x)
            {
            case 0: conf.mashine = PENT128;  break;
            case 1: conf.mashine = SPEC48 ;  break;
            case 2: conf.mashine = SCORP256; break;    
            case 3: conf.mashine = NOVA256;  break;
            }
            init_mashine_and_extram(conf.mashine);
            continue;
            }
        }
        #else // только 128 и 48 машина
        {
            if (numsetup == M_RAM)
            {
            uint8_t x = MenuBox(90, 52, 17, 2, "RAM Seting", menu_ram_128_48, 2, conf.mashine, 1);
            if (x==0xff) continue;
            conf.mashine  = x;
            init_mashine_and_extram(conf.mashine);
            continue;
            }
        }
        #endif

        #ifndef GENERAL_SOUND
        if (numsetup == M_SOUND)
        {
            uint8_t x = MenuBox(90, 52, 16, 8, "Sound Seting", menu_sound, 8, conf.type_sound, 1);

           if (x==0xff) continue;
           if (conf.type_sound  == x) continue;// то же что и было
           uint8_t y = conf.type_sound;
            conf.type_sound  = x;

          //  select_audio(conf.type_sound); // переключение режимов вывода звука
            // сохранение и перезагрузка
             save_config();
         //   if (conf.type_sound == y) continue;
             MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);
             pico_reset();
            continue;
        }
        #endif
if (numsetup == M_JOY) 
        {
           uint8_t x = MenuBox(74, 52, 18, 6, "Joystick",menu_joy, 6, conf.joyMode, 1);
           if (x==0xff) continue;
           conf.joyMode = x;
           joy_redirecting();
           continue;
        }

  if (numsetup == M_SOUND_SETUP) // Sound setup
        {
          uint8_t x = MenuBox_sound_setup(94, 44, 17, 7, "Sound setup", 7, 6, 1);
           if (x==0xff) continue;

           continue;
        }    



  if (numsetup == M_ADVANCED)
        {
          uint8_t x = MenuBox_advanced_setup(94, 44, 17, 8, "Advanced setup", 8, 7, 1);
           if (x==0xff) continue;

           continue;
        }    


        if (numsetup == M_TURBO) // переключение NORMAL/TURBO
        {
            uint8_t x = MenuBox(94, 42, 17, 2, "Speed Mode", menu_speed, 2, conf.turbo, 1);
            conf.turbo = x;
            turbo_switch();
            continue;
        }

 if (numsetup == M_AUTORUN) // 
        {
           uint8_t x = MenuBox(94, 92, 17, 3, "Auto Run",menu_autorun, 3, conf.autorun , 1);
           if (x==0xff) continue;
           conf.autorun  = x;
           save_config();
           continue;
        }

        if (numsetup == M_SAVE_CONFIG) //  save config
        {
           save_config();
           continue;
        }

        if (numsetup ==M_SOFT_RESET) // Soft reset
        {
            MessageBox(" ZX SPECTRUM RESET ", "", CL_WHITE, CL_RED, 2);
            zx_machine_reset(3);
            im_z80_stop = false;
            is_menu_mode = false;
        //    is_new_screen = false;
            return;
 
        }
        if (numsetup == M_HARD_RESET) // Hard reset
        {
            MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);
            pico_reset();
            im_z80_stop = false;
            is_menu_mode = false;
         //   is_new_screen = false;
            return;
 
        }
             //--------------
        if (numsetup == M_POWER_OFF) // power off
        {
              save_all(); // запись всей памяти и файла конфигурации
              hardAY_on_off=0;
              hardAY_off();// off hard AY POWER OFF
              im_z80_stop = true;
	          is_menu_mode = true;

  MessageBox_off("     The current configuration saved   ", "     the computer can be powered off ", CL_WHITE, CL_BLUE, 0);

 while (1)
  {
      draw_img(0,0);        
  }

        }

//--------------
#ifndef MOS2
        if (numsetup == M_UPDATE) // update mode
        {
            im_z80_stop = true;
            is_menu_mode = true;

            hardAY_on_off=0;
           hardAY_off(); // off hard AY UPDATE

           draw_img(0,0);
           MessageBox("  switching to firmware update mode ","", CL_WHITE, CL_RED, 3);
           sleep_ms(256);
           reset_usb_boot(0, 0);
        }
#else
        if (numsetup == M_UPDATE) // Run Murmulator OS 
        {
            im_z80_stop = true;
            is_menu_mode = true;

            hardAY_on_off=0;
           hardAY_off(); // off hard AY UPDATE

           draw_img(0,0);
           MessageBox("          Run Murmulator OS         ","", CL_WHITE, CL_RED, 0);
           sleep_ms(256);

           FIL f;
           sprintf(temp_msg, "0:/.firmware");
           int fd = f_open(&f, temp_msg, FA_READ);
           if (fd != FR_OK)
           {
           f_close(&f);
           draw_img(0,0);
           MessageBox("                 RESET              ","", CL_WHITE, CL_RED, 0);
           sleep_ms(256);
           pico_reset(); // нет файла .firmware загрузка не под Murmulator OS
           return;
           }

        //   draw_img(0,0);
        //   MessageBox("          Run Murmulator OS         ","", CL_WHITE, CL_RED, 0);
          // удаление файла "0:/.firmware"
           fd = f_unlink(temp_msg);

           sleep_ms(256);
           f_close(&f);
           pico_reset();
           //reset_usb_boot(0, 0);
        }
#endif        
//--------------
        if (numsetup == M_EXIT) // Exit
        {
            im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on(); // EXIT SETUP
            return;

        }
        if (numsetup == 0xff) // exit
        {
            numsetup = 13;
            im_z80_stop = false;
            is_menu_mode = false;
            is_new_screen = false;
            hardAY_on();// EXIT SETUP
            return;
        }

// Только для режима TFT 10-й пункт меню
//if (conf.vout==VIDEO_TFT)
if (vout_select == VIDEO_TFT)
{
if (numsetup == M_TFT_BRIGHT)
        {
        while (1)
        {
        static uint8_t x=0;
        x= MenuBox_tft_setup(90, 44, 22, 11, "Setting TFT", submenu_setup_tft,7, x , 1);
 
        if (x==0xff) {x=0; goto L_EXIT;}

            switch (x)
            {
            case 0:
            conf.tft=TFT_9345;// ili9341
                break;
            case 1:
            conf.tft=TFT_9345I;// ili9341 ips
                break;              
            case 2:
            conf.tft=TFT_7789;// st7789
                break;
            case 3://переворот TFT 0 - 0x20 180 -0xE0
            if (conf.tft_rotate  == 0) conf.tft_rotate  = 1;
            else conf.tft_rotate  = 0;
                break;
            case 4://инверсия TFT // TFT_INV=0x20 #0x21
            if (conf.tft_invert == 0) conf.tft_invert  = 1;
            else conf.tft_invert = 0;
                break;
            case 5://RGB=0x00  BGR=0x08
            if (conf.tft_rgb == 0) conf.tft_rgb  = 1;
            else conf.tft_rgb = 0;
                break;
            case 6://Save config + reset
            save_config(); //сохраняем  конфигурацию
            pico_reset();
                break;

            default:
                break;
            }

        }

        L_EXIT:
     }


       

    }
//---
else   // Только для режима VGA/HDMI 10-й пункт меню
{
 if (numsetup == M_PALLETE)
 {
           for (int i = 0; i < 8; i++)
         {       
             draw_rect(i*40,230-24,40,12,i,true);//спектр
         }
           for (int i = 8; i < 16; i++)
         {       
             draw_rect((i-8)*40,230-12,40,12,i,true);//спектр
         }
            
            uint8_t x= MenuBox(90, 44, 16, 12, "Pallete", menu_pallete,12, conf.pallete , 1);
 
            if (x==0xff) continue;
            conf.pallete  = x;
 
            set_palette(conf.pallete); // переключение палитр
             continue;
         }
        }
 //---
    }
}
//==================================================================================================================================================
void slot_screen(uint8_t cPos)
{    
    ScreenShot_Y=18;// координата Y для вывода скриншота
    sprintf(conf.activefilename, "0:/save/%d_slot.Z80", cPos);
    if (!LoadScreenFromZ80Snapshot(conf.activefilename))
    {
        draw_rect(95, 18, 217, 210, CL_BLACK, true);
      //  draw_rect(17 + 8 * 14, 208, 182, 22, COLOR_BACKGOUND, true); // Фон отображения скринов
        draw_text(170, 77, "EMPTY SLOT", CL_GRAY, CL_BLACK);
    }
    ScreenShot_Y=40;// координата Y для вывода скриншота
}
//===================================================================================================================================================
uint8_t MenuBox_sv(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text,  uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * 8) + 10;
  uint16_t hFrame = ((1 + hPos) * 8) + 20;
  // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
  draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_BLACK, false); // рамка 1
  draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 3, CL_GRAY, false);  // рамка 2
  draw_rect(xPos, yPos, lFrame, hFrame, CL_BLACK, true);                  // рамка 3 фон
  draw_rect(xPos, yPos, lFrame, 9, CL_GRAY, true);                        // шапка меню
  draw_text(xPos + 10, yPos + 0, text, CL_PAPER, CL_INK);                 // шапка меню


  draw_line(100-6, 15, 100-6, 236, CL_INK);// рамка картинки
 // draw_rect(100-5, 15, 217, 220, CL_BLACK, true);
  yPos = yPos + 10;
  for (uint8_t i = 0; i < Pos; i++)
  {
    if (i == cPos) // курсор
    {
        sprintf(temp_msg, " %d.Slot ", i);
        draw_text(xPos+1, yPos + 8 + 8 * i, temp_msg, CL_PAPER, CL_LT_CYAN);

    } // курсор
    else
    {
        sprintf(temp_msg, " %d.Slot ", i);
        draw_text(xPos+1, yPos + 8 + 8 * i, temp_msg, CL_INK, CL_PAPER);
    }
  }
  kb_st_ps2.u[0] = 0x0;
  kb_st_ps2.u[1] = 0x0;
  kb_st_ps2.u[2] = 0x0;
  kb_st_ps2.u[3] = 0x0;
   slot_screen(cPos);
  while (1)
  {

    if (!decode_key_joy()) continue;
  //  decode_key();
   // sleep_ms(DELAY_KEY);
    if (kb_st_ps2.u[2] & KB_U2_DOWN)
    {
      kb_st_ps2.u[2] = 0;
      sprintf(temp_msg, " %d.Slot ", cPos);
      draw_text(xPos+1 , yPos + 8 + 8 * cPos, temp_msg, CL_INK, CL_BLACK); // стирание курсора
      cPos++;
      if (cPos == Pos)
        cPos = 0;
      sprintf(temp_msg, " %d.Slot ", cPos);  
      draw_text(xPos+1 , yPos + 8 + 8 * cPos, temp_msg, CL_BLACK, CL_LT_CYAN); // курсор
     //  sleep_ms(DELAY_KEY);
      slot_screen(cPos);
    }

    if (kb_st_ps2.u[2] & KB_U2_UP)

    {
      kb_st_ps2.u[2] = 0;
      sprintf(temp_msg, " %d.Slot ", cPos);
      draw_text(xPos+1 , yPos + 8 + 8 * cPos, temp_msg, CL_INK, CL_BLACK); // стирание курсора
      if (cPos == 0)
        cPos = Pos;
      cPos--;
      sprintf(temp_msg, " %d.Slot ", cPos);
      draw_text(xPos+1 , yPos + 8 + 8 * cPos, temp_msg, CL_BLACK, CL_LT_CYAN); // курсор
    //   sleep_ms(DELAY_KEY);
      slot_screen(cPos);
    }

    if (kb_st_ps2.u[1] & KB_U1_ENTER) // enter
    {
      sprintf(temp_msg, " %d.Slot ", cPos);  
       wait_enter();
      return cPos;
    }

    if (kb_st_ps2.u[1] & KB_U1_ESC)
    {
     wait_esc();
      return 0xff; // ESC exit
    }
  }
}
//=================================================================================
	void save_slot(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
        hardAY_on_off=0;
		hardAY_off();//save_slot
  //      uint8_t num = MenuBox_sv(38+16, 7, 31, 25, "SAVE",  25, 0, 1);
           uint8_t num = MenuBox_sv(38, 7, 33, 25, "SAVE",  25, 0, 1);
		if (num == 0xff) // exit
		{
			im_z80_stop = false;
			is_menu_mode = false;
            hardAY_on();// exit save slot
			return;
		}
    
		sprintf(save_file_name_image, "0:/save/%d_slot.Z80", num);
		sleep_ms(10);
		int fd = f_mkdir("0:/save");
		if ((fd != FR_OK) && (fd != FR_EXIST))
		{
			MessageBox(" Error saving ","", CL_LT_YELLOW, CL_RED, 2);
		}
		else
		{
            hardAY_on();// Quick save
			MessageBox(" Quick saving... ","", CL_WHITE, CL_BLUE, 0);
			save_image_z80(save_file_name_image);
            if (num==0) save_config(); //если нулевой слот то сохраняем ещё и всю конфигурацию


		}
		
		im_z80_stop = false;
		is_menu_mode = false;
		sleep_ms(3000);
	
		return;
	}
    //=================================================================================
	void save_all(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
        hardAY_on_off=0;
		hardAY_off();//save all
        
    
		sprintf(save_file_name_image, "0:/save/0_slot.Z80");
	//	sleep_ms(10);

 		int fd = f_mkdir("0:/save");

		if ((fd != FR_OK) && (fd != FR_EXIST))
		{
			MessageBox(" Error saving ","", CL_LT_YELLOW, CL_RED, 2);
		}  
		else
		{
            hardAY_on();// save all
			MessageBox(" SAVE ALL RAM & CONFIG ","", CL_WHITE, CL_BLUE, 0);
			save_image_z80(save_file_name_image);
         //   sleep_ms(1000);
            save_config(); //если нулевой слот то сохраняем ещё и всю конфигурацию
		}
		
		im_z80_stop = false;
		is_menu_mode = false;
       // hardAY_on();
	//	sleep_ms(1000);
	
		return;
	}
	//=================================================================================
	void load_slot(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
		im_ready_loading = false;
        hardAY_on_off=0;
        hardAY_off();// load slot

		uint8_t num = MenuBox_sv(38, 7, 33, 25, "LOAD",  25, 0, 1);

		if (num == 0xff) // exit
		{
			im_z80_stop = false;
			is_menu_mode = false;
            hardAY_on();//load slot
          
			return;
		}

		zx_machine_reset(3);
		sprintf(save_file_name_image, "0:/save/%d_slot.Z80", num);
		if (load_image_z80(save_file_name_image))
			MessageBox("Loading slot...", temp_msg, CL_WHITE, CL_BLUE, 2);
		else
			MessageBox(" Error loading ", "", CL_LT_YELLOW, CL_RED, 2);

            if (num==0) config_init(); //если нулевой слот то загружаем ещё и всю конфигурацию
		im_z80_stop = false;
		is_menu_mode = false;


      //  hardAY_on();
	}
//==============================================================================================
// AUTORUN
//==============================================================================================
void load_all(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
		im_ready_loading = false;
          AY_reset();

		sprintf(save_file_name_image, "0:/save/0_slot.Z80");
        sleep_ms(2000);
       load_image_z80(save_file_name_image);
	//	if (load_image_z80(save_file_name_image))

	//		MessageBox("Loading all RAM...", temp_msg, CL_WHITE, CL_BLUE, 2);
	//	else
	//		MessageBox(" Error loading ", "", CL_LT_YELLOW, CL_RED, 2);
//printf("LOADING\n" );
        //    if (num==0) config_init(); //если нулевой слот то загружаем ещё и всю конфигурацию
		im_z80_stop = false;
		is_menu_mode = false;
//im_ready_loading = true;
        zx_machine_enable_vbuf(true);

      //  hardAY_on();
	}
//===============================================================
    void load_trd(void)
    {

     //   hardAY_on();
//conf.FileAutorunType = 4;
        if (conf.FileAutorunType == SCL)
        {
          //  write_protected = true; // защита записи для SCL
            strcpy(conf.activefilename, conf.Disks[0]);// disk A   
               file_type[0] = SCL;           
            Run_file_scl(conf.activefilename, 0);
        }
        if (conf.FileAutorunType == TRD)
        {
             file_type[0] = TRD;// trd
               strcpy(conf.activefilename, conf.Disks[0]);// disk A
               OpenTRDFile(conf.activefilename, 0);        
          //  write_protected = false; // защита записи отключена для TRD
        }

        if (conf.FileAutorunType == FDI)
        {
                             file_type[0] = FDI; 
                             strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                             OpenFDI_File(conf.activefilename,0);
                             write_protected = true; // защита записи включена
        }

        im_z80_stop = false;
        is_menu_mode = false;
        zx_machine_enable_vbuf(true);
    }
//=========================================================
void disk_autorun(void)
{
	switch (conf.autorun)
    {
    case 0:
    break;         
    case 1:
        load_trd();
    break;
    case 2:
        load_all();
    break;  
  	    im_z80_stop = true;
		is_menu_mode = true;
		im_ready_loading = false;
        AY_reset();
    
	return;
    }
    
}   
//===============================================================
// индикатор работы trdos

void led_trdos(void)
{
    
   #if LED_BOARD != 255 
 // gpio_put(LED_BOARD, Requests  & 0b01000000);
   #endif

    if (!vbuf_en)
        return; // если экран эмуляции отключен то не шумим ffd
  //  if ((GetWD1793_Status()==1))

  if (Requests & 0b01000000)  
    {    
      //  static uint8_t x =0;
        uint8_t color_fon = zx_Border_color & 0x07; // дублируем для 4 битного видеобуфера
        draw_symbol(0, 240-16,0,CL_LT_BLUE, color_fon);
    }

#ifdef Z_CONTROLER
if ((z_controler_cs & 0x02) == 0)
    {    
        static uint8_t x =0;
         uint8_t color_fon = zx_Border_color & 0x07; // дублируем для 4 битного видеобуфера
        //if (x&0x80) 
         draw_symbol(0, 240-16,0,CL_LT_GREEN, color_fon);
    }
#endif
}
//===============================================================
// Работа с бутербродной PSRAM 
//===============================================================
volatile uint8_t * PSRAM_DATA = (uint8_t*)0x11000000;
#if defined(PICO_RP2350)
#include <hardware/structs/qmi.h>
#include <hardware/structs/xip.h>
//=================================================================
/**
 * @brief Проверка доступности PSRAM 
 * @return Размер PSRAM в Мегабайтах (8MB для APS6404)
 */
#define MB16 (16ul << 20)
#define MB8 (8ul << 20)
#define MB4 (4ul << 20)
#define MB1 (1ul << 20)


static int BUTTER_PSRAM_SIZE = 0;
 uint32_t __not_in_flash_func(get_psram_size)() {
   // if (BUTTER_PSRAM_SIZE != -1) return BUTTER_PSRAM_SIZE;
    for(register int i = MB8; i < MB16; i += 4096)
        PSRAM_DATA[i] = 16;
    for(register int i = MB4; i < MB8; i += 4096)
        PSRAM_DATA[i] = 8;
    for(register int i = MB1; i < MB4; i += 4096)
        PSRAM_DATA[i] = 4;
    for(register int i = 0; i < MB1; i += 4096)
        PSRAM_DATA[i] = 1;
    register uint32_t res = PSRAM_DATA[MB16 - 4096];
    for (register int i = MB16 - MB1; i < MB16; i += 4096) {
        if (res != PSRAM_DATA[i])
            return 0;
    }
    BUTTER_PSRAM_SIZE = res;// << 20;
    return BUTTER_PSRAM_SIZE;
} 
// упрощенная версия только PSRAM = 8
/* uint32_t __not_in_flash_func(get_psram_size)() {
             PSRAM_DATA[0] = 8;
        if (PSRAM_DATA[0]!=8)
            return PSRAM_DATA[0];

    return PSRAM_DATA[0];
}
 */


/**
 * @brief Инициализация PSRAM (APS6404) на Raspberry Pi Pico
 * @param cs_pin Номер пина Chip Select для PSRAM
 * 
 * @note Функция размещается в RAM (не во flash) для корректной работы с XIP
 */
void __no_inline_not_in_flash_func(init_psram_butter)(uint cs_pin) {
    // 1. Настройка пина Chip Select
    gpio_set_function(cs_pin, GPIO_FUNC_XIP_CS1);

    // 2. Включение прямого режима (Direct Mode) QSPI
    // - Делитель частоты 10 (для 125 МГц: 12.5 МГц)
    // - Автоматическое управление CS1
    qmi_hw->direct_csr = 10 << QMI_DIRECT_CSR_CLKDIV_LSB | 
                        QMI_DIRECT_CSR_EN_BITS | 
                        QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    // Ожидание завершения операции
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);

    // 3. Включение QPI-режима (4-битный интерфейс)
    const uint CMD_QPI_EN = 0x35; // Команда перехода в QPI режим
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_QPI_EN;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);

    // 4. Расчет временных параметров для PSRAM APS6404
    const int max_psram_freq = PSRAM_MAX_FREQ_MHZ * 1000000; // Макс. частота PSRAM (166 МГц)
  //  const int max_psram_freq = 166000000; // Макс. частота PSRAM (166 МГц)
    const int clock_hz = clock_get_hz(clk_sys); // Текущая частота ядра
    
    // Расчет делителя частоты
    int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2; // Ограничение для высоких частот
    }
   
    real_psram_freq = CPU_MHZ/divisor;

    // Расчет задержки чтения
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1; // Доп. задержка для частот >100 МГц
    }

    // 5. Настройка таймингов доступа
    // Расчет периода системного такта (в фемтосекундах)
    const int clock_period_fs = 1000000000000000ll / clock_hz;
    
    // Макс. время удержания CS (<=8 мкс)
    const int max_select = (125 * 1000000) / clock_period_fs; // 8000нс/64
    
    // Мин. время между транзакциями (>=18 нс)
    const int min_deselect = (18 * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;

    // Запись параметров в регистр таймингов
    qmi_hw->m[1].timing = 
        1 << QMI_M1_TIMING_COOLDOWN_LSB | // Время охлаждения
        QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB | // Граница страницы
        max_select << QMI_M1_TIMING_MAX_SELECT_LSB | // Макс. время CS
        min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB | // Мин. время между CS
        rxdelay << QMI_M1_TIMING_RXDELAY_LSB | // Задержка чтения
        divisor << QMI_M1_TIMING_CLKDIV_LSB; // Делитель частоты

    // 6. Настройка форматов команд
    // Формат чтения (Quad I/O Fast Read)
    qmi_hw->m[1].rfmt =
        QMI_M0_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_PREFIX_WIDTH_LSB | // 4-битный префикс
        QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_RFMT_ADDR_WIDTH_LSB | // 4-битный адрес
        QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB | // 4-битный суффикс
        QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_RFMT_DUMMY_WIDTH_LSB | // 4-битные dummy-циклы
        QMI_M0_RFMT_DATA_WIDTH_VALUE_Q << QMI_M0_RFMT_DATA_WIDTH_LSB | // 4-битные данные
        QMI_M0_RFMT_PREFIX_LEN_VALUE_8 << QMI_M0_RFMT_PREFIX_LEN_LSB | // 8 бит префикса
        6 << QMI_M0_RFMT_DUMMY_LEN_LSB; // 6 dummy-циклов
    
    qmi_hw->m[1].rcmd = 0xEB; // Команда чтения (Quad I/O Fast Read)

    // Формат записи (Quad Page Program)
    qmi_hw->m[1].wfmt =
        QMI_M0_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_PREFIX_WIDTH_LSB |
        QMI_M0_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_WFMT_ADDR_WIDTH_LSB |
        QMI_M0_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_SUFFIX_WIDTH_LSB |
        QMI_M0_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_WFMT_DUMMY_WIDTH_LSB |
        QMI_M0_WFMT_DATA_WIDTH_VALUE_Q << QMI_M0_WFMT_DATA_WIDTH_LSB |
        QMI_M0_WFMT_PREFIX_LEN_VALUE_8 << QMI_M0_WFMT_PREFIX_LEN_LSB;
    
    qmi_hw->m[1].wcmd = 0x38; // Команда записи (Quad Page Program)

    // 7. Завершение инициализации
    qmi_hw->direct_csr = 0; // Отключение прямого режима
    
    // Разрешение записи в PSRAM через XIP
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
}
//-------------------------------------------------------------------------------
/**
 * @brief Деинициализация PSRAM и восстановление настроек по умолчанию
 * @param cs_pin Номер пина Chip Select для PSRAM
 */
void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin) {
    // 1. Запрещаем запись в PSRAM через XIP
    hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);

    // 2. Сбрасываем настройки команд
    qmi_hw->m[1].rcmd = 0x00;  // Сброс команды чтения
    qmi_hw->m[1].wcmd = 0x00;  // Сброс команды записи

    // 3. Восстанавливаем стандартные тайминги
    qmi_hw->m[1].timing = 0x0; // Сброс всех битов таймингов

    // 4. Возвращаемся в SPI режим (из QPI)
    const uint CMD_SPI_EN = 0xF5; // Команда перехода в SPI режим
    qmi_hw->direct_csr = QMI_DIRECT_CSR_EN_BITS; // Включаем прямой режим
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_SPI_EN;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);

    // 5. Отключаем прямой режим
    qmi_hw->direct_csr = 0;

    // 6. Сбрасываем настройки QSPI
   // qmi_hw->io_ctrl = 0;        // Сброс управления вводом-выводом
   // qmi_hw->ctrl = 0;           // Сброс основного контроля

    // 7. Восстанавливаем функцию пина CS
    gpio_set_function(cs_pin, GPIO_FUNC_NULL);
    gpio_set_dir(cs_pin, GPIO_IN);

    // 8. Сбрасываем тактирование QSPI
  //  clocks_hw->periph_reset |= CLOCKS_PERIPH_RESET_QSPI_BITS;
 //   clocks_hw->periph_reset &= ~CLOCKS_PERIPH_RESET_QSPI_BITS;

    // 9. Отключаем банк памяти M1 в XIP
   // hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_ENABLE_M1_BITS);
}
//------------------------------------------------------------------------

//########################

#endif
//######################################################################################


void  fast(init_psram_board_all_version)(void)
#ifdef NO_PSRAM   
//-----------------NO PSRAM -------------------
  {
    psram_type = NOT_PSRAM;// 
  }
//----------------- PSRAM END ---------------
#endif
//#define NOT_PSRAM_21
#ifdef NOT_PSRAM_21    
//-----------------NO PSRAM -------------------
  {
    psram_type = NOT_PSRAM_PIN21;// NO PSRAM / Clock AY on pin21
  }
//----------------- PSRAM END ---------------
#endif
//----------------- PSRAM -------------------
//conf.mashine = 0; // only 128kB test
//----------------------------------------------------------------------------
#ifdef  PSRAM_BUTTER_OR_PSRAM_PSRAM_BOARD// если rp23550 на murm1  psram бутерброд и psram board
{
        init_psram_butter(psram_pin_cs);//PSRAM_BUTTER_PIN_CS=19 это PSRAM_PIN_CS для платы murm1 psram бутерброд
     //  size_psram=psram_b_size();
       size_psram= get_psram_size();
      if (size_psram!=0) 
      {
       psram_avaiable =1;
       type_psram=BUTTER_PSRAM;// тип psram бутерброд или на плате murm1
       gpio_put(LED_BOARD, 0); 
       return;
      }  

      /*значит PSRAM бутерброд нет проверяем PSRAM  на плате*/ 
        
       deinit_psram_butter(psram_pin_cs);


      size_psram =  init_psram_board();// если 0 то деинсталяция программы
      gpio_put(LED_BOARD, 0); 
//
if (size_psram==0) 
{
    type_psram = NOT_PSRAM;// PSRAM not found
}
    type_psram=BOARD_PSRAM;
    
}
#endif ////////////////////////////////////////////////////////////
//----------------------------------------------------------------
#ifdef PSRAM_BUTTER// если rp2350 и psram бутерброд 
{
       
    init_psram_butter(psram_pin_cs);  
    gpio_put(LED_BOARD, 0); 

      

          size_psram= get_psram_size();
      
//size_psram=0;
 if (size_psram==0) 
{
    type_psram = NOT_PSRAM;// PSRAM not found
    deinit_psram_butter(psram_pin_cs);
} 
else 
{
    psram_avaiable =1;
    type_psram=BUTTER_PSRAM;
}
}
#endif////////////////////////////////////////////////////////////
//----------------------------------------------------------------
#ifdef PSRAM_BOARD // если rp2040 и psram на плате murm1
{
   size_psram =  init_psram_board();// если 0 то деинсталяция программы
//
if (size_psram==0) 
{
    type_psram =NOT_PSRAM;// PSRAM not found
    return;
}

    type_psram = BOARD_PSRAM;
}
#endif
//--------------------------------------------------------------
#ifdef PSRAM_NOSUPORT // если rp2040 и psram на плате murm1
{

    type_psram=BOARD_PSRAM_NOSUPORT; 
}
#endif
//----------------- PSRAM END ---------------
//###########################################
//   Файловое меню
//###########################################         
void file_manager (void)     
         //    if ((is_menu_mode) && (!trdos))// файловое меню
            {
              
                //	tap_loader_active=false;// рудимент от аудио загрузки
                 hardAY_on_off=0;
                hardAY_off();// off hard AY файловое меню
                
               if (init_fs!=FR_OK)
                {
                    g_delay_ms(10);
                    init_fs = init_filesystem();
                    N_files = read_select_dir(cur_dir_index);
                    if (N_files == 0)  init_fs = FR_NO_FILE;
                }
                //++++++++++++++++++++++++++++++++++++++++++
              if (is_new_screen)
               { 

                     if (init_fs != FR_OK)
                 {
                    memset(g_gbuf, COLOR_BACKGOUND, sizeof(g_gbuf));
                    MessageBox("SD Card not found!!!", "    Please REBOOT   ", CL_LT_YELLOW, CL_RED, 0);
                    return; //continue;
                 } 
                      draw_main_window();// рисование рамок
                      draw_file_window();// рисование каталога файлов

 
                     if (init_fs==FR_OK)
                    {
                       N_files = read_select_dir(cur_dir_index);

                        if (N_files == 0)
                        {
                            init_fs = FR_NO_FILE;// нет файлов
                        }
                        else
                        {
                            cur_file_index_old = -1;
                        }
                      
                    }  
  
                }  
                  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++

                if (init_fs==FR_OK)
                {
                 // кнопка выхода из меню файлов по [ESC]
                         if ( (ESC_EXIT) )// exit [ESC]
             {
                            im_z80_stop = false;
                            is_menu_mode = false;
                            is_new_screen = false;
                            hardAY_on();//выход из меню файлов по [ESC]
                            return; //continue;
             }        

                   if (((KEY_SPACE)|JOY_B)  && (init_fs==FR_OK)) //  нажатие пробела запуск после сброса [B] joy
                    {        
                       // off_any_key() ;// отжатие любой клавиши
                     
                        strcpy(conf.activefilename, dir_patch); // strcpy
                        strcat(conf.activefilename, "/");
                        strcat(conf.activefilename, files[cur_file_index]);

                        afilename[0] = 0;
                        strcat(afilename, files[cur_file_index]);

                        const char *ext = get_file_extension(conf.activefilename);
                        if (strcasecmp(ext, "trd") == 0) //   запуск после сброса
                        {
                            // file_select_trdos();
                            // Копируем строку длиною не более 10 символов из массива src в массив dst1.
                            // strncpy (dst1, src,3);

                            MessageBox(" RUNING TRD FILE ", "", CL_WHITE, CL_BLUE, 4);

                             file_type[0] = TRD; //trd
                             conf.FileAutorunType = TRD; // к диску а подключен TRD образ
                             strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                             OpenTRDFile(conf.activefilename,0);
                             write_protected = false; // защита записи отключена для TRD

                            zx_machine_reset(1);// включить загрузку файла при reset 1 раз

                            is_new_screen = false;
                            is_menu_mode = false;
                            im_z80_stop = false;
                            return;// continue; 
                        }
                        if (strcasecmp(ext, "scl") == 0) //  запуск после сброса
                        {
                            MessageBox(" RUNING SCL FILE ", "", CL_WHITE, CL_BLUE, 4);
                            write_protected = true; // защита записи для SCL
                            conf.FileAutorunType = SCL;
                            file_type[0] = SCL;
                            strncpy(conf.DiskName[0], files[cur_file_index], LENF); // disk A
                            Run_file_scl(conf.activefilename, 0);
                            zx_machine_reset(1);// включить загрузку файла при reset 1 раз
                            im_z80_stop = false;
                            is_menu_mode = false;
                            is_new_screen = false;
                            return; // continue;
                        }
//##########################################
                        if (strcasecmp(ext, "fdi") == 0) //   запуск после сброса
                        {		
                            MessageBox(" RUNING FDI FILE ", "", CL_WHITE, CL_BLUE, 4);
                             conf.FileAutorunType = FDI;
                             file_type[0] = FDI; 
                             strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                             OpenFDI_File(conf.activefilename,0);
                          //   write_protected = true; // защита записи включена

                            zx_machine_reset(1);// включить загрузку файла при reset 1 раз

                            is_new_screen = false;
                            is_menu_mode = false;
                            im_z80_stop = false;
                            return;// continue; 
                        }
//##########################################
                            return; // continue;
                    }// end KEY_SPACE

                       if (((KEY_ENTER)|JOY_A) && (init_fs==FR_OK))// нажатие enter
                      
                    {
     
                        flag_usb_kb = false;  

                        if (files[cur_file_index][LENF1])
                        { // выбран каталог

                            if (cur_file_index == 0)
                            { // на уровень выше
                                if (cur_dir_index)
                                {                      
                                    cur_dir_index--;
                                    N_files = read_select_dir(cur_dir_index);
                                    cur_file_index = 0;
                                   // draw_text_len(2 + FONT_W, 2 * FONT_H , " ****     ", CL_TEST, COLOR_BORDER, 14);
                                    return; // continue;
                                };
                            }
                            if (cur_dir_index < (DIRS_DEPTH - 2))
                            { // выбор каталога
                                cur_dir_index++;
                                strncpy(dirs[cur_dir_index], files[cur_file_index], LENF1);
                                N_files = read_select_dir(cur_dir_index);
                                cur_file_index = 0;
                                cur_file_index_old = cur_file_index;
                               shift_file_index=0;
                                last_action = time_us_32();
                                return; // continue;
                            }
                        }
                        //
                        else
                        { // выбран файл

                            strcpy(conf.activefilename, dir_patch); // strcpy
                            strcat(conf.activefilename, "/");
                            strcat(conf.activefilename, files[cur_file_index]);

                            afilename[0] = 0;
                            strcat(afilename, files[cur_file_index]);
                            const char *ext = get_file_extension(conf.activefilename);

                            if (strcasecmp(ext, "z80") == 0)
                            {
                                im_z80_stop = true;
                                while (im_z80_stop)
                                {
                                    sleep_ms(10);
                                    if (im_ready_loading)
                                    {
                                        // sleep_ms(10);
                                        conf.turbo = 0;
                                        turbo_switch();
                                        zx_machine_reset(3);
                                      //  AY_reset(); // сбросить AY

                                        if (load_image_z80(conf.activefilename))
                                        {
                                            memset(temp_msg, 0, sizeof(temp_msg));
                                            sprintf(temp_msg, " Loading file:%s", afilename);
                                            MessageBox("Z80", temp_msg, CL_WHITE, CL_BLUE, 2);
                                            conf.activefilename[0] = 0;
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            is_menu_mode = false;
                                            break;
                                        }
                                        else
                                        {
                                            MessageBox("Error loading snapshot!!!", afilename, CL_YELLOW, CL_LT_RED, 1);
                                            last_action = time_us_32();
                                            draw_file_window();
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            break;
                                        }
                                    }
                                }
                                return; // continue;
                            }
                            else if (strcasecmp(ext, "sna") == 0)
                            {
                                // G_PRINTF_DEBUG("current file select=%s\n",conf.activefilename);
                                // load_image_z80(conf.activefilename);
                                im_z80_stop = true;
                                while (im_z80_stop)
                                {
                                    sleep_ms(10);
                                    if (im_ready_loading)
                                    {
                                     //   if (conf.mashine!=SCORP256) zx_machine_reset(3); // убрать для работы в SCORPION
                                        zx_machine_reset(3);
                                      //  AY_reset(); // сбросить AY
                                        if (load_image_sna(conf.activefilename))
                                        {
                                            memset(temp_msg, 0, sizeof(temp_msg));
                                            sprintf(temp_msg, " Loading file:%s", afilename);
                                            MessageBox("SNA", temp_msg, CL_WHITE, CL_BLUE, 2);
                                            conf.activefilename[0] = 0;
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            is_menu_mode = false;
                                            break;
                                        }
                                        else
                                        {
                                            MessageBox("Error loading snapshot!!!", afilename, CL_YELLOW, CL_LT_RED, 1);
                                            // printf("load_image_sna - ERROR\n");
                                            last_action = time_us_32();
                                            draw_file_window();
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            break;
                                        }
                                    }
                                }
                                return; // continue;
                            }
                            else if (strcasecmp(ext, "scr") == 0)
                            {
                                if (LoadScreenshot(conf.activefilename, true))
                                {
                                    is_menu_mode = false;
                                    return; // continue;
                                }
                                else
                                {
                                    MessageBox("Error loading screen!!!", afilename, CL_YELLOW, CL_LT_RED, 1);
                                   // break;
                                }
                            }
                            else if (strcasecmp(ext, "tap") == 0)
                            {
                                Set_load_tape(conf.activefilename, current_lfn);
                                strcpy(temp_msg, current_lfn);
                                MessageBox("    TAPE    ", temp_msg, CL_WHITE, CL_BLUE, 4);
                                //     zx_machine_reset();

                                im_z80_stop = false;
                                is_menu_mode = false;
                                 hardAY_on();// exit tape select
                                return; // continue;
                            }
                            // TRDOS обработка

                            if (strcasecmp(ext, "trd") == 0)
                            {
                                file_select_trdos();
                                return; // continue;
                            }
                            // SCL обработка

                            if (strcasecmp(ext, "scl") == 0)
                            {
                                MessageBox("SCL files are mounted", "   only on Drive A:", CL_WHITE, CL_BLUE, 4);// 3 delay 250 4 -1000
                                conf.FileAutorunType = SCL;
                               file_type[0] = SCL;
                                strncpy(conf.DiskName[0], files[cur_file_index], LENF);// disk A
                                Run_file_scl(conf.activefilename, 0);

                                draw_main_window(); // восстановление текста
                                draw_file_window();
                                last_action = time_us_32();
                                is_new_screen = 1;
                                return; // continue;
                            }

                            if (strcasecmp(ext, "fdi") == 0)
                            {
                                MessageBox("FDI files are mounted", "   only on Drive A:", CL_WHITE, CL_BLUE, 4);// 3 delay 250 4 -1000
                                conf.FileAutorunType = FDI;
                                file_type[0] = FDI;
                                strncpy(conf.DiskName[0], files[cur_file_index], LENF);// disk A
                                OpenFDI_File(conf.activefilename,0);

                                draw_main_window(); // восстановление текста
                                draw_file_window();
                                last_action = time_us_32();
                                is_new_screen = 1;
                                return; // continue;
                            }


                        }
                    }



                    int num_show_files = 18; // количество файлов при показе


                    // стрелки вверх вниз
                    if (((kb_st_ps2.u[2] & KB_U2_DOWN) | JOY_DOWN) && (cur_file_index < (N_files)))
                    {
                        cur_file_index++;
                        last_action = time_us_32();
                        
                    }
                   if (((kb_st_ps2.u[2] & KB_U2_UP) | JOY_UP) && (cur_file_index > 0))
                    {
                        cur_file_index--;
                        last_action = time_us_32();
                       
                    }
                    // начало и конец списка
                    if ((kb_st_ps2.u[2] & KB_U2_LEFT))
                    {
                        cur_file_index = 0;
                        shift_file_index = 0;
                        last_action = time_us_32();
                       
                    }
                    if ((kb_st_ps2.u[2] & KB_U2_RIGHT))
                    {
                        cur_file_index = N_files;
                        shift_file_index = (N_files >= num_show_files) ? N_files - num_show_files : 0;
                        last_action = time_us_32();
                        
                    }

                    // PAGE_UP PAGE_DOWN
                    if (((kb_st_ps2.u[2] & KB_U2_PAGE_DOWN) | JOY_RIGHT) && (cur_file_index < (N_files)))
                    {
                        cur_file_index += num_show_files;
                        last_action = time_us_32();
                        
                    }
                    if (((kb_st_ps2.u[2] & KB_U2_PAGE_UP) | JOY_LEFT) && (cur_file_index > 0))
                    {
                        cur_file_index -= num_show_files;
                        last_action = time_us_32();
                        
                    }
                    // Возврат на уровень выше по BACKSPACE
                    if ((kb_st_ps2.u[1] & KB_U1_BACK_SPACE) | (data_joy == 0x40)) 
                    {
                        if (cur_dir_index == 0)
                        {
                            if (cur_file_index == 0)
                                cur_file_index = 1; // не можем выбрать каталог вверх
                            if (shift_file_index == 0)
                                shift_file_index = 1; // не отображаем каталог вверх
                            read_select_dir(cur_dir_index);
                        }
                        else
                        {
                            cur_dir_index--;
                            N_files = read_select_dir(cur_dir_index);
                            cur_file_index = 0;
                            draw_text_len(2 + FONT_W, FONT_H - 1, "                    ", COLOR_BACKGOUND, COLOR_BORDER, 20);
                            cur_file_index = 0;
                            shift_file_index = 0;
                          //  continue;
                        }
                    }
                    if (cur_file_index < 0)
                        cur_file_index = 0;
                    if (cur_file_index >= N_files)
                        cur_file_index = N_files;

                    if (data_joy > 0)
                    {
                        old_data_joy = 0;
                    };

                    for (int i = num_show_files; i--;)
                    {
                        if ((cur_file_index - shift_file_index) >= (num_show_files))
                            shift_file_index++;
                        if ((cur_file_index - shift_file_index) < 0)
                            shift_file_index--;
                    }

                    // ограничения корневого каталога
                    if (cur_dir_index == 0)
                    {
                        if (cur_file_index == 0)
                            cur_file_index = 1; // не можем выбрать каталог вверх
                        if (shift_file_index == 0)
                            shift_file_index = 1; // не отображаем каталог вверх
                    }

                    // прорисовка
                    // заголовок окна - текущий каталог

                    if (strlen(dir_patch) > 0)
                    {
                        draw_text_len(FONT_W + 3, FONT_H - 1, dir_patch + 1, COLOR_UP, COLOR_BORDER, 51);// путь папки в шапке
                    }
                    else
                    {
                        draw_text_len(FONT_W + 3, FONT_H - 1, "                                                  ", CL_TEST, COLOR_BORDER, 51);
                    }

                    for (int i = 0; i < num_show_files; i++)
                    {
                        uint8_t color_text = CL_GREEN;//!!!!!
                        uint8_t color_text_d = CL_YELLOW; // если директория
                        uint8_t color_bg = COLOR_BACKGOUND;

                        if (i == (cur_file_index - shift_file_index))
                        {
                            color_text = CL_BLACK;
                            color_bg = COLOR_SELECT;
                            color_text_d = CL_BLACK;
                        }
                        // если файлов меньше, чем отведено экрана - заполняем пустыми строками
                        if ((i > N_files) || ((cur_dir_index == 0) && (i > (N_files - 1))))
                        {
                            draw_text_file(4+ FONT_W, 2 * FONT_H + i * FONT_H, " ", color_text, color_bg, NUMBER_CHAR);
                            continue;
                        }

                        if (files[i + shift_file_index][LENF1])
                        {

                            draw_text_file(4 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text_d, color_bg, NUMBER_CHAR); 
                        }
                        else
                        {

                            draw_text_file(4 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text, color_bg, NUMBER_CHAR); 
                        }
                    }
                     // имя длинное должно быть
                   strcpy(current_lfn, get_current_altname(dir_patch, files[cur_file_index]));

                    // draw_rect(10+FONT_W*13,17,3,5,0xf,true);
                    int file_inx = cur_file_index - 1;
                    if (file_inx == -1)
                        file_inx = 0;
                    if (file_inx == N_files)
                        file_inx += 1;
       
               //     int shft = 156 * (file_inx) / (N_files <= 1 ? 1 : N_files - 1);

                    if (strcasecmp(files[cur_file_index], "..") == 0)
                    {
                        cur_file_index_old = cur_file_index;
                    
                    }


                 if (cur_file_index)  
                 {
//======================================================================================================================
               strncpy(temp_msg, get_lfn_from_dir(dir_patch, files[cur_file_index]),72); // имя длинное должно быть
				draw_text_len(12+FONT_W*14,18, temp_msg,CL_INK,COLOR_BACKGOUND,35); // длинное имя должно быть показывается вверху
                for (size_t i = 0; i < 36; i++)
                {
                   temp_msg[i] = temp_msg[i+35];
                }
                draw_text_len(12+FONT_W*14,28, temp_msg,CL_INK,COLOR_BACKGOUND,35); // длинное имя длжно быть показывается вверху
 //=====================================================================================================================
                 }
                   else
                   {
                    draw_rect(FONT_W*16,18,FONT_W*36,20,COLOR_BACKGOUND,true) ;
                   }

                    if ((cur_file_index > 0) && (cur_file_index_old == -1))
                    {
                        last_action = time_us_32();  
                    }
                }
                //++++++++++++++++++++++++++++++++++++++++++++++++++++++
                file_info ();
            }
//################################################################################
// информация о файлах
      // if ((is_menu_mode) && (init_fs==FR_OK))
void file_info (void)       
      
      {

       //     if ((last_action > 0) && (time_us_32() - last_action) > SHOW_SCREEN_DELAY * 1000)
            {
                 last_action = 0;
            //    CLEAR_INFO; // Фон отображения информации о файле
                 const char* ext = get_file_extension(files[cur_file_index]);
                //-----------------------------------------------
                // TRD INFO
                if (strcasecmp(ext, "trd") == 0)
                {
                    strncpy(temp_msg, current_lfn, 22);
                    strcpy(conf.activefilename, dir_patch);
                    strcat(conf.activefilename, "/");
                    strcat(conf.activefilename,files[cur_file_index]);
                    cur_file_index_old = cur_file_index;

                    if (!ReadCatalog(conf.activefilename, current_lfn, false))
                    {

                        //    
                    }

                     return; 
                }
                //-----------------------------------------------
                // FDI INFO
                if (strcasecmp(ext, "fdi") == 0)
                {
                    strncpy(temp_msg, current_lfn, 22);
                    strcpy(conf.activefilename, dir_patch);
                    strcat(conf.activefilename, "/");
                    strcat(conf.activefilename,files[cur_file_index]);
                    cur_file_index_old = cur_file_index;

                    if (!Read_Info_FDI(conf.activefilename, current_lfn, false))
                    {

                        //    
                    }

                     return; 
                }
                //-----------------------------------------------               
                // SCL INFO
                if (strcasecmp(ext, "scl") == 0)
                {
					strncpy(temp_msg,current_lfn,22);
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					cur_file_index_old=cur_file_index;		
			    	if(!ReadCatalog_scl(conf.activefilename,current_lfn,false))
                    {
				    } 
                 return; 
                 }
                 //-----------------------------------------------------
				if(strcasecmp(ext, "z80")==0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					cur_file_index_old=cur_file_index;					
					if(!LoadScreenFromZ80Snapshot(conf.activefilename)){
						CLEAR_INFO;
					}
					return; 
				} else
				if(strcasecmp(ext, "scr") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
				//	printf("LoadScreenshot: %s\n",conf.activefilename);
					CLEAR_INFO;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenshot(conf.activefilename,false)){
						CLEAR_INFO;

					} 
					return; 
				} else
				if(strcasecmp(ext, "tap") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					CLEAR_INFO;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenFromTap(conf.activefilename))
						CLEAR_INFO;;
					return; 
				} 
				if(strcasecmp(ext, "sna") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					CLEAR_INFO;;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenFromSNASnapshot(conf.activefilename)){
						CLEAR_INFO;;
					}
					return; 
				} 
				else {
					if (cur_file_index_old==-1) CLEAR_INFO;//Фон отображения скринов и информации очистка	
					 else CLEAR_INFO;
					cur_file_index_old=cur_file_index;    
				}
			}


		}



//################################################################################
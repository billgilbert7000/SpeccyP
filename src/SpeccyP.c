#include <stdio.h>
#include "config.h" 
#include "SpeccyP.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
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

#ifdef PICO_RP2350
#include <hardware/structs/qmi.h>
#include <hardware/structs/xip.h>
#include <hardware/regs/sysinfo.h>
#endif

#include "screen_util.h"
#include "tf_card.h"
#include "ff.h"
#include "util_z80.h"
#include "util_sna.h"
#include "util_tap.h"
#include "string.h"
#include "util_sd.h"
#include "util_trd.h"
#include "util_cpm.h"
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

#ifdef GENERAL_SOUND   
#include "gs_picobus.h"
#endif

#include "zx_emu/zx_machine.h"
#include "zx_emu/Z80.h"

#ifndef USB_SERIAL
#include "tusb.h"
#endif

#include "usb_key.h"
#include "joy.h"
#include "diskio.h"

extern Z80 cpu;
int real_psram_freq;
int real_flash_freq;
uint8_t vout_select;

// ===============================================================
// СОСТОЯНИЕ ЭМУЛЯЦИИ (единая точка управления)
// ===============================================================

// Определение переменной (без static, чтобы была видна в других файлах)
EmuState emu_state = EMU_RUNNING;
static EmuState prev_emu_state = EMU_RUNNING;  // static - только в этом файле

// ===============================================================
// УПРАВЛЕНИЕ СОСТОЯНИЕМ ЭМУЛЯЦИИ
// ===============================================================
inline void emu_stop(void) {
    if (emu_state == EMU_RUNNING) {
        prev_emu_state = EMU_RUNNING;
    }
    emu_state = EMU_STOPPED;
    zx_machine_enable_vbuf(false);
    hardAY_off();
}

inline void emu_start(void) {
    prev_emu_state = EMU_STOPPED;
    emu_state = EMU_RUNNING;
    zx_machine_enable_vbuf(true);
    hardAY_on();
}

inline void emu_toggle(void) {
    if (emu_state == EMU_RUNNING) {
        emu_stop();
    } else {
        emu_start();
    }
}

inline EmuState emu_suspend(void) {
    EmuState old_state = emu_state;
    if (emu_state == EMU_RUNNING) {
        emu_stop();
    }
    return old_state;
}

inline void emu_restore(EmuState state) {
    emu_state = state;
    if (emu_state == EMU_RUNNING) {
        zx_machine_enable_vbuf(true);
        hardAY_on();
    } else {
        zx_machine_enable_vbuf(false);
        hardAY_off();
    }
}

inline bool emu_is_running(void) {
    return emu_state == EMU_RUNNING;
}

inline bool emu_is_stopped(void) {
    return emu_state == EMU_STOPPED;
}

inline bool emu_just_stopped(void) {
    return (emu_state == EMU_STOPPED && prev_emu_state == EMU_RUNNING);
}

// ===============================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ===============================================================
void file_manager(void);
void file_info(void);
void file_select_trdos(void);
void file_select_cpm(void);
void setup_zx(void);
void config_init(void);
void help_zx(void);
void pause_zx(void);
void help_keyboard(void);
void led_trdos(void);
void load_slot(void);
void save_all(void);
void save_slot(void);
void disk_autorun(void);
void disasm(void);
void fast(init_psram_board_all_version)(void);
uint8_t psram_pin_cs;
bool rp2350a;

#if PICO_RP2350
void __no_inline_not_in_flash_func(init_psram_butter)(uint cs_pin);
void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin);
static uint32_t __no_inline_not_in_flash_func(psram_b_size)(void);
uint32_t get_psram_size();
static bool __no_inline_not_in_flash_func(psram_detect)(void);
#endif

// ===============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ===============================================================
char auto_start_filename[LENF];
uint8_t saves[11];
static uint32_t last_action = 0;
bool need_reset_after_menu = false;
bool is_osk_mode = false;
uint8_t current_key = 0;
char save_file_name_image[25];

// Переменные для меню
int shift_file_index = 0;
int cur_dir_index = 0;
int cur_file_index = 0;
int cur_file_index_old = 0;
int N_files = 0;
char current_lfn[LENF];
int lineStart = 0;
int tap_block_percent = 0;

bool old_key_menu_state = false;
bool key_menu_state = false;
bool old_key_help_state = false;
bool key_help_state = false;
bool help_mode_draw = false;

extern bool im_ready_loading;

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

	//bool need_reset_after_menu=false;

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
    return NULL;
}

// Меню
char __in_flash() *menu_initial[4] = { " " };
char __in_flash() *menu_setup_tft[16] = {
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
    " Update mode   ",
    " Exit          ",
    "               ",
};

char __in_flash() *menu_setup[16] = {
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
    " Update mode   ",
    " Exit          ",
    "               ",
};

// меню help keyboard
char __in_flash() *menu_keyboard[1]={"",	};

char __in_flash() *menu_help[17] = {
    "[F11], [Insert] file browser",
    "[F12], [HOME] settings menu",
    "[F2] Save slots [F3] Load slots",
    "[F4] Help ZX Keyboard",
    "[F5] Save slots 0 and config",
    "[F7] Volume down [F8] Volume up",
    "[F10] Normal/Turbo/Fast",
    "[F9] NMI key",
    "[ESC] exit almost all menus",
    "[END] Dissasembler",
    "[END] USB Update mode only here",
    "  ",
    "Video driver, etc. @Alex_Eburg",
    "https://t.me/ZX_MURMULATOR",
    "  ",
    "The author of the compilation:",
    "https://t.me/const_bill",
};

char __in_flash() *menu_sound[8] = {
    " Soft AY-3-8910  ",
    " Soft TurboSound ",
    " I2S AY-3-8910   ",
    " I2S TurboSound  ",
    " Hard AY-3-8910  ",
    " Hard TurboSound ",
    " Hard TSFM *     ",
    " Hard TS+SAA *   ",
};

char __in_flash() *menu_joy[6] = {
    " Kempston  NES Joy ",
    " Kempston   Arrows ",
    " Interface2 Arrows ",
    " Cursor     Arrows ",
    " Q A O P M  Arrows ",
    " Kempston   WASDKL ",
};

char __in_flash() *menu_autorun[3] = {
    " OFF              ",
    " File TR-DOS      ",
    " QuickSave Slot 0 ",
};

char __in_flash() *menu_speed[2] = {
    " NORMAL INT  50Hz ",
    " FAST   INT 100Hz ",
};

char __in_flash() *menu_pallete[12] = {
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

char __in_flash() *submenu_setup_tft[11] = {
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

// ===============================================================
// ДЖОЙСТИК
// ===============================================================
void (*joy_scan)(void);

void joy_mode_0(void) {
    if (kb_st_ps2.u[2] & KB_U2_UP) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 3;
    }
    if (kb_st_ps2.u[2] & KB_U2_DOWN) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 4;
    }
    if (kb_st_ps2.u[2] & KB_U2_LEFT) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[3] |= 1 << 4;
    }
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 2;
    }
}

void joy_mode_1(void) {
    data_joy = 0;
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) data_joy |= 0b00000001;
    if (kb_st_ps2.u[2] & KB_U2_LEFT) data_joy |= 0b00000010;
    if (kb_st_ps2.u[2] & KB_U2_DOWN) data_joy |= 0b00000100;
    if (kb_st_ps2.u[2] & KB_U2_UP) data_joy |= 0b00001000;
    if (JOY_FIRE) data_joy |= 0b00010000;
    zx_input.kempston = (uint8_t)(data_joy);
}

void joy_mode_2(void) {
    if (kb_st_ps2.u[2] & KB_U2_UP) zx_input.kb_data[4] |= 1 << 1;
    if (kb_st_ps2.u[2] & KB_U2_DOWN) zx_input.kb_data[4] |= 1 << 2;
    if (kb_st_ps2.u[2] & KB_U2_LEFT) zx_input.kb_data[4] |= 1 << 4;
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) zx_input.kb_data[4] |= 1 << 3;
    if (JOY_FIRE) zx_input.kb_data[4] |= 1 << 0;
}

void joy_mode_3(void) {
    if (kb_st_ps2.u[2] & KB_U2_UP) zx_input.kb_data[4] |= 1 << 3;
    if (kb_st_ps2.u[2] & KB_U2_DOWN) zx_input.kb_data[4] |= 1 << 4;
    if (kb_st_ps2.u[2] & KB_U2_LEFT) zx_input.kb_data[3] |= 1 << 4;
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) zx_input.kb_data[4] |= 1 << 2;
    if (JOY_FIRE) zx_input.kb_data[4] |= 1 << 0;
}

void joy_mode_4(void) {
    if (kb_st_ps2.u[2] & KB_U2_UP) zx_input.kb_data[2] |= 1 << 0;
    if (kb_st_ps2.u[2] & KB_U2_DOWN) zx_input.kb_data[1] |= 1 << 0;
    if (kb_st_ps2.u[2] & KB_U2_LEFT) zx_input.kb_data[5] |= 1 << 1;
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) zx_input.kb_data[5] |= 1 << 0;
    if (JOY_FIRE) zx_input.kb_data[7] |= 1 << 2;
}

void joy_mode_5(void) {
    data_joy = 0;
    if (kb_st_ps2.u[0] & KB_U0_D) data_joy |= 0b00000001;
    if (kb_st_ps2.u[0] & KB_U0_A) data_joy |= 0b00000010;
    if (kb_st_ps2.u[0] & KB_U0_W) data_joy |= 0b00001000;
    if (kb_st_ps2.u[0] & KB_U0_S) data_joy |= 0b00000100;
    if (kb_st_ps2.u[0] & KB_U0_K) data_joy |= 0b00010000;
    if (kb_st_ps2.u[0] & KB_U0_L) data_joy |= 0b00100000;
    zx_input.kempston = (uint8_t)(data_joy);
    
    if (kb_st_ps2.u[2] & KB_U2_UP) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 3;
    }
    if (kb_st_ps2.u[2] & KB_U2_DOWN) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 4;
    }
    if (kb_st_ps2.u[2] & KB_U2_LEFT) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[3] |= 1 << 4;
    }
    if (kb_st_ps2.u[2] & KB_U2_RIGHT) {
        zx_input.kb_data[0] |= 1 << 0;
        zx_input.kb_data[4] |= 1 << 2;
    }
}

void joy_redirecting(void) {
    switch (conf.joyMode) {
        case 0: joy_scan = joy_mode_0; break;
        case 1: joy_scan = joy_mode_1; break;
        case 2: joy_scan = joy_mode_2; break;
        case 3: joy_scan = joy_mode_3; break;
        case 4: joy_scan = joy_mode_4; break;
        case 5: joy_scan = joy_mode_5; break;
    }
}

// ===============================================================
// ИНИЦИАЛИЗАЦИЯ И РАЗГОН
// ===============================================================

#ifdef PICO_RP2350
static void __no_inline_not_in_flash_func(set_flash_timings)(void) {
    const uint32_t clock_hz = conf.cpu_freq * 1000000;  
    const int max_flash_freq = FLASH_MAX_FREQ_MHZ * 1000000;

    int divisor = (clock_hz + max_flash_freq - 1) / max_flash_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }
    qmi_hw->m[0].timing = 0x60007000 |
                          rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                          divisor << QMI_M0_TIMING_CLKDIV_LSB;
    real_flash_freq = clock_hz / divisor / 1000000;
}

//############################################################
void fast(init_pico)(void) // настройка и разгон для RP2350
{
    vreg_disable_voltage_limit();
    vreg_set_voltage(conf.voltage);
  //  sleep_ms(50);
    cpu_pico_khz = conf.cpu_freq * 1000;
    // Настройка делителя для HDMI в зависимости от частоты pico 
    conf.hdmi_fdiv = 1.0; // 1.0->60Hz cpu=252MHz
    if (conf.cpu_freq == 504)
        conf.hdmi_fdiv = 2.0; // 60Hz
    else if (conf.cpu_freq == 378)
        conf.hdmi_fdiv = 1.5; // 60Hz
    else if (conf.cpu_freq == 252)
        conf.hdmi_fdiv = 1.0; // 60Hz
 //   const uint32_t ints = save_and_disable_interrupts();    
    
    // На время смены частоты флешу нужен ЗАВЕДОМО безопасный тайминг.
    //
    // set_sys_clock_pll() лежит во флеше и дотягивает свои же инструкции через
    // XIP прямо во время подъёма частоты. Ни стартовый делитель (расчитан на
    // 120 МГц, на 378 МГц читать им нельзя), ни финальный (расчитан на 378 МГц,
    // на 120 МГц у него слишком большой RXDELAY) в этот момент не годятся: в
    // конвейер приезжает мусор, ловим UNDEFINSTR -> HardFault -> lockup.
    // Проскочит или нет — зависит от того, лежит ли нужная строка в XIP-кэше,
    // поэтому без SD-карты обычно проносит, а с картой (FatFs вытесняет кэш)
    // плата стабильно виснет на старте.
    //
    // CLKDIV=4, RXDELAY=2 корректен и на 120 МГц (30 МГц флеш), и на 378 МГц
    // (94.5 МГц) — ровно этим значением пользовались релизные 1.7.4. Финальный
    // тайминг ставим уже после разгона, когда частота устоялась.
    qmi_hw->m[0].timing = 0x60007204;

    set_sys_clock_khz(conf.cpu_freq * 1000, 0);
    set_flash_timings();  // Настройка таймингов для новой частоты
 //   restore_interrupts(ints);   
 }

#else
void fast(init_pico)(void) // настройка и разгон для RP2040
{
    // hw_set_bits(&vreg_and_chip_reset_hw->vreg, VREG_AND_CHIP_RESET_VREG_VSEL_BITS);
    // sleep_ms(10);
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(50);
    cpu_pico_khz = CPU_MHZ * 1000;
    conf.cpu_freq = CPU_MHZ;
    set_sys_clock_khz(cpu_pico_khz, false);
    conf.hdmi_fdiv = 1.5;
}
#endif
// ===============================================================
// ИНИЦИАЛИЗАЦИЯ USB
// ===============================================================
void init_usb(void) {
    init_usb_hid();
    for (int i = 0; i < 320; i++) {
        tuh_task();
        g_delay_ms(10);
        draw_symbol(i, 240 - 48, 0, CL_WHITE, 0);
        draw_symbol(320 - i + 1, 240 - 48, 0x20, CL_WHITE, 0);
        draw_symbol(320 - i, 240 - 48, 0, CL_WHITE, 0);
    }
}

// ===============================================================
// ИНИЦИАЛИЗАЦИЯ И ИНФОРМАЦИЯ
// ===============================================================
static uint inx = 0;

void init_and_info() {
    init_psram_board_all_version();
    //psram_avaiable =0;         // ❌тест без psram
    //type_psram=NOT_PSRAM;      // ❌тест без psram
    #if LED_BOARD != 255
    gpio_put(LED_BOARD, 0);
    #endif

    #if PI_CARD// ???? это для того чтобы настроить выходы для PICARD 1 ИНАЧЕ МИКРОСХЕМЫ ОБВЯЗКИ ГРЕЮТСЯ
    pio_set_gpio_base(PIO_PS2, 0x10);
    gpio_init(40);
    gpio_set_dir(40, GPIO_OUT);
    gpio_put(41, 1);
    gpio_init(41);
    gpio_set_dir(41, GPIO_OUT);
    gpio_put(41, 1);
    #endif

    #ifdef WS_ZERO2
    pio_set_gpio_base(PIO_VIDEO, 0x10);
    for (uint8_t p = 44; p <= 46; p++) {
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
    }
    for (uint8_t p = 44; p <= 46; p++)
        gpio_deinit(p);
    #endif

    #ifdef GENERAL_SOUND
    gpio_out_init(BEEP_PIN);
    #endif

    gpio_in_init(PIN_ZX_LOAD);

    #ifdef HDMI_ONLY
    vout_select = VIDEO_HDMI;
    startVIDEO(VIDEO_HDMI);
    #else
    #if defined(HDMI_HSTX)
    vout_select = VIDEO_HDMI;
    #else
    vout_select = video_select();
    #endif
    startVIDEO(vout_select);
    #endif

    #ifdef DEVICE_HDMI90_I2S
    conf.type_sound = 3;
    conf.hdmi_fdiv = 1.0;
    vout_select = VIDEO_HDMI;
    startVIDEO(VIDEO_HDMI);
    #endif

    #ifdef SOUND_I2S_ONLY
    conf.type_sound = I2S_TS;
    #endif
    #ifdef SOUND_PWM_ONLY
    conf.type_sound = 1;
    #endif

#if defined(HDMI_HSTX) && !defined(HDMI_HSTX_DVI)
        // В HDMI HSTX-сборке (с HDMI audio через Data Islands) звук
        // обязан идти через i2s_out — иначе в HDMI поедет тишина, а
        // sample'ы AY уйдут на PWM-пины мимо приёмника. Любые другие
        // режимы (Soft AY, Hard AY, TSFM…) подменяем на I2S_TS, чтобы
        // user'ская настройка звука не глушила HDMI audio.
        if (conf.type_sound != I2S_AY && conf.type_sound != I2S_TS) {
            conf.type_sound = I2S_TS;
        }
#endif

    zx_machine_enable_vbuf(false);
    init_screen(g_gbuf, SCREEN_W, SCREEN_H);
    draw_text(160 - 21, 240 - 64, "SpeccyP", CL_WHITE, CL_BLACK);

    turbo_switch();
    joy_redirecting();

    #if LED_BOARD != 255
    gpio_put(LED_BOARD, 0);
    #endif

//---------------------------------------------------------------------
// Защита от автозапуска пустого или некорректного диска в CP/M Кворума
// после включения и при Hard Reset 
if (conf.mashine==QUORUM1024) conf.Disks[0][0] =0 ;  
// TODO надо сделать правильно и поумнее
//####################################################################
#ifdef USB_SERIAL
   // Режим USB CDC консоли: USB HID хост отключаем, вместо него
   // используем CDC-ACM для отладочного вывода. Даём 5 секунд на
   // подключение терминала со стороны хоста до запуска эмулятора.
   stdio_init_all();
   for (int i = 0; i < 50; i++) { g_delay_ms(100); }
#else

  if (conf.autorun == 0) {sleep_ms(100); init_usb();} // Инициализация USB
  else  init_usb_hid(); // USB HID
#endif

    convert_kb_u_to_kb_zx(&kb_st_ps2, zx_input.kb_data);

    #define YPOS FONT_H * 2
    #define XPOS FONT_W + 2
    uint8_t y_info = YPOS;

    draw_rect(0, 0, SCREEN_W, SCREEN_H, CL_BLACK, true);
    draw_rect(9, 9, SCREEN_W - 20, SCREEN_H - 100, CL_CYAN, false);
    draw_logo(156, 160, CL_CYAN, CL_BLACK);
    draw_text(7 + XPOS, 0 + YPOS, "ZX SpeccyP", CL_LT_CYAN, CL_BLACK);
    draw_text_len(7 + 11 * FONT_W + XPOS, YPOS, FW_VERSION, CL_LT_CYAN, CL_BLACK, 16);

    #ifndef PICO_RP2040
    if (rp2350a) snprintf(temp_msg, sizeof temp_msg, "RP2350A %dMHz", conf.cpu_freq);
    else snprintf(temp_msg, sizeof temp_msg, "RP2350B %dMHz", conf.cpu_freq);
    #else
    snprintf(temp_msg, sizeof temp_msg, " RP2040 %dMHz", CPU_MHZ);
    #endif
    draw_text(210 + XPOS, YPOS, temp_msg, CL_GRAY, CL_BLACK);

    draw_text((320 - (34 * FONT_W)) / 2, 180 + YPOS, "[F12]-Setup [F11]-Files [F1]-Help", CL_GREEN, CL_BLACK);

    y_info += 20;
    if (init_fs == FR_OK) {
        tf_card_get_complete_info_string(temp_msg, sizeof(temp_msg));
        draw_text(10 + XPOS, y_info, temp_msg, CL_GREEN, CL_BLACK);
    } else {
        draw_text(10 + XPOS, y_info, "SD card not detected", CL_RED, CL_BLACK);
    }

    #if D_JOY_CLK_PIN != 255
    d_joy_init();
    decode_joy();
    #if !DEVICE_HDMI90_I2S
    if (gpio_get(D_JOY_DATA_PIN)) {
        y_info += 10;
        draw_text_len(10 + XPOS, y_info, "NES Joy present", CL_GREEN, CL_BLACK, 20);
    }
    #endif
    #endif

    mouse[0] = 0x00;
    mouse[1] = 0xff;
    mouse[2] = 0xff;
    mouse[3] = 0xff;
    start_PS2_capture();

    y_info += 10;
    switch (type_psram) {
        case NOT_PSRAM:
            draw_text_len(10 + XPOS, y_info, "PSRAM not found", CL_RED, CL_BLACK, 16);
            #ifdef RP2350_256K
            if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 1) conf.mashine = PENT128;
            #else
            if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 0) conf.mashine = PENT128;
            #endif
            psram_avaiable = 0;
            break;
        case BUTTER_PSRAM:
            snprintf(temp_msg, sizeof temp_msg, "PSRAM %d Mb QSPI CS:%d", size_psram, psram_pin_cs);
            draw_text(10 + XPOS, y_info, temp_msg, CL_GREEN, CL_BLACK);
            psram_type = 0;
            psram_avaiable = 1;
            break;
        case BOARD_PSRAM:
            snprintf(temp_msg, sizeof temp_msg, "PSRAM %d Mb SPI board", size_psram);
            draw_text(10 + XPOS, y_info, temp_msg, CL_GREEN, CL_BLACK);
            psram_type = 1;
            psram_avaiable = 1;
            break;
        case BOARD_PSRAM_NOSUPORT:
            snprintf(temp_msg, sizeof temp_msg, "PSRAM board is not supported");
            draw_text(10 + XPOS, y_info, temp_msg, CL_BLUE, CL_BLACK);
            #ifdef RP2350_256K
            if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 1) conf.mashine = PENT128;
            #else
            if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 0) conf.mashine = PENT128;
            #endif
            psram_avaiable = 0;
            break;
    }

    filterZxMachines(psram_avaiable);
    draw_text(6 + FONT_W, 75 + YPOS, getZxMachineVariant(conf.mashine)->name, CL_GRAY, CL_BLACK);

    #ifndef HDMI_HSTX
    #ifndef GENERAL_SOUND
    draw_text(6 + FONT_W, 85 + YPOS, menu_sound[conf.type_sound], CL_GRAY, CL_BLACK);
    #else
    #ifdef Z_CONTROLER
    draw_text(11 + FONT_W, 85 + YPOS, "TurboSound + Z-Controller SD", CL_GRAY, CL_BLACK);
    #else
    draw_text(11 + FONT_W, 85 + YPOS, "GeneralSound + TurboSound", CL_GRAY, CL_BLACK);
    #endif
    #endif
    #else
    draw_text(11 + FONT_W, 85 + YPOS, "HDMI Audio TurboSound", CL_GRAY, CL_BLACK);
    #endif

    snprintf(temp_msg, sizeof temp_msg, "%s %s", BUILD_DATE, BUILD_TIME);
    draw_text(12 + FONT_W, 110 + YPOS, temp_msg, CL_LT_CYAN, CL_BLACK);

    if (vout_select == VIDEO_VGA) {
        snprintf(temp_msg, sizeof temp_msg, "VGA %dHz", 60);
        draw_text(246 + XPOS, YPOS + 110, temp_msg, CL_LT_CYAN, CL_BLACK);
    }
    if (vout_select == VIDEO_HDMI) {
        #if defined(HDMI_HSTX)
        snprintf(temp_msg, sizeof temp_msg, "HDMI HSTX %dHz", (int)(conf.cpu_freq * 10 / (42 * conf.hdmi_fdiv)));
        draw_text(210 + XPOS, YPOS + 110, temp_msg, CL_BLUE, CL_BLACK);
        #else
        snprintf(temp_msg, sizeof temp_msg, "HDMI %dHz", (int)(conf.cpu_freq * 10 / (42 * conf.hdmi_fdiv)));
        draw_text(240 + XPOS, YPOS + 110, temp_msg, CL_LT_CYAN, CL_BLACK);
        #endif
    }
    if (vout_select == VIDEO_TFT) {
        if (conf.tft == TFT_9345) snprintf(temp_msg, sizeof temp_msg, "ILI9345 TFT");
        if (conf.tft == TFT_9345I) snprintf(temp_msg, sizeof temp_msg, "ILI9345 IPS");
        if (conf.tft == TFT_7789) snprintf(temp_msg, sizeof temp_msg, "ST7789");
        draw_text(228 + XPOS, YPOS + 110, temp_msg, CL_LT_BLUE, CL_BLACK);
    }

    y_info += 10;
    switch (usb_device) {
        case 0: snprintf(temp_msg, sizeof temp_msg, "No USB device"); break;
        case 1: snprintf(temp_msg, sizeof temp_msg, "USB keyboard        "); break;
        case 2: snprintf(temp_msg, sizeof temp_msg, "USB mouse           "); break;
        case 3: snprintf(temp_msg, sizeof temp_msg, "USB keyboard + mouse"); break;
    }
    draw_text(10 + XPOS, y_info, temp_msg, CL_GREEN, CL_BLACK);
    flag_usb_kb = false;

    #if defined GENERAL_SOUND
    draw_text(12 + FONT_W, 100 + YPOS, "Connect PicoBus ....", CL_LT_BLUE, CL_BLACK);
    sleep_ms(1500);
    init_picobus();
    flag_gs = 1;
    sys_GS(GS_INFO);
    tx_buffer[60] = 0;
    draw_text_len(12 + FONT_W, 100 + YPOS, tx_buffer, CL_GREEN, CL_BLACK, 32);
    draw_text(12 + FONT_W, 110 + YPOS, tx_buffer + 32, CL_LT_BLUE, CL_BLACK);
    select_audio();
    #if defined(RTC_NOVA) || defined(RTC_SMUC) || defined(RTC_GLUK)
    rtc_enable = 1;
    #endif
    #endif

    adc_init();
    adc_set_temp_sensor_enabled(true);
}

// ===============================================================
// СООБЩЕНИЯ
// ===============================================================
void Message_Print() {
    wait_msg--;
    switch (msg_bar) {
        #define X_INFO 320 - 48
        #define Y_INFO 240 - 16
    //   case 0:
    //        draw_text((320-(16*FONT_W))/2,Y_INFO,"2026  ZX SPECCY P",CL_LT_CYAN ,CL_BLACK);//232   
    //        break;
        case 1:
        case 2:
            if (conf.type_sound < I2S_AY) {
                sprintf(temp_msg, "VOL %d ", conf.vol_ay);
            } else {
                sprintf(temp_msg, "VOL %d ", conf.vol_i2s);
            }
            draw_text(X_INFO, Y_INFO, temp_msg, CL_GREEN, CL_BLACK);
            break;
            
        case 3:
            sprintf(temp_msg, "NORMAL ");
            draw_text(X_INFO, Y_INFO, temp_msg, CL_GREEN, CL_BLACK);
            break;
            
        case 4:
            sprintf(temp_msg, "TURBO  ");
            draw_text(X_INFO, Y_INFO, temp_msg, CL_GREEN, CL_BLACK);
            break;
            
        case 5:
            sprintf(temp_msg, "FAST  ");
            draw_text(X_INFO, Y_INFO, temp_msg, CL_GREEN, CL_BLACK);
            break;
            
        case 8:
            sprintf(temp_msg, " GAMEPAD XBOX  %4X:%4X                  ", vid, pid);
            draw_text(0, Y_INFO, temp_msg, CL_WHITE, CL_BLACK);
            break;
            
        case 10:
            sprintf(temp_msg, " Read Only! ");
            draw_text(0, Y_INFO, temp_msg, CL_WHITE, CL_RED);
            break;
            
        case 18:
            sprintf(temp_msg, dir_patch_info);
            sprintf(temp_msg, "ERR TR:%2X Sc:%2X       ", WD1793.RealTrack, WD1793.RealSector);
            draw_text(8, Y_INFO, temp_msg, CL_WHITE, CL_RED);
            break;
            
        case 19:
            sprintf(temp_msg, "S:%d ", conf.shift_img);
            draw_text(X_INFO, Y_INFO, temp_msg, CL_LT_BLUE, CL_BLACK);
            break;
            
        case 17:
            draw_text(8, Y_INFO, menu_pallete[conf.pallete], CL_WHITE, CL_BLUE);
            break;
            
        case 77:
            sprintf(temp_msg, dir_patch_info);
            sprintf(temp_msg, "C:%2X H:%2X  R:%2X   ", WD1793.RealTrack, WD1793.Side, WD1793.RealSector);
            draw_text(8, Y_INFO - 10, temp_msg, CL_WHITE, CL_BLUE);
            sprintf(temp_msg, "C:%2X H:%2X  R:%2X   N:%2X  SIZE:%d ", 
                   sector_inf->c, sector_inf->h, sector_inf->r, sector_inf->n, sector_inf->size);
            draw_text(8, Y_INFO, temp_msg, CL_WHITE, CL_BLUE);
            wait_msg = 1000;
            break;
            
        default:
            wait_msg = 0;
            break;
    }
}

// ===============================================================
// КЛАВИАТУРА И ОБРАБОТКА ВВОДА (ФИНАЛЬНАЯ РАБОЧАЯ ВЕРСИЯ)
// ===============================================================
void keyboard_and_other(void) {
    if (wait_msg != 0) Message_Print();
    
    if (conf.joyMode == 0) {
        if ((inx++ % 5) == 0) {
            if (joy_connected) zx_input.kempston = (uint8_t)(data_joy);
            else zx_input.kempston = 0;
        }
    }
    
    // ОПРОС КЛАВИАТУРЫ И ДЖОЙСТИКА
    if ((decode_PS2()) | (decode_key(emu_is_stopped())) | (decode_joy())) {
        // кнопка перехода в меню файлов
        key_menu_state = ((MENU) | (joy_key_ext == 0x84));
        
        // кнопка выхода из меню файлов по [start] joy
        if ((joy_key_ext == 0x80) && emu_is_stopped()) {
            key_menu_state = true;
        }
        
        if (key_menu_state & !old_key_menu_state) {
            data_joy = 0;
            // Переключаем состояние
            if (emu_is_running()) {
                emu_stop();
            } else {
                emu_start();
            }
         //   hardAY_on();// todo
        }
        old_key_menu_state = key_menu_state;
        
        // ВАЖНО: Проверяем состояние ПОСЛЕ обработки нажатия
        // Если мы в меню - показываем файловый менеджер
        if (emu_is_stopped() && !trdos) {
            file_manager();
            TAP_RestorePage();
            return;
        }
        
        // Если эмуляция НЕ запущена и мы не в меню - выходим
        if (!emu_is_running()) return;
        
        // Обработка остальных клавиш ТОЛЬКО если эмуляция запущена
        if (((kb_st_ps2.u[3] & KB_U3_F2) | (joy_key_ext == 0x82))) {
            save_slot();
            TAP_RestorePage();
        }
        if (((kb_st_ps2.u[3] & KB_U3_F3) | (joy_key_ext == 0x81))) {
            load_slot();
            TAP_RestorePage();
        }
        if (kb_st_ps2.u[3] & KB_U3_F5) save_all();
        if (END) disasm();
        if (F10) {
            conf.turbo++;
            turbo_switch();
            msg_bar = 3 + conf.turbo;
            wait_msg = 2000;
        }
        if (((MENU_SETUP) | (joy_key_ext == 0x88))) {
            setup_zx();
        }
        if (F1) help_zx();
        if (F4) help_keyboard();
        if (F9) nmi_zx();
        if (PAUSE) pause_zx();
        
        if (KEY_RESET_PICO) pico_reset();
        
        if (KEY_CTRL_F7) {
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
               kb_st_ps2.u[1] = 0; // Обнуляет ряд клавиш с LCTRL, RCTRL и т.д.
               zx_machine_reset(3);
               im_z80_stop = false;
               is_menu_mode = false;
           }
            }
            // ######################################################
            //  Работа эмулятора
            // ######################################################
            if (is_menu_mode==false)
            { // Emulation mode
                zx_machine_enable_vbuf(true);
                im_z80_stop = false;
                // zx_machine_set_vbuf(g_gbuf);
/*                 if (need_reset_after_menu)
                {
                    zx_machine_reset(3);
                }
 */
                convert_kb_u_to_kb_zx(&kb_st_ps2, zx_input.kb_data);

                joy_scan(); // переопределление kempston joy на клавиши

            } // Emulation mode end
    }
}  
//=========================================================================
// MAIN
int fast(main)(void){  
   
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(50);
    set_sys_clock_khz(252*1000, 0);// стартовая частота pico 252

#ifdef PICO_RP2350 
  // определение RP2350 A или B  
     rp2350a = (*((io_ro_32*)(SYSINFO_BASE + SYSINFO_PACKAGE_SEL_OFFSET)) & 1);
     psram_pin_cs = rp2350a ? PSRAM_BUTTER_PIN_CS : 47;
//--------------------------------------------------------------------------------------------------
//  GPIO 23 // Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
// MODE=0 (PFM — Pulse Frequency Modulation)
// MODE=1 (PWM — Pulse Width Modulation)
#if POWER_MODE != 255
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, POWER_MODE);
#endif
    //--------------------------------------------------------------------------------------------------
    // для корректного запуска с бутербродом PSRAM  GPIO 0,8,19,47 для всех вариантов ))
    gpio_init(psram_pin_cs);
   // gpio_set_dir(psram_pin_cs, GPIO_OUT);// Не загружается если psram_pin_cs притянут физически к +3.3В через R= 10 KOм
    gpio_set_dir(psram_pin_cs, GPIO_IN); //Так работает
    gpio_pull_up(psram_pin_cs); 
    //gpio_disable_pulls(psram_pin_cs); // 
     
#endif

#if LED_BOARD != 255
    gpio_init(LED_BOARD);
    gpio_set_dir(LED_BOARD, GPIO_OUT);
    gpio_put(LED_BOARD, 1);
#endif 

#if defined(RTC_NOVA) || defined(RTC_SMUC) || defined(RTC_GLUK)
    rtc_enable=0;
#endif

    init_fs = disk_initialize(0); // инициализация SD
    DIR fs;
    init_fs = init_filesystem();                // монтирование и инициализация SD
    config_init();                              // загрузка файла конфигурации если он есть "0:/.config/speccy_p.cnf"
    config_ini_load("0:/.config/speccy_p.ini"); // текстовый файл конфига  если его нет то файл записывается

    init_pico(); // инициализация частоты cpu pico и частоты флеш

    pico_fatfs_reboot_spi();      // Переинициализировать SPI
    init_fs = disk_initialize(0); // инициализация SD
    init_fs = init_filesystem();

    init_and_info();
    //-----------------------------------------------------------------
    // если одна плата без GS 
    #ifndef  GENERAL_SOUND     
    select_audio(); // переключение режимов вывода звука 
 	//int hz = 96000;	//44000 //44100 //96000 //22050
	repeating_timer_t timer_audio;
	// negative timeout means exact delay (rather than delay between callbacks)
    // f = 1 / T = 1 / 9 μs = 111111 Гц
    // f = 1 / T = 1 / -21 μs = 47619Гц	-0,79% Гц
 	if (!add_repeating_timer_us(AY_SAMPLE_RATE, AY_timer_callback, NULL, &timer_audio)) // -10  частота ноты До 237Гц  нужно 240,0058 Гц
    {
    return 1;
    }
    #endif

	repeating_timer_t zx_flash_timer;
	if (!add_repeating_timer_us(-1000000 / 2/*Hz*/, zx_flash_callback, NULL, &zx_flash_timer)) {
        return 1;
	}
//---------------------------------------------------------------

multicore_launch_core1(ZXThread);// запуск эмулятора
//sleep_ms(3000);
    disk_autorun ();
    gpio_put(LED_BOARD, 0);
//######################
//   основной цикл
//######################


    while (1)
    {
           keyboard_and_other();
           
           zx_machine_input_set(&zx_input);
      
           led_trdos();// мигаие led 


      } // while(1)
  
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

void file_select_cpm(void) // 
{
	is_menu_mode = true;
	
    is_new_screen = true;
	//     MenuTRDOS(); // меню выбора и подключения образов cpm
	uint8_t Drive = MenuBox_trd(64, 54, 22, 7, "Drive CP/M", 4, 0, 1);
	if (Drive < 5)
	{
		// Копируем строку длиною не более 10 символов из массива src в массив dst1.
		// strncpy (dst1, src,3);
        strncpy(conf.DiskName[Drive], files[cur_file_index], LENF);

        conf.FileAutorunType=NONE;
        file_type[Drive] = CP_M;
        OpenCPMFile(conf.activefilename,Drive);

        write_protected = false; // защита записи отключена для CP/M
	}

	draw_main_window(); // восстановление текста
	draw_file_window();

    g_delay_ms(200);
	last_action = time_us_32();


}
////// CP/M end

//++++++++++++++++++++++++++++++++++++++++++
//================================================================
void config_init(void)
{
    enable_tape = false; // tap файл не подключен при запуске

    FIL f;
    // Создаём каталог .config (если его нет)
     FRESULT fr = f_mkdir("0:/.config");
    if (fr != FR_OK && fr != FR_EXIST) // Error creating .config dir
    {
        config_defain();
        return;
    } 

    sprintf(temp_msg, "0:/.config/speccy_p.cnf");
    int fd = f_open(&f, temp_msg, FA_READ);
    if (fd != FR_OK)
    {
        f_close(&f);
        config_defain();                            // если нет файла конфигурации
    //    config_ini_load("0:/.config/speccy_p.ini"); // текстовый файл конфига
        // если его нет то загружается дефолтная конфигурация и файл записывается
        return;
    }
    UINT bytesRead;
    fd = f_read(&f, &conf, sizeof(conf), &bytesRead); // f_read(&f, *conf ,sizeof(conf) , &bytesRead);
    if (fd != FR_OK)                                  // если ошибка то конфиг по умолчанию */
    {
        f_close(&f);
        config_defain();
        return;
    }
    if (conf.version != CONFIG_VERSION)
    {
        f_close(&f);
        config_defain();                            // если файл конфигурации неправильной версии
    //    config_ini_load("0:/.config/speccy_p.ini"); // текстовый файл конфига
        // если его нет то загружается дефолтная конфигурация и файл записывается
        return;
    }

    f_close(&f);
   // config_ini_load("0:/.config/speccy_p.ini"); // текстовый файл конфига
    conf.turbo = 0;                             // при включении TURBO OFF!
    return;
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

    sprintf(temp_msg, "0:/.config/speccy_p.cnf");//

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

    config_ini_save("0:/.config/speccy_p.ini");//текстовый файл конфига


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
    dis_adres = Z80_PC(cpu_zx);
    bool dis_dump = true;
    if (dis_dump) list_disassm();
    else list_dump();
    
    while (1) {
        if (!decode_key_joy()) continue;
        
        if (dis_dump) {
            if (kb_st_ps2.u[2] & KB_U2_DOWN) {
                kb_st_ps2.u[2] = 0;
                dis_adres += OpcodeLen(dis_adres);
                list_disassm();
            }
            if (kb_st_ps2.u[2] & KB_U2_UP) {
                kb_st_ps2.u[2] = 0;
                dis_adres--;
                list_disassm();
            }
            if (kb_st_ps2.u[2] & KB_U2_RIGHT) {
                kb_st_ps2.u[2] = 0;
                dis_adres += 20;
                list_disassm();
            }
            if (kb_st_ps2.u[2] & KB_U2_LEFT) {
                kb_st_ps2.u[2] = 0;
                dis_adres -= 20;
                list_disassm();
            }
        } else {
            if (kb_st_ps2.u[2] & KB_U2_DOWN) {
                kb_st_ps2.u[2] = 0;
                dis_adres += 8;
                list_dump();
            }
            if (kb_st_ps2.u[2] & KB_U2_UP) {
                kb_st_ps2.u[2] = 0;
                dis_adres -= 8;
                list_dump();
            }
            if (kb_st_ps2.u[2] & KB_U2_RIGHT) {
                kb_st_ps2.u[2] = 0;
                dis_adres += 8 * 20;
                list_dump();
            }
            if (kb_st_ps2.u[2] & KB_U2_LEFT) {
                kb_st_ps2.u[2] = 0;
                dis_adres -= 8 * 20;
                list_dump();
            }
        }
        
        if (kb_st_ps2.u[1] & KB_U1_ENTER) {
            wait_enter();
            draw_rect(248, 20, 50, 190, CL_BLACK, true);
            dis_dump = !dis_dump;
            if (dis_dump) list_disassm();
            else list_dump();
        }
        
        if (kb_st_ps2.u[1] & KB_U1_ESC) {
            wait_esc();
            emu_restore(old_state);
            hardAY_on();
            return;
        }
    }
}

// ===============================================================
// НАСТРОЙКИ
// ===============================================================
void setup_zx(void) {
    EmuState old_state = emu_suspend();
    hardAY_off();
    
    #define w1 290
    #define h1 180
    #define x1 18
    #define y1 20
    
    draw_rect(x1, y1, w1, h1, CL_BLACK, true);
    draw_rect(x1, y1, w1, h1, CL_GRAY, false);
    draw_rect(x1 + 2, y1 + 2, w1 - 4, h1 - 4, CL_GRAY, false);
    draw_rect(x1 + 3, y1 + 3, w1 - 6, 8, CL_GRAY, true);
    draw_text(x1 + 10, y1 + 3, FW_VERSION, CL_BLACK, CL_GRAY);
    
    if (!psram_avaiable) {
        #ifdef RP2350_256K
        if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 1) conf.mashine = PENT128;
        #else
        if (getZxMachineVariant(conf.mashine)->NeedPSRAM != 0) conf.mashine = PENT128;
        #endif
    }
    
    while (1) {
        draw_rect(30, 40, 240, 155, CL_BLACK, true);
        draw_text(x1 + 120, y1 + 20, getZxMachineVariant(conf.mashine)->name, CL_GRAY, CL_BLACK);
        
        #ifdef GENERAL_SOUND
        draw_text(x1 + 126, y1 + 20 + M_SOUND * 10, "GeneralSound + TS", CL_GRAY, CL_BLACK);
        #elifdef HDMI_HSTX
        draw_text(x1 + 126, y1 + 20 + M_SOUND * 10, "HDMI Audio TS", CL_GRAY, CL_BLACK);
        #else
        draw_text(x1 + 120, y1 + 20 + M_SOUND * 10, menu_sound[conf.type_sound], CL_GRAY, CL_BLACK);
        #endif
        
        draw_text(x1 + 120, y1 + 20 + M_TURBO * 10, menu_speed[conf.turbo], CL_GRAY, CL_BLACK);
        draw_text(x1 + 120, y1 + 20 + M_JOY * 10, menu_joy[conf.joyMode], CL_GRAY, CL_BLACK);
        
        if (vout_select != VIDEO_TFT) {
            draw_text(x1 + 120, y1 + 20 + M_PALLETE * 10, menu_pallete[conf.pallete], CL_GRAY, CL_BLACK);
        }
        draw_text(x1 + 120, y1 + 20 + M_AUTORUN * 10, menu_autorun[conf.autorun], CL_GRAY, CL_BLACK);
        
        #ifndef PICO_RP2040
        if (rp2350a) snprintf(temp_msg, sizeof temp_msg, "RP2350A %dMHz %.2fV ", clock_get_hz(clk_sys) / MHZ, table_voltage[conf.voltage] / 100.0);
        else snprintf(temp_msg, sizeof temp_msg, "RP2350B %dMHz", clock_get_hz(clk_sys) / MHZ);
        #else
        snprintf(temp_msg, sizeof temp_msg, "RP2040  %dMHz", CPU_MHZ);
        #endif
        draw_text(138, 150, temp_msg, CL_GRAY, CL_BLACK);
        
        #ifdef PICO_RP2350
        snprintf(temp_msg, sizeof temp_msg, "FLASH %d MHz ", real_flash_freq);
        draw_text(138, 160, temp_msg, CL_GRAY, CL_BLACK);
        #endif
        
        if (psram_avaiable) {
            if (type_psram == 2)
                snprintf(temp_msg, sizeof temp_msg, "Q-PSRAM %dMb %dMHz", size_psram, real_psram_freq);
            else
                snprintf(temp_msg, sizeof temp_msg, "PSRAM   %dMb %dMHz", size_psram, real_psram_freq);
            draw_text(138, 170, temp_msg, CL_GRAY, CL_BLACK);
        }
        
        static uint8_t numsetup = 14;
        if (vout_select == VIDEO_TFT) {
            numsetup = MenuBox_bw(30, 20, 18, 15, menu_setup_tft, 15, numsetup, 1);
        } else {
            numsetup = MenuBox_bw(30, 20, 18, 15, menu_setup, 15, numsetup, 1);
        }
        
        if (numsetup == M_RAM) {
            int count = getZxMachineVariantCount();
            uint8_t x = MenuBox(90, 52, 17, count, "Model & RAM", (char **)getZxMachineNames(), count, 0, 1);
            if (x == 0xff) continue;
            #ifdef NO_GMX
            if (x == 0x05) continue;
            #endif
            conf.mashine = getZxMachineIds()[x];
            init_mashine_and_extram(conf.mashine);
            continue;
        }
        
        #if !defined(GENERAL_SOUND) && !defined(HDMI_HSTX)
        if (numsetup == M_SOUND) {
            uint8_t x = MenuBox(90, 52, 16, 8, "Sound Seting", menu_sound, 8, conf.type_sound, 1);
            if (x == 0xff) continue;
            if (conf.type_sound == x) continue;
            conf.type_sound = x;
            save_config();
            MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);
            pico_reset();
            continue;
        }
        #endif
        
        if (numsetup == M_JOY) {
            uint8_t x = MenuBox(74, 52, 18, 6, "Joystick", menu_joy, 6, conf.joyMode, 1);
            if (x == 0xff) continue;
            conf.joyMode = x;
            joy_redirecting();
            continue;
        }
        
        if (numsetup == M_SOUND_SETUP) {
            uint8_t x = MenuBox_sound_setup(94, 44, 17, 7, "Sound setup", 7, 6, 1);
            if (x == 0xff) continue;
            continue;
        }
        
        if (numsetup == M_ADVANCED) {
            uint8_t x = MenuBox_advanced_setup(94, 44, 17, 9, "Advanced setup", 9, 8, 1);
            if (x == 0xff) continue;
            continue;
        }
        
        if (numsetup == M_TURBO) {
            uint8_t x = MenuBox(94, 42, 17, 2, "Speed Mode", menu_speed, 2, conf.turbo, 1);
            if (x == 0xff) continue;
            conf.turbo = x;
            turbo_switch();
            continue;
        }
        
        if (numsetup == M_AUTORUN) {
            uint8_t x = MenuBox(94, 92, 17, 3, "Auto Run", menu_autorun, 3, conf.autorun, 1);
            if (x == 0xff) continue;
            conf.autorun = x;
            save_config();
            continue;
        }
        
        if (numsetup == M_SAVE_CONFIG) {
            save_config();
            continue;
        }
        
        if (numsetup == M_SOFT_RESET) {
            MessageBox(" ZX SPECTRUM RESET ", "", CL_WHITE, CL_RED, 2);
            zx_machine_reset(3);
            emu_restore(old_state);
            return;
        }
        
        if (numsetup == M_HARD_RESET) {
            MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);
            pico_reset();
            return;
        }
        
        if (numsetup == M_POWER_OFF) {
            save_all();
            hardAY_off();
            emu_stop();
            MessageBox_off("     The current configuration saved   ", 
                          "     the computer can be powered off ", CL_WHITE, CL_BLUE, 0);
            while (1) {
                draw_img(0, 0);
            }
        }
        
        if (numsetup == M_UPDATE) {
            emu_stop();
            hardAY_off();
            draw_img(0, 0);
            MessageBox("  switching to firmware update mode ", "", CL_WHITE, CL_RED, 3);
            sleep_ms(256);
            reset_usb_boot(0, 0);
        }
        
        if (numsetup == M_EXIT || numsetup == 0xff) {
            if (numsetup == 0xff) numsetup = 13;
            emu_restore(old_state);
            hardAY_on();
            return;
        }
        
        if (vout_select == VIDEO_TFT && numsetup == M_TFT_BRIGHT) {
            while (1) {
                static uint8_t x = 0;
                x = MenuBox_tft_setup(90, 44, 22, 11, "Setting TFT", submenu_setup_tft, 7, x, 1);
                if (x == 0xff) { x = 0; goto L_EXIT_TFT; }
                
                switch (x) {
                    case 0: conf.tft = TFT_9345; break;
                    case 1: conf.tft = TFT_9345I; break;
                    case 2: conf.tft = TFT_7789; break;
                    case 3: conf.tft_rotate = (conf.tft_rotate == 0) ? 1 : 0; break;
                    case 4: conf.tft_invert = (conf.tft_invert == 0) ? 1 : 0; break;
                    case 5: conf.tft_rgb = (conf.tft_rgb == 0) ? 1 : 0; break;
                    case 6: save_config(); pico_reset(); break;
                    default: break;
                }
            }
            L_EXIT_TFT:;
        }
        
        if (vout_select != VIDEO_TFT && numsetup == M_PALLETE) {
            for (int i = 0; i < 8; i++) {
                draw_rect(i * 40, 230 - 24, 40, 12, i, true);
            }
            for (int i = 8; i < 16; i++) {
                draw_rect((i - 8) * 40, 230 - 12, 40, 12, i, true);
            }
            
            uint8_t x = MenuBox(90, 44, 16, 12, "Pallete", menu_pallete, 12, conf.pallete, 1);
            if (x == 0xff) continue;
            conf.pallete = x;
            set_palette(conf.pallete);
            continue;
        }
    }
}

// ===============================================================
// СЛОТЫ (СОХРАНЕНИЕ/ЗАГРУЗКА)
// ===============================================================
void slot_screen(uint8_t cPos) {
    ScreenShot_Y = 18;
    sprintf(conf.activefilename, "0:/save/%d_slot.Z80", cPos);
    if (!LoadScreenFromZ80Snapshot(conf.activefilename)) {
        draw_rect(95, 18, 217, 210, CL_BLACK, true);
        draw_text(170, 77, "EMPTY SLOT", CL_GRAY, CL_BLACK);
    }
    ScreenShot_Y = 40;
}

uint8_t MenuBox_sv(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text, uint8_t Pos, uint8_t cPos, uint8_t over_emul) {
    if (over_emul) zx_machine_enable_vbuf(false);
    
    uint16_t lFrame = (lPos * 8) + 10;
    uint16_t hFrame = ((1 + hPos) * 8) + 20;
    
    draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_BLACK, false);
    draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 3, CL_GRAY, false);
    draw_rect(xPos, yPos, lFrame, hFrame, CL_BLACK, true);
    draw_rect(xPos, yPos, lFrame, 9, CL_GRAY, true);
    draw_text(xPos + 10, yPos + 0, text, CL_PAPER, CL_INK);
    
    draw_line(100 - 6, 15, 100 - 6, 236, CL_INK);
    yPos = yPos + 10;
    
    for (uint8_t i = 0; i < Pos; i++) {
        if (i == cPos) {
            sprintf(temp_msg, " %d.Slot ", i);
            draw_text(xPos + 1, yPos + 8 + 8 * i, temp_msg, CL_PAPER, CL_LT_CYAN);
        } else {
            sprintf(temp_msg, " %d.Slot ", i);
            draw_text(xPos + 1, yPos + 8 + 8 * i, temp_msg, CL_INK, CL_PAPER);
        }
    }
    
    kb_st_ps2.u[0] = 0x0;
    kb_st_ps2.u[1] = 0x0;
    kb_st_ps2.u[2] = 0x0;
    kb_st_ps2.u[3] = 0x0;
    slot_screen(cPos);
    
    while (1) {
        if (!decode_key_joy()) continue;
        
        if (kb_st_ps2.u[2] & KB_U2_DOWN) {
            kb_st_ps2.u[2] = 0;
            sprintf(temp_msg, " %d.Slot ", cPos);
            draw_text(xPos + 1, yPos + 8 + 8 * cPos, temp_msg, CL_INK, CL_BLACK);
            cPos++;
            if (cPos == Pos) cPos = 0;
            sprintf(temp_msg, " %d.Slot ", cPos);
            draw_text(xPos + 1, yPos + 8 + 8 * cPos, temp_msg, CL_BLACK, CL_LT_CYAN);
            slot_screen(cPos);
        }
        
        if (kb_st_ps2.u[2] & KB_U2_UP) {
            kb_st_ps2.u[2] = 0;
            sprintf(temp_msg, " %d.Slot ", cPos);
            draw_text(xPos + 1, yPos + 8 + 8 * cPos, temp_msg, CL_INK, CL_BLACK);
            if (cPos == 0) cPos = Pos;
            cPos--;
            sprintf(temp_msg, " %d.Slot ", cPos);
            draw_text(xPos + 1, yPos + 8 + 8 * cPos, temp_msg, CL_BLACK, CL_LT_CYAN);
            slot_screen(cPos);
        }
        
        if (kb_st_ps2.u[1] & KB_U1_ENTER) {
            wait_enter();
            return cPos;
        }
        
        if (kb_st_ps2.u[1] & KB_U1_ESC) {
            wait_esc();
            return 0xff;
        }
    }
}

void save_slot(void) {
    EmuState old_state = emu_suspend();
    hardAY_off();
    
    uint8_t num = MenuBox_sv(38, 7, 33, 25, "SAVE", 25, 0, 1);
    if (num == 0xff) {
        emu_restore(old_state);
        hardAY_on();
        return;
    }
    
    sprintf(save_file_name_image, "0:/save/%d_slot.Z80", num);
    sleep_ms(10);
    
    int fd = f_mkdir("0:/save");
    if ((fd != FR_OK) && (fd != FR_EXIST)) {
        MessageBox(" Error saving ", "", CL_LT_YELLOW, CL_RED, 2);
    } else {
        hardAY_on();
        MessageBox(" Quick saving... ", "", CL_WHITE, CL_BLUE, 0);
        save_image_z80(save_file_name_image);
        if (num == 0) save_config();
    }
    
    emu_restore(old_state);
    sleep_ms(3000);
}

void save_all(void) {
    EmuState old_state = emu_suspend();
    hardAY_off();
    
    sprintf(save_file_name_image, "0:/save/0_slot.Z80");
    int fd = f_mkdir("0:/save");
    
    if ((fd != FR_OK) && (fd != FR_EXIST)) {
        MessageBox(" Error saving ", "", CL_LT_YELLOW, CL_RED, 2);
    } else {
        hardAY_on();
        MessageBox(" SAVE ALL RAM & CONFIG ", "", CL_WHITE, CL_BLUE, 0);
        save_image_z80(save_file_name_image);
        save_config();
    }
    
    emu_restore(old_state);
}

void load_slot(void) {
    EmuState old_state = emu_suspend();
    im_ready_loading = false;
    hardAY_off();
    
    uint8_t num = MenuBox_sv(38, 7, 33, 25, "LOAD", 25, 0, 1);
    if (num == 0xff) {
        emu_restore(old_state);
        hardAY_on();
        return;
    }
    
    zx_machine_reset(3);
    sprintf(save_file_name_image, "0:/save/%d_slot.Z80", num);
    
    if (load_image_z80(save_file_name_image)) {
        MessageBox("Loading slot...", temp_msg, CL_WHITE, CL_BLUE, 2);
    } else {
        MessageBox(" Error loading ", "", CL_LT_YELLOW, CL_RED, 2);
    }
    
    if (num == 0) config_init();
    emu_restore(old_state);
}

// ===============================================================
// AUTORUN MOUNT IMAGE Z80
// ===============================================================
void mount_image_Z80(void) {
    EmuState old_state = emu_suspend();
    im_ready_loading = false;
    AY_reset();
    
    sprintf(save_file_name_image, "0:/save/0_slot.Z80");
    sleep_ms(2000);
    load_image_z80(save_file_name_image);
    
    emu_restore(old_state);
    zx_machine_enable_vbuf(true);
}

void mount_disk_image(void) {
    if (conf.FileAutorunType == SCL) {
        strcpy(conf.activefilename, conf.Disks[0]);
        file_type[0] = SCL;
        Run_file_scl(conf.activefilename, 0);
    }
    if (conf.FileAutorunType == TRD) {
        file_type[0] = TRD;
        strcpy(conf.activefilename, conf.Disks[0]);
        OpenTRDFile(conf.activefilename, 0);
    }
    if (conf.FileAutorunType == FDI) {
        file_type[0] = FDI;
        strncpy(conf.DiskName[0], files[cur_file_index], LENF);
        OpenFDI_File(conf.activefilename, 0);
        write_protected = true;
    }
}

// ===============================================================
// AUTORUN DISK
// ===============================================================
void disk_autorun(void)
{
    switch (conf.autorun)
    {
    case 0:
        if (conf.Disks[0][0] != 0)
            mount_disk_image();
        ; // Если диск есть то монтируем его но не запускаем
        break;
    case 1:
        mount_disk_image();
        emu_start();
        zx_machine_enable_vbuf(true);
        break;
    case 2:
        mount_image_Z80();
        break;
    }
    im_ready_loading = false;
    AY_reset();
}

// ===============================================================
// ФАЙЛОВЫЙ МЕНЕДЖЕР
// ===============================================================
void file_manager(void) {
    strncpy(files[0], "..", LENF1);
    hardAY_off();
    
    if (init_fs != FR_OK) {
        g_delay_ms(10);
        init_fs = init_filesystem();
        N_files = read_select_dir(cur_dir_index);
        if (N_files == 0) init_fs = FR_NO_FILE;
    }
    
    if (emu_just_stopped()) {
        if (init_fs != FR_OK) {
            memset(g_gbuf, COLOR_BACKGOUND, sizeof(g_gbuf));
            MessageBox("SD Card not found!!!", "    Please REBOOT   ", CL_LT_YELLOW, CL_RED, 0);
            return;
        }
        draw_main_window();
        draw_file_window();
        prev_emu_state = EMU_STOPPED;
        
        if (init_fs == FR_OK) {
            N_files = read_select_dir(cur_dir_index);
            if (N_files == 0) {
                init_fs = FR_NO_FILE;
            } else {
                cur_file_index_old = -1;
            }
        }
    }
    
    if (init_fs == FR_OK) {
        if ((ESC_EXIT)) {
            emu_start();
          //  hardAY_on();// todo
            return;
        }
        
        if (((KEY_SPACE) | JOY_B) && (init_fs == FR_OK)) {
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            
            auto_start_filename[0] = 0;
            strcat(auto_start_filename, files[cur_file_index]);
            
            const char *ext = get_file_extension(conf.activefilename);
            
            if (strcasecmp(ext, "trd") == 0) {
                MessageBox(" RUNING TRD FILE ", "", CL_WHITE, CL_BLUE, 4);
                file_type[0] = TRD;
                conf.FileAutorunType = TRD;
                strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                OpenTRDFile(conf.activefilename, 0);
                write_protected = false;
                zx_machine_reset(1);
                emu_start();
                return;
            }
            
            if (strcasecmp(ext, "scl") == 0) {
                MessageBox(" RUNING SCL FILE ", "", CL_WHITE, CL_BLUE, 4);
                write_protected = true;
                conf.FileAutorunType = SCL;
                file_type[0] = SCL;
                strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                Run_file_scl(conf.activefilename, 0);
                zx_machine_reset(1);
                emu_start();
                return;
            }
            
            if (strcasecmp(ext, "fdi") == 0) {
                MessageBox(" RUNING FDI FILE ", "", CL_WHITE, CL_BLUE, 4);
                conf.FileAutorunType = FDI;
                file_type[0] = FDI;
                strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                OpenFDI_File(conf.activefilename, 0);
                zx_machine_reset(1);
                emu_start();
                return;
            }
            return;
        }
        
        if (((KEY_ENTER) | JOY_A) && (init_fs == FR_OK)) {
            flag_usb_kb = false;
            
            if (files[cur_file_index][LENF1]) {
                if (cur_file_index == 0) {
                    if (cur_dir_index) {
                        cur_dir_index--;
                        N_files = read_select_dir(cur_dir_index);
                        cur_file_index = 0;
                        return;
                    }
                }
                if (cur_dir_index < (DIRS_DEPTH - 2)) {
                    cur_dir_index++;
                    strncpy(dirs[cur_dir_index], files[cur_file_index], LENF1);
                    N_files = read_select_dir(cur_dir_index);
                    cur_file_index = 0;
                    cur_file_index_old = cur_file_index;
                    shift_file_index = 0;
                    last_action = time_us_32();
                    return;
                }
            } else {
                strcpy(conf.activefilename, dir_patch);
                strcat(conf.activefilename, "/");
                strcat(conf.activefilename, files[cur_file_index]);
                
                auto_start_filename[0] = 0;
                strcat(auto_start_filename, files[cur_file_index]);
                const char *ext = get_file_extension(conf.activefilename);
                
                if (strcasecmp(ext, "z80") == 0) {
                    EmuState old_state = emu_suspend();
                    while (emu_is_stopped()) {
                        sleep_ms(10);
                        if (im_ready_loading) {
                            conf.turbo = 0;
                            turbo_switch();
                            zx_machine_reset(3);
                            if (load_image_z80(conf.activefilename)) {
                                memset(temp_msg, 0, sizeof(temp_msg));
                                sprintf(temp_msg, " Loading file:%s", auto_start_filename);
                                MessageBox("Z80", temp_msg, CL_WHITE, CL_BLUE, 2);
                                conf.activefilename[0] = 0;
                                emu_restore(old_state);
                                im_ready_loading = false;
                                break;
                            } else {
                                MessageBox("Error loading snapshot!!!", auto_start_filename, CL_YELLOW, CL_LT_RED, 1);
                                last_action = time_us_32();
                                draw_file_window();
                                emu_restore(old_state);
                                im_ready_loading = false;
                                break;
                            }
                        }
                    }
                    return;
                }
                
                if (strcasecmp(ext, "sna") == 0) {
                    EmuState old_state = emu_suspend();
                    while (emu_is_stopped()) {
                        sleep_ms(10);
                        if (im_ready_loading) {
                            zx_machine_reset(3);
                            if (load_image_sna(conf.activefilename)) {
                                memset(temp_msg, 0, sizeof(temp_msg));
                                sprintf(temp_msg, " Loading file:%s", auto_start_filename);
                                MessageBox("SNA", temp_msg, CL_WHITE, CL_BLUE, 2);
                                conf.activefilename[0] = 0;
                                emu_restore(old_state);
                                im_ready_loading = false;
                                break;
                            } else {
                                MessageBox("Error loading snapshot!!!", auto_start_filename, CL_YELLOW, CL_LT_RED, 1);
                                last_action = time_us_32();
                                draw_file_window();
                                emu_restore(old_state);
                                im_ready_loading = false;
                                break;
                            }
                        }
                    }
                    return;
                }
                
                if (strcasecmp(ext, "scr") == 0) {
                    if (LoadScreenshot(conf.activefilename, true)) {
                        emu_start();
                        return;
                    } else {
                        MessageBox("Error loading screen!!!", auto_start_filename, CL_YELLOW, CL_LT_RED, 1);
                    }
                }
                
                if (strcasecmp(ext, "tap") == 0) {
                    Set_load_tape(conf.activefilename, current_lfn);
                    strcpy(temp_msg, current_lfn);
                    MessageBox("    TAPE    ", temp_msg, CL_WHITE, CL_BLUE, 4);
                    emu_start();
                  //  hardAY_on(); // todo
                    return;
                }
                
                if (strcasecmp(ext, "trd") == 0) {
                    file_select_trdos();
                    return;
                }
                
                if (strcasecmp(ext, "cpm") == 0) {
                    file_select_cpm();
                    return;
                }
                
                if (strcasecmp(ext, "scl") == 0) {
                    MessageBox("SCL files are mounted", "   only on Drive A:", CL_WHITE, CL_BLUE, 4);
                    conf.FileAutorunType = SCL;
                    file_type[0] = SCL;
                    strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                    Run_file_scl(conf.activefilename, 0);
                    draw_main_window();
                    draw_file_window();
                    last_action = time_us_32();
                    return;
                }
                
                if (strcasecmp(ext, "fdi") == 0) {
                    MessageBox("FDI files are mounted", "   only on Drive A:", CL_WHITE, CL_BLUE, 4);
                    conf.FileAutorunType = FDI;
                    file_type[0] = FDI;
                    strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                    OpenFDI_File(conf.activefilename, 0);
                    draw_main_window();
                    draw_file_window();
                    last_action = time_us_32();
                    return;
                }
            }
        }
        
        int num_show_files = 18;
        
        if (((kb_st_ps2.u[2] & KB_U2_DOWN) | JOY_DOWN) && (cur_file_index < N_files)) {
            cur_file_index++;
            last_action = time_us_32();
        }
        if (((kb_st_ps2.u[2] & KB_U2_UP) | JOY_UP) && (cur_file_index > 0)) {
            cur_file_index--;
            last_action = time_us_32();
        }
        if ((kb_st_ps2.u[2] & KB_U2_LEFT)) {
            cur_file_index = 0;
            shift_file_index = 0;
            last_action = time_us_32();
        }
        if ((kb_st_ps2.u[2] & KB_U2_RIGHT)) {
            cur_file_index = N_files;
            shift_file_index = (N_files >= num_show_files) ? N_files - num_show_files : 0;
            last_action = time_us_32();
        }
        if (((kb_st_ps2.u[2] & KB_U2_PAGE_DOWN) | JOY_RIGHT) && (cur_file_index < N_files)) {
            cur_file_index += num_show_files;
            last_action = time_us_32();
        }
        if (((kb_st_ps2.u[2] & KB_U2_PAGE_UP) | JOY_LEFT) && (cur_file_index > 0)) {
            cur_file_index -= num_show_files;
            last_action = time_us_32();
        }
        
        if ((kb_st_ps2.u[1] & KB_U1_BACK_SPACE) | (data_joy == 0x40)) {
            if (cur_dir_index == 0) {
                if (cur_file_index == 0) cur_file_index = 1;
                if (shift_file_index == 0) shift_file_index = 1;
                read_select_dir(cur_dir_index);
            } else {
                cur_dir_index--;
                N_files = read_select_dir(cur_dir_index);
                cur_file_index = 0;
                draw_text_len(2 + FONT_W, FONT_H - 1, "                    ", COLOR_BACKGOUND, COLOR_BORDER, 20);
                cur_file_index = 0;
                shift_file_index = 0;
            }
        }
        
        if (cur_file_index < 0) cur_file_index = 0;
        if (cur_file_index >= N_files) cur_file_index = N_files;
        
        if (data_joy > 0) old_data_joy = 0;
        
        for (int i = num_show_files; i--;) {
            if ((cur_file_index - shift_file_index) >= (num_show_files)) shift_file_index++;
            if ((cur_file_index - shift_file_index) < 0) shift_file_index--;
        }
        
        if (cur_dir_index == 0) {
            if (cur_file_index == 0) cur_file_index = 1;
            if (shift_file_index == 0) shift_file_index = 1;
        }
        
        if (strlen(dir_patch) > 0) {
            draw_text_len(FONT_W + 3, FONT_H - 1, dir_patch + 1, COLOR_UP, COLOR_BORDER, 51);
        } else {
            draw_text_len(FONT_W + 3, FONT_H - 1, "                                                  ", CL_TEST, COLOR_BORDER, 51);
        }
        
        for (int i = 0; i < num_show_files; i++) {
            uint8_t color_text = CL_GREEN;
            uint8_t color_text_d = CL_YELLOW;
            uint8_t color_bg = COLOR_BACKGOUND;
            
            if (i == (cur_file_index - shift_file_index)) {
                color_text = CL_BLACK;
                color_bg = COLOR_SELECT;
                color_text_d = CL_BLACK;
            }
            
            if ((i > N_files) || ((cur_dir_index == 0) && (i > (N_files - 1)))) {
                draw_text_file(4 + FONT_W, 2 * FONT_H + i * FONT_H, " ", color_text, color_bg, NUMBER_CHAR);
                continue;
            }
            
            if (files[i + shift_file_index][LENF1]) {
                draw_text_file(4 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text_d, color_bg, NUMBER_CHAR);
            } else {
                draw_text_file(4 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text, color_bg, NUMBER_CHAR);
            }
        }
        
        strcpy(current_lfn, get_current_altname(dir_patch, files[cur_file_index]));
        
        if (cur_file_index) {
            strncpy(temp_msg, get_lfn_from_dir(dir_patch, files[cur_file_index]), 72);
            draw_text_len(12 + FONT_W * 14, 18, temp_msg, CL_INK, COLOR_BACKGOUND, 35);
            for (size_t i = 0; i < 36; i++) {
                temp_msg[i] = temp_msg[i + 35];
            }
            draw_text_len(12 + FONT_W * 14, 28, temp_msg, CL_INK, COLOR_BACKGOUND, 35);
        } else {
            draw_rect(FONT_W * 16, 18, FONT_W * 36, 20, COLOR_BACKGOUND, true);
        }
        
        if ((cur_file_index > 0) && (cur_file_index_old == -1)) {
            last_action = time_us_32();
        }
    }
    file_info();
}

// ===============================================================
// ИНФОРМАЦИЯ О ФАЙЛАХ
// ===============================================================
void file_info(void) {
    if (emu_is_stopped() && init_fs == FR_OK) {
        last_action = 0;
        const char *ext = get_file_extension(files[cur_file_index]);
        
        if (strcasecmp(ext, "trd") == 0) {
            strncpy(temp_msg, current_lfn, 22);
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            cur_file_index_old = cur_file_index;
            ReadCatalog(conf.activefilename, current_lfn, false);
            return;
        }
        
        if (strcasecmp(ext, "cpm") == 0) {
            strncpy(temp_msg, current_lfn, 22);
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            cur_file_index_old = cur_file_index;
            ReadCPMDir(conf.activefilename, current_lfn, false);
            return;
        }
        
        if (strcasecmp(ext, "fdi") == 0) {
            strncpy(temp_msg, current_lfn, 22);
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            cur_file_index_old = cur_file_index;
            Read_Info_FDI(conf.activefilename, current_lfn, false);
            return;
        }
        
        if (strcasecmp(ext, "scl") == 0) {
            strncpy(temp_msg, current_lfn, 22);
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            cur_file_index_old = cur_file_index;
            ReadCatalog_scl(conf.activefilename, current_lfn, false);
            return;
        }
        
        if (strcasecmp(ext, "z80") == 0) {
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            cur_file_index_old = cur_file_index;
            if (!LoadScreenFromZ80Snapshot(conf.activefilename)) {
                CLEAR_INFO;
            }
            return;
        }
        
        if (strcasecmp(ext, "scr") == 0) {
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            CLEAR_INFO;
            cur_file_index_old = cur_file_index;
            if (!LoadScreenshot(conf.activefilename, false)) {
                CLEAR_INFO;
            }
            return;
        }
        
        if (strcasecmp(ext, "tap") == 0) {
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            CLEAR_INFO;
            cur_file_index_old = cur_file_index;
            if (!LoadScreenFromTap(conf.activefilename)) {
                CLEAR_INFO;
            }
            return;
        }
        
        if (strcasecmp(ext, "sna") == 0) {
            strcpy(conf.activefilename, dir_patch);
            strcat(conf.activefilename, "/");
            strcat(conf.activefilename, files[cur_file_index]);
            CLEAR_INFO;
            cur_file_index_old = cur_file_index;
            if (!LoadScreenFromSNASnapshot(conf.activefilename)) {
                CLEAR_INFO;
            }
            return;
        }
        
        if (cur_file_index_old == -1) CLEAR_INFO;
        else CLEAR_INFO;
        cur_file_index_old = cur_file_index;
    }
}

// ===============================================================
// ФАЙЛЫ TRD И CPM
// ===============================================================
void file_select_trdos(void) {
    emu_stop();
    
    uint8_t Drive = MenuBox_trd(64, 54, 22, 7, "Drive TR-DOS", 4, 0, 1);
    if (Drive < 5) {
        strncpy(conf.DiskName[Drive], files[cur_file_index], LENF);
        if (Drive == 0) conf.FileAutorunType = TRD;
        file_type[Drive] = TRD;
        OpenTRDFile(conf.activefilename, Drive);
        write_protected = false;
    }
    
    draw_main_window();
    draw_file_window();
    g_delay_ms(200);
    last_action = time_us_32();
}

void file_select_cpm(void) {
    emu_stop();
    
    uint8_t Drive = MenuBox_trd(64, 54, 22, 7, "Drive CP/M", 4, 0, 1);
    if (Drive < 5) {
        strncpy(conf.DiskName[Drive], files[cur_file_index], LENF);
        conf.FileAutorunType = NONE;
        file_type[Drive] = CP_M;
        OpenCPMFile(conf.activefilename, Drive);
        write_protected = false;
    }
    
    draw_main_window();
    draw_file_window();
    g_delay_ms(200);
    last_action = time_us_32();
}

// ===============================================================
// КОНФИГУРАЦИЯ
// ===============================================================
void config_init(void) {
    enable_tape = false;
    FIL f;
    
    FRESULT fr = f_mkdir("0:/.config");
    if (fr != FR_OK && fr != FR_EXIST) {
        config_defain();
        return;
    }
    
    sprintf(temp_msg, "0:/.config/speccy_p.cnf");
    int fd = f_open(&f, temp_msg, FA_READ);
    if (fd != FR_OK) {
        f_close(&f);
        config_defain();
        return;
    }
    
    UINT bytesRead;
    fd = f_read(&f, &conf, sizeof(conf), &bytesRead);
    if (fd != FR_OK) {
        f_close(&f);
        config_defain();
        return;
    }
    
    if (conf.version != CONFIG_VERSION) {
        f_close(&f);
        config_defain();
        return;
    }
    
    f_close(&f);
    conf.turbo = 0;
}

bool save_config(void) {
    FIL f;
    MessageBox("       Saving config      ", "", CL_WHITE, CL_BLUE, 2);
    
    sprintf(temp_msg, "0:/.config/speccy_p.cnf");
    int fd = f_open(&f, temp_msg, FA_CREATE_ALWAYS | FA_WRITE);
    if (fd != FR_OK) {
        MessageBox("      Error saving config      ", "", CL_LT_YELLOW, CL_RED, 1);
        f_close(&f);
        return false;
    }
    
    UINT bytesWritten;
    fd = f_write(&f, &conf, sizeof(conf), &bytesWritten);
    if (bytesWritten != sizeof(conf)) {
        f_close(&f);
        return false;
    }
    
    f_close(&f);
    config_ini_save("0:/.config/speccy_p.ini");
    return true;
}

// ===============================================================
// PSRAM
// ===============================================================
volatile uint8_t *PSRAM_DATA = (uint8_t*)0x11000000;

#if defined(PICO_RP2350)
#define MB16 (16ul << 20)
#define MB8 (8ul << 20)
#define MB4 (4ul << 20)
#define MB1 (1ul << 20)

static int BUTTER_PSRAM_SIZE = 0;

uint32_t __not_in_flash_func(get_psram_size)() {
    for (register int i = MB8; i < MB16; i += 4096) PSRAM_DATA[i] = 16;
    for (register int i = MB4; i < MB8; i += 4096) PSRAM_DATA[i] = 8;
    for (register int i = MB1; i < MB4; i += 4096) PSRAM_DATA[i] = 4;
    for (register int i = 0; i < MB1; i += 4096) PSRAM_DATA[i] = 1;
    
    register uint32_t res = PSRAM_DATA[MB16 - 4096];
    for (register int i = MB16 - MB1; i < MB16; i += 4096) {
        if (res != PSRAM_DATA[i]) return 0;
    }
    BUTTER_PSRAM_SIZE = res;
    return BUTTER_PSRAM_SIZE;
}

static bool __no_inline_not_in_flash_func(psram_detect)(void) {
    const uint8_t CMD_EXIT_QPI = 0xF5;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) |
                        CMD_EXIT_QPI;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    (void)qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    for (volatile int d = 0; d < 64; ++d);
    
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    uint8_t mfid = 0, kgd = 0;
    
    for (int i = 0; i < 6; ++i) {
        qmi_hw->direct_tx = (i == 0) ? 0x9F : 0xFF;
        while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS));
        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
        uint8_t b = (uint8_t)qmi_hw->direct_rx;
        if (i == 4) mfid = b;
        else if (i == 5) kgd = b;
    }
    
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    if (kgd == 0x5D || mfid == 0x5D || mfid == 0x0D) {
        return true;
    }
    
    for (volatile int d = 0; d < 256; ++d);
    
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    mfid = 0;
    kgd = 0;
    
    for (int i = 0; i < 6; ++i) {
        qmi_hw->direct_tx = (i == 0) ? 0x9F : 0xFF;
        while (!(qmi_hw->direct_csr & QMI_DIRECT_CSR_TXEMPTY_BITS));
        while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
        uint8_t b = (uint8_t)qmi_hw->direct_rx;
        if (i == 4) mfid = b;
        else if (i == 5) kgd = b;
    }
    
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    return (kgd == 0x5D || mfid == 0x5D || mfid == 0x0D);
}

void __no_inline_not_in_flash_func(init_psram_butter)(uint cs_pin) {
    const uint32_t ints = save_and_disable_interrupts();
    
    gpio_set_function(cs_pin, GPIO_FUNC_XIP_CS1);
    
    qmi_hw->direct_csr = 30 << QMI_DIRECT_CSR_CLKDIV_LSB | 
                         QMI_DIRECT_CSR_EN_BITS |
                         QMI_DIRECT_CSR_AUTO_CS1N_BITS;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    
    if (!psram_detect()) {
        qmi_hw->direct_csr = 0;
        restore_interrupts(ints);
        type_psram = NOT_PSRAM;
        psram_avaiable = 0;
        return;
    }
    
    const uint8_t CMD_EXIT_QPI = 0xF5;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_OE_BITS |
                        (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) |
                        CMD_EXIT_QPI;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    (void)qmi_hw->direct_rx;
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    for (volatile int d = 0; d < 64; ++d);
    
    const uint8_t CMD_QPI_EN = 0x35;
    qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | CMD_QPI_EN;
    while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
    qmi_hw->direct_csr &= ~QMI_DIRECT_CSR_ASSERT_CS1N_BITS;
    
    qmi_hw->direct_csr = 0;
    restore_interrupts(ints);
    
    const int max_psram_freq = PSRAM_MAX_FREQ_MHZ * 1000000;
    const int clock_hz = clock_get_hz(clk_sys);
    
    int divisor = (clock_hz + max_psram_freq - 1) / max_psram_freq;
    if (divisor == 1 && clock_hz > 100000000) {
        divisor = 2;
    }
    real_psram_freq = clock_hz / divisor / 1000000;
    
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000) {
        rxdelay += 1;
    }
    
    const int clock_period_fs = 1000000000000000ll / clock_hz;
    const int max_select = (125 * 1000000) / clock_period_fs;
    const int min_deselect = (18 * 1000000 + (clock_period_fs - 1)) / clock_period_fs - (divisor + 1) / 2;
    
    qmi_hw->m[1].timing = 1 << QMI_M1_TIMING_COOLDOWN_LSB |
                          QMI_M1_TIMING_PAGEBREAK_VALUE_1024 << QMI_M1_TIMING_PAGEBREAK_LSB |
                          max_select << QMI_M1_TIMING_MAX_SELECT_LSB |
                          min_deselect << QMI_M1_TIMING_MIN_DESELECT_LSB |
                          rxdelay << QMI_M1_TIMING_RXDELAY_LSB |
                          divisor << QMI_M1_TIMING_CLKDIV_LSB;
    
    qmi_hw->m[1].rfmt = QMI_M0_RFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_PREFIX_WIDTH_LSB |
                        QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_RFMT_ADDR_WIDTH_LSB |
                        QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB |
                        QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_RFMT_DUMMY_WIDTH_LSB |
                        QMI_M0_RFMT_DATA_WIDTH_VALUE_Q << QMI_M0_RFMT_DATA_WIDTH_LSB |
                        QMI_M0_RFMT_PREFIX_LEN_VALUE_8 << QMI_M0_RFMT_PREFIX_LEN_LSB |
                        6 << QMI_M0_RFMT_DUMMY_LEN_LSB;
    
    qmi_hw->m[1].rcmd = 0xEB;
    
    qmi_hw->m[1].wfmt = QMI_M0_WFMT_PREFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_PREFIX_WIDTH_LSB |
                        QMI_M0_WFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_WFMT_ADDR_WIDTH_LSB |
                        QMI_M0_WFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_WFMT_SUFFIX_WIDTH_LSB |
                        QMI_M0_WFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_WFMT_DUMMY_WIDTH_LSB |
                        QMI_M0_WFMT_DATA_WIDTH_VALUE_Q << QMI_M0_WFMT_DATA_WIDTH_LSB |
                        QMI_M0_WFMT_PREFIX_LEN_VALUE_8 << QMI_M0_WFMT_PREFIX_LEN_LSB;
    
    qmi_hw->m[1].wcmd = 0x38;
    hw_set_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
    
    type_psram = BUTTER_PSRAM;
    psram_avaiable = 1;
    size_psram = get_psram_size();
}

void __no_inline_not_in_flash_func(deinit_psram_butter)(uint cs_pin) {
    hw_clear_bits(&xip_ctrl_hw->ctrl, XIP_CTRL_WRITABLE_M1_BITS);
    qmi_hw->direct_csr = 0;
    gpio_set_function(cs_pin, GPIO_FUNC_SIO);
    gpio_set_dir(cs_pin, GPIO_IN);
}
#endif

// ===============================================================
// ИНИЦИАЛИЗАЦИЯ PSRAM (ВСЕ ВЕРСИИ)
// ===============================================================
void fast(init_psram_board_all_version)(void) {
    #ifdef NO_PSRAM
    psram_type = NOT_PSRAM;
    #endif
    
    #ifdef PSRAM_BUTTER_OR_PSRAM_PSRAM_BOARD
    init_psram_butter(psram_pin_cs);
    size_psram = get_psram_size();
    if (size_psram != 0) {
        psram_avaiable = 1;
        type_psram = BUTTER_PSRAM;
        return;
    }
    deinit_psram_butter(psram_pin_cs);
    size_psram = init_psram_board();
    if (size_psram == 0) {
        type_psram = NOT_PSRAM;
    } else {
        type_psram = BOARD_PSRAM;
    }
    #endif
    
    #ifdef PSRAM_BUTTER
    init_psram_butter(psram_pin_cs);
    size_psram = get_psram_size();
    if (size_psram == 0) {
        type_psram = NOT_PSRAM;
        deinit_psram_butter(psram_pin_cs);
    } else {
        psram_avaiable = 1;
        type_psram = BUTTER_PSRAM;
    }
    #endif
    
    #ifdef PSRAM_BOARD
    size_psram = init_psram_board();
    if (size_psram == 0) {
        type_psram = NOT_PSRAM;
        return;
    }
    type_psram = BOARD_PSRAM;
    #endif
    
    #ifdef PSRAM_NOSUPORT
    type_psram = BOARD_PSRAM_NOSUPORT;
    #endif
}

// ===============================================================
// ЗАКРЫТИЕ И СБРОС
// ===============================================================
void close_all(void) {
    #ifdef PSRAM_BUTTER_OR_PSRAM_PSRAM_BOARD
    if (type_psram == BUTTER_PSRAM) {
        memset((void *)PSRAM_DATA, 0, size_psram);
        qmi_hw->direct_csr = 0;
        gpio_set_function(psram_pin_cs, GPIO_FUNC_NULL);
        gpio_set_dir(psram_pin_cs, GPIO_IN);
        return;
    }
    if (type_psram == BOARD_PSRAM) {
        gpio_init(psram_pin_cs);
        gpio_set_dir(psram_pin_cs, GPIO_OUT);
        gpio_put(psram_pin_cs, true);
    }
    #endif
}

void pico_reset(void) {
    #ifdef GENERAL_SOUND
    sys_GS(GS_RESET);
    #endif
    close_all();
    watchdog_enable(1, true);
    while (1);
}

// ===============================================================
// ТРИГГЕРЫ И ИНДИКАТОРЫ
// ===============================================================
void led_trdos(void) {
    #if LED_BOARD != 255
    #endif
    
    if (!vbuf_en) return;
    
    if (Requests & 0b01000000) {
        uint8_t color_fon = zx_Border_color & 0x07;
        draw_symbol(0, 240 - 16, 0, CL_BLUE, color_fon);
    }
    
    #ifdef Z_CONTROLER
    if ((z_controler_cs & 0x02) == 0) {
        uint8_t color_fon = zx_Border_color & 0x07;
        draw_symbol(0, 240 - 16, 0, CL_GREEN, color_fon);
    }
    #else
    if ((z_controler_cs & 0x02) == 0) {
        uint8_t color_fon = zx_Border_color & 0x07;
        draw_symbol(0, 240 - 16, 0, CL_GREEN, color_fon);
    }
    #endif
}

// ===============================================================
// ZX THREAD
// ===============================================================
void ZXThread(void) {
    zx_machine_init();
    zx_machine_main_loop_start();
}

// ===============================================================
// MAIN
// ===============================================================
int fast(main)(void) {
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(50);
    set_sys_clock_khz(252*1000, 0);// стартовая частота pico 120

#ifdef PICO_RP2350 
  // определение RP2350 A или B  
     rp2350a = (*((io_ro_32*)(SYSINFO_BASE + SYSINFO_PACKAGE_SEL_OFFSET)) & 1);
     psram_pin_cs = rp2350a ? PSRAM_BUTTER_PIN_CS : 47;
//--------------------------------------------------------------------------------------------------
//  GPIO 23 // Drive high to force power supply into PWM mode (lower ripple on 3V3 at light loads)
// MODE=0 (PFM — Pulse Frequency Modulation)
// MODE=1 (PWM — Pulse Width Modulation)
#if POWER_MODE != 255
    gpio_init(23);
    gpio_set_dir(23, GPIO_OUT);
    gpio_put(23, POWER_MODE);
#endif
    //--------------------------------------------------------------------------------------------------
    // для корректного запуска с бутербродом PSRAM  GPIO 0,8,19,47 для всех вариантов ))
     gpio_init(psram_pin_cs);
   // gpio_set_dir(psram_pin_cs, GPIO_OUT);// Не загружается если psram_pin_cs притянут физически к +3.3В через R= 10 KOм
    gpio_set_dir(psram_pin_cs, GPIO_IN); //Так работает
    gpio_pull_up(psram_pin_cs); 
    //gpio_disable_pulls(psram_pin_cs); // 
#endif

    #if LED_BOARD != 255
    gpio_init(LED_BOARD);
    gpio_set_dir(LED_BOARD, GPIO_OUT);
    gpio_put(LED_BOARD, 1);
    #endif

    #if defined(RTC_NOVA) || defined(RTC_SMUC) || defined(RTC_GLUK)
    rtc_enable = 0;
    #endif

    init_fs = disk_initialize(0); // инициализация SD
    DIR fs;
    init_fs = init_filesystem();                // монтирование и инициализация SD
    config_init();                              // загрузка файла конфигурации если он есть "0:/.config/speccy_p.cnf"
    config_ini_load("0:/.config/speccy_p.ini"); // текстовый файл конфига  если его нет то файл записывается

    init_pico(); // инициализация частоты cpu pico и частоты флеш

    pico_fatfs_reboot_spi();      // Переинициализировать SPI
    init_fs = disk_initialize(0); // инициализация SD
    init_fs = init_filesystem();

    init_and_info();

   // emu_start(); // Запускаем эмуляцию

    #ifndef GENERAL_SOUND
  
    select_audio();
    repeating_timer_t timer_audio;
    if (!add_repeating_timer_us(AY_SAMPLE_RATE, AY_timer_callback, NULL, &timer_audio)) {
        return 1;
    }
    #endif

    repeating_timer_t zx_flash_timer;
    if (!add_repeating_timer_us(-1000000 / 2, zx_flash_callback, NULL, &zx_flash_timer)) {
        return 1;
    }

    multicore_launch_core1(ZXThread);
    disk_autorun();
    gpio_put(LED_BOARD, 0);

    while (1) {
        keyboard_and_other();
        zx_machine_input_set(&zx_input);
        led_trdos();
    }
}
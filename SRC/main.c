#pragma message " SPECCY P."

#include "config.h"

#include <stdio.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "hardware/flash.h"
#include <hardware/sync.h>
#include <hardware/irq.h>
#include "hardware/watchdog.h"
#include "hardware/clocks.h"
#include "hardware/structs/systick.h"
#include "hardware/pwm.h"
#include "hardware/vreg.h"
#include "screen_util.h"
#include "util_sd.h"
#include "util_z80.h"
#include "util_sna.h"
#include "util_tap.h"
#include "string.h"


#include "utf_handle.h"

#include "util_trd.h"

#include "drivers/psram/psram_spi.h"


#include "g_config.h"

#include "tft_driver.h"

#include "ps2.h"
#include "PIO_program1.h"

#include "zx_emu/zx_machine.h"
//#include "psram_spi.h"
//////////////////////////////////////////
#include "tusb.h"

#include "I2C_rp2040.h"// это добавить
#include "usb_key.h"// это добавить
#include "joy.h"
#include "zx_emu/zx_machine.h"
//#include "drivers\psram\psram_spi.h"
/////////////////////////////////////////
// headers
void file_select_trdos(void);
void setup_zx(void);
bool save_config(void);
void  config_init(void);
void sd_speed(uint8_t x);
void help_zx(void);
void turbo_switch(void);
void led_trdos(void);
void load_slot(void);
void save_all(void);
void save_slot(void);

void resetAY(){
	AY_reset();	
}
//===============================================================
void nmi_zx()
{
   main_nmi_key  = true; 
}
//--------------------------------------------------
	FIL f;
	int fr =-1;


//extern bool z80_gen_nmi_from_main;//  = false;
extern bool im_z80_stop;
extern bool im_ready_loading;

uint8_t saves[11];

uint32_t last_action = 0;
uint32_t scroll_action = 0;
uint16_t scroll_pos =0;
//bool showEmpySlots = false;



bool is_menu_mode=false;

ZX_Input_t zx_input;


//---------------------------------------------------------
//инициализация переменных для меню
	bool read_dir = true;
	int shift_file_index=0;
	int cur_dir_index=0;
	int cur_file_index=0;
	int cur_file_index_old=0;
	int N_files=0;
	char current_lfn[LENF];
	//uint64_t current_time = 0;
	char icon[3];
	uint8_t sound_reg_pause[18];

	int lineStart=0;	

	int tap_block_percent = 0;
	
	bool go_menu_mode=false;
	//bool is_menu_mode=false;
   bool is_menu_d_mode=false;

	bool old_key_menu_state=false;
	bool key_menu_state=false;
	bool is_new_screen=false;
	bool is_nmi_menu=false;///////////////////

	
	bool old_key_help_state=false;
	bool key_help_state=false;
	bool help_mode_draw=false;
	
	static bool is_emulation_mode=true;
	bool need_reset_after_menu=false;
	
	bool is_pause_mode=false;
	bool old_key_pause_state=false;
	bool key_pause_state=false;

	//bool is_fast_submenu_mode=false;
	//int fast_menu_index=0;
//	uint8_t current_fast_menu=0;

	bool is_osk_mode=false;
	uint8_t current_key=0;
	
	uint8_t paused=0;

	char save_file_name_image[25];


//---------------------------------------------------------

void off_any_key(void)
{ return;
    while (1)
    {
       decode_key(is_menu_mode);  
       if  ( decode_joy_to_keyboard() ) return ;
       if (KEY_ENTER)  NULL ;
       else return;
    }
}
//---------------------------------------------

//----------------------------
	// меню  initial
	const char __in_flash() *menu_initial[4]={
	//char*  menu_initial[1]={	
	" ",
	
};
#if defined(TFT)
 	// меню setup
	const char __in_flash() *menu_setup[14]={
	" Memory      ",
	" Sound       ",
    " Turbo       ",
    " AY Table    ",
    " SPIbaudrate ",
    " Noice FFD   ",
    " AutoRun     ",
    " Joystick    ",
    " Bright TFT  ",
    "             ",
	" Save config ",
	" Z80  reset  ",
	" Hard reset  ",
    " Exit        ",
	"             ",
	
};

#else

	// меню setup
	const char __in_flash() *menu_setup[15]={
	" Memory      ",
	" Sound       ",
    " Speed Mode  ",
    " AY Table    ",
    " Speed SD    ",
    " Noice FFD   ",
    " AutoRun     ",
    " Joystick    ",
    "             ",
    "             ",
	" Save config ",
	" Z80  reset  ",
	" Hard reset  ",
    " Exit        ",
	"             ",
	
};
#endif

// меню help
	const char __in_flash() *menu_help[17]={
	//char*  menu_setup[6]={	
	"[F11], [WIN],[Insert] file browser",
	"[F12], [HOME] settings menu     ",
    "[F2] Save slots [F3] Load slots",
    "[F5] Save slots 0 and config",
    "[F7] Volume down [F8] Volume up",
    "[F10] Normal/Turbo/Fast",
    "[F9] NMI key only Navigator 256",
    "  ",
    "[ESC] exit almost all menus ",
    "  ",
    "Video driver, etc. @Alex_Eburg ",
    "https://t.me/ZX_MURMULATOR ",
    "TR-DOS sources are used:",
    "https://bitbucket.org/rudolff/fdcduino",
    "And the rest",
    "The author of the compilation:",
	"https://t.me/const_bill",
	
};
	// меню ram
	const char __in_flash() *menu_ram[8]={
	//char*  menu_ram[7]={	
	" ZX Spectrum  128 ",
	" Pentagon     512 ",
	" Pentagon    1024 ",
	" Scorpion     256 ",
	" Profi       1024 ",
	" ScorpionGMX 2048 ",
	" Unreal      4096 ",
    " Navigator    256 ",
};
	// меню sound
	const char __in_flash() *menu_sound[6]={
	//char*  menu_sound[4]={	
	" Soft   AY Sound ",
	" Soft TurboSound ",
	" Hard   AY Sound ",
	" Hard TurboSound ",
    " I2S    AY Sound ",
    " I2S  TurboSound ",
};
	// меню joy
	const char __in_flash() *menu_joy[4]={
	" Kempston  NES Joy ",
	" Kempston   Arrows ",
	" Interface2 Arrows ",
	" Cursor     Arrows ",
    };

	// меню spi baudrate
	const char __in_flash() *menu_spi[4]={
	" < 25000000 bd ",
	" < 12000000 bd ",
	" < 10000000 bd ",
	"    8000000 bd ",
    };

    // menu_autorun
	const char __in_flash() *menu_autorun[3]={
    " OFF              ",    
	" File TR-DOS      ",
	" QuickSave Slot 0 ",
    };

    //menu_speed
	const char __in_flash() *menu_speed[3]={
    " NORMAL INT  50Hz ",    
	" TURBO  INT  50Hz ",
	" FAST   INT 100Hz ",
    };

	// меню table of amplitudes
	const char __in_flash() *menu_ampl[7]={
	" 0.default     ",
	" 1.Chinese     ",
	" 2.linear      ",
	" 3.curved      ",
    " 4.depending   ",
	" 5.hybrid      ",
	" 6.Rampa table ",
    };
//---------------------------------------------------------
void software_reset(){
	watchdog_enable(1, 1);
	while(1);
}

//функция ввода загрузки спектрума
uint8_t valLoad=0;




bool b_beep;
bool b_save;


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
	zx_machine_main_loop_start();
	G_PRINTF_INFO("END spectrum emulation\n");
	return ;
}

void  inInit(uint gpio){
	gpio_init(gpio);
	gpio_set_dir(gpio,GPIO_IN);
	gpio_pull_up(gpio);
	
}

//=================================================================================================================================

bool __not_in_flash_func(AY_timer_callback)(repeating_timer_t *rt)
{
if (!is_menu_mode)          //(is_menu_mode == 0)
{
    audio_out();
    return true;
}
 return true;
}
//============================================================================


//============================================================================
//bool zx_flash_callback(repeating_timer_t *rt) {zx_machine_flashATTR();};
bool zx_flash_callback(repeating_timer_t *rt) {zx_machine_flashATTR();return true;};
//============================================================================
bool zx_int(repeating_timer_t *rt) {zx_machine_int();return true;};
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//=========================================================================
int main(void){  
	delay_key=DELAY_KEY;
	vreg_set_voltage(VOLTAGE);
	sleep_ms(100);
	set_sys_clock_khz(CPU_KHZ, false);

	stdio_init_all();
	g_delay_ms(100);

#ifdef WS2812
ws2812_init();
ws2812_reset();
#endif
	//пин ввода звука
	inInit(PIN_ZX_LOAD);

     int fs=vfs_init();
     bool no_sd  = init_filesystem();
     config_init();
   printf("starting  config_init\n");
   sd_speed(conf.spi_bd);// установка скорости spi sd карты 0..3 def 1
   select_audio(conf.ay);// переключение режимов вывода звука 
 printf("starting select_audio \n");

   init_vol_ay(conf.vol_bar);// установка громкости AY

	int hz = 96000;	//44000 //44100 //96000 //22050
//sleep_ms(4000);
  //     printf("F:%x\n", (1000000 / hz));
	repeating_timer_t timer;
	// negative timeout means exact delay (rather than delay between callbacks)
	//if (!add_repeating_timer_us(-10, AY_timer_callback, NULL, &timer)) {
	if (!add_repeating_timer_us(AY_SAMPLE_RATE, AY_timer_callback, NULL, &timer)) // -10  частота ноты До 237Гц  нужно 240,0058 Гц
    {
	//	G_PRINTF_ERROR("Failed to add timer\n");
		return 1;
	}
	
	repeating_timer_t zx_flash_timer;
	 hz=2;
	if (!add_repeating_timer_us(-1000000 / hz, zx_flash_callback, NULL, &zx_flash_timer)) {
	//	G_PRINTF_ERROR("Failed to add zx flash timer\n");
		return 1;
	}


//---------------------------------------------------------
// INT generator 50Hz
 	repeating_timer_t int_timer; 
	 hz=50;
	if (!add_repeating_timer_us(-1000000 / hz, zx_int, NULL, &int_timer)) {
		G_PRINTF_ERROR("Failed to add INT timer\n");
		return 1;
	}  

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    startVIDEO();

//--------------------------------------------------------


	uint inx=0;		

	convert_kb_u_to_kb_zx(&kb_st_ps2,zx_input.kb_data);
	
	
	inInit(PIN_ZX_LOAD);

// инициализация с выводом результата на дисплей
 
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);

        zx_machine_enable_vbuf(false);
        
	    init_screen(g_gbuf,SCREEN_W,SCREEN_H);

        #define YPOS FONT_H*2 
        #define XPOS FONT_W+2

    	draw_rect(0,0,SCREEN_W,SCREEN_H,CL_BLACK,true);//Заливаем экран 
	    draw_rect(9,9,SCREEN_W-20,SCREEN_H-100,CL_CYAN,false);//Основная рамка TEST
        draw_text(7+XPOS,0+YPOS,"ZXSpeccy P",CL_GRAY,CL_BLACK);	
        draw_text_len(100+XPOS,YPOS,FW_VERSION,CL_GRAY,CL_BLACK,10);	
        snprintf(temp_msg, sizeof temp_msg, "CPU %d MHz",(int) (clock_get_hz(clk_sys)/1000000));
		draw_text(200+XPOS,YPOS,temp_msg,CL_GRAY,CL_BLACK);

		if (no_sd) draw_text(10+XPOS,20+YPOS,"SD card detected",CL_GREEN,CL_BLACK);
	        	else draw_text(10+XPOS,20+YPOS,"SD card not detected",CL_RED,CL_BLACK);

       d_joy_init();
	   decode_joy(); //????????

	      if(gpio_get(D_JOY_DATA_PIN)) draw_text_len(10+XPOS,30+YPOS,"NES Joy present",CL_GREEN,CL_BLACK,20);
         else draw_text_len(10+XPOS,30+YPOS,"NES Joy not found",CL_RED,CL_BLACK,20);
      
////////////////////////////////////////////////////////////////
         g_delay_ms(100);

   //      draw_text(10+XPOS,40+YPOS,"Please restart",CL_RED,CL_BLACK);
   //      draw_text(10+XPOS,50+YPOS,"USB_to_I2C adapter",CL_RED,CL_BLACK);
       // initialize i2c0 
      //   I2C_Init1(400000); // I2C1
      //   I2C_Pins(18,19); //18 I2C1 SDA , 19 I2C1 SCL
      //////   I2C_Init0(400000); // I2C0
      //////   I2C_Pins(0,1); //0 I2C1 SDA , 1 I2C1 SCL
////////////////////////////////////////////////////////////////	

	  //  uint8_t ibuff2[5] = {0x00}; //address i2c 0x77

	    ibuff[0] = 0x00; //address i2c 0x77
        ibuff[1] = 0xff; 
        ibuff[2] = 0xff; 
        ibuff[3] = 0xff; 
	/////    I2C_Read_nbytes_stop(i2c0,0x77, ibuff, 1);

	//	mode_kbms = ibuff[0];
       
 //printf("init_usb_hid \n");

         if (init_usb_hid()) mode_kbms =8;// USB HID


    //    draw_text_len(10+XPOS,50+YPOS,temp_msg,CL_GREEN,CL_BLACK,20); 
start_PS2_capture(); // multicore_launch_core1( start_PS2_capture);

#if defined(NO_PSRAM_21)    
//-----------------NO PSRAM -------------------
PSRAM_AVAILABLE =0;

	draw_text(10+XPOS,60+YPOS,"NO PSRAM / Clock AY on pin21",CL_LT_PINK,CL_BLACK);
    conf.mashine = 0; // only 128kB

//----------------- PSRAM END ---------------
#else
//----------------- PSRAM -------------------
psram_size = init_psram();
//PSRAM_AVAILABLE =0;
if (PSRAM_AVAILABLE) 
{
     snprintf(temp_msg, sizeof temp_msg, "PSRAM %dMb available", psram_size);
	draw_text(10+XPOS,60+YPOS,temp_msg,CL_GREEN,CL_BLACK);
}
else 
{
	draw_text_len(10+XPOS,60+YPOS,"PSRAM not found",CL_RED,CL_BLACK,16);
	uninit_psram();
    conf.mashine = 0; // only 128kB
}
//----------------- PSRAM END ---------------
#endif

// информация из setup
draw_text(10+FONT_W+1,75+YPOS,menu_ram[conf.mashine],CL_GRAY,CL_BLACK);
//init_extram(conf.mashine);
draw_text(10+FONT_W+1,85+YPOS,menu_sound[conf.ay],CL_GRAY,CL_BLACK);    


draw_text(10+XPOS,205+YPOS," [F12]-Setup [F11]-Files [F1]-Help",CL_GREEN,CL_BLACK);
draw_text(70+XPOS,140+YPOS,"Forward to the past",CL_GRAY,CL_BLACK);
//==================================================================================

     //    if (init_usb_hid()) mode_kbms =8;// USB HID
   mode_kbms =8;// USB HID      
// init_decode_key(mode_kbms);
//g_delay_ms(10000);
 if (init_usb_hid()) // USB HID
 {
for(int i = 0; i < 1000; i++)
{
     tuh_task(); // tinyusb host task
    switch (usb_device) 
    {
              case 0:
              snprintf(temp_msg, sizeof temp_msg, "No USB device");
              break;
              case 1:
              snprintf(temp_msg, sizeof temp_msg, "USB keyboard");
              break;
              case 2:
              snprintf(temp_msg, sizeof temp_msg, "USB mouse");
              break;
              case 3:
              snprintf(temp_msg, sizeof temp_msg, "USB keyboard + mouse");
              break;
    }
    draw_text_len(10+XPOS,50+YPOS,temp_msg,CL_GREEN,CL_BLACK,20); 
}

 }
flag_usb_kb = false;

    // ожидание клавиши перед запуском
 /*     while (!((kb_st_ps2.u[3] == 0) & (kb_st_ps2.u[2] == 0) & (kb_st_ps2.u[1] == 0) & (kb_st_ps2.u[0] == 0)))
    {
        decode_key();
    }
 
 */
//	while (! ((decode_key()) | (decode_joy()) )){}
   //  sleep_ms(3000);
    printf("starting main loop \n");
 #if defined(TRDOS_0)   
  InitTRDOS();
  #endif
// disk_autorun ();
  	//g_delay_ms(10);
	multicore_launch_core1(ZXThread);
//	g_delay_ms(500);


 //sleep_ms(9000) ;  
   disk_autorun ();



    //   основной цикл
    #ifdef TEST
    wait_msg = 5000;// сообщения внизу и громкость время вывода
     msg_bar = 0;
    vol_ay = conf.vol_bar/10; 
    vol_i2s = conf.vol_bar;
  #endif

    while (1)
    {

        //++++++++++++++++++++++++++++++++++++++++++
        #ifdef TEST
        
       
        if (wait_msg !=0)
        {
           wait_msg--;
           
        switch (msg_bar)
        {
        case 0:
            draw_text(84,240-7,"ZX SPECCY P v0.15.3",CL_LT_CYAN ,CL_BLACK);//232
            break;
            case 1:
            sprintf(temp_msg, "VOL %d ", conf.vol_bar);//%%
            draw_text(250,240-7,temp_msg,CL_GREEN ,CL_BLACK);//232
            break;
        case 2:
            sprintf(temp_msg, "VOL %d ", conf.vol_bar);//%%
            draw_text(250,240-7,temp_msg,CL_GREEN ,CL_BLACK);//232
            break;
        case 3:
            sprintf(temp_msg, "NORMAL ");
            draw_text(250,240-7,temp_msg,CL_GREEN ,CL_BLACK);//232
            break;
        case 4:
            sprintf(temp_msg, "TURBO  ");
            draw_text(250,240-7,temp_msg,CL_GREEN ,CL_BLACK);//232
            break;
        case 5:
            sprintf(temp_msg, "FAST  ");
            draw_text(250,240-7,temp_msg,CL_GREEN ,CL_BLACK);//232
            break;            

 /*        case 6:
            sprintf(temp_msg, " CONNECTED USB  %4X:%4X         ", vid,pid);//%%
            draw_text(0,240-7,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;    */

        //case 7:
          //  sprintf(temp_msg, " %4X KEMPSTON", joy_k);//%%
          //  draw_text(0,240-7,temp_msg,CL_WHITE ,CL_BLACK);//232
          //  break;   
      

        case 8:
            sprintf(temp_msg, "  GAMEPAD XBOX             ");//%%
            draw_text(0,240-7,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;   
      
    /*     case 9:
           sprintf(temp_msg, "  ROM:  %4X    %4X       ", rom_n,rom_n1 );//%%zx_7ffd_lastOut
        //  sprintf(temp_msg, "  zx_7ffd_lastOut:  %4X        ", zx_machine_get_7ffd_lastOut() );//%%
            draw_text(0,240-7,temp_msg,CL_WHITE ,CL_BLACK);//232
            break;   
 */


        default:
            break;
        }


        }
         
        #endif
       //++++++++++++++++++++++++++++++++++++++++++
        // опрос джоя
       if (conf.joyMode == 0)
        {
            if ((inx++ %5) == 0)
            {

                if (joy_connected)
                {
                    zx_input.kempston = (uint8_t)(data_joy);
                }
                else
                {
                    zx_input.kempston = 0;
                }
            };
        }

  // ОПРОС КЛАВИАТУРЫ И ДЖОЙСТИКА 
 // if ((decode_key(is_menu_mode)) | (decode_joy())| (go_menu_mode))
  if ((decode_PS2()) | (decode_key(is_menu_mode)) | (decode_joy())| (go_menu_mode))
    {

            go_menu_mode = false;
            //  reset pico
            if (KEY_RESET_PICO)
            {
                software_reset();
            }
            //==============================

 
            //==============================
            // кнопка перехода в меню файлов
             key_menu_state =( (MENU) | (joy_key_ext  == 0x84));

           // кнопка выхода из меню файлов по joy 
           //  if ( (joy_key_ext  == 0x10) & (is_menu_mode))// exit [B] joy
             if ( (joy_key_ext  == 0x80) & (is_menu_mode)) // exit [start] joy
             {
                key_menu_state = true;
             }


            if (key_menu_state & !old_key_menu_state)
            {
                data_joy =0;
                is_menu_mode ^= 1;
                is_new_screen = true;
                zx_machine_enable_vbuf(false);
                is_emulation_mode = false;
                im_z80_stop = false;
                hardAY_on();
            }

            else
            {  
                is_new_screen = false;
                is_emulation_mode = true;
                im_z80_stop = true;
            }
            old_key_menu_state = key_menu_state;

            //
            // кнопка перехода в меню SETUP
           if (((MENU_SETUP) | (joy_key_ext  == 0x88)) )  setup_zx();  // START+ [B] SetUp

            //
            if (F1)  help_zx();

            if (F9)  nmi_zx();   

            if (type_sound!=1) // если hard ts 
            {
            if (F7)  // громкость -
            {
              // kb_st_ps2.u[3] = 0; // key F7
               msg_bar=1;
               wait_msg = 1000;                 
               if (conf.vol_bar==0) conf.vol_bar=0;
               else conf.vol_bar--;
               vol_ay=conf.vol_bar; 
               init_vol_ay(vol_ay);  
              } 

            if (F8)  // громкость +
            {  
             //  kb_st_ps2.u[3] = 0; // key F8
               msg_bar=2;  
               wait_msg = 1000; 
               uint8_t vol_max;
               if (type_sound==0) vol_max=32; else vol_max=255;
               if (conf.vol_bar==vol_max) conf.vol_bar=vol_max;
                else conf.vol_bar++;
               vol_ay=conf.vol_bar; 
               init_vol_ay(vol_ay);
            }
            }
            if (F10)  // TURBO
            {
               conf.turbo++; 

               turbo_switch();
               msg_bar=3+conf.turbo;
               wait_msg = 1000; 

            }

            // кнопка остановки по PAUSE
 

 //  key_pause_state = (((PAUSE) | (joy_key_ext == 0xa0)) ); //Кнопка [PAUSE] клавиатуры - останов эмулятора
             key_pause_state = (PAUSE); //Кнопка [PAUSE] клавиатуры - останов эмулятора


            if (key_pause_state & !old_key_pause_state)
            {
                is_pause_mode ^= 1;
            };
            old_key_pause_state = key_pause_state;

            is_emulation_mode = !(is_menu_mode); //

            if ((is_pause_mode) && (paused == 0))
            {
                paused = 1;
            }
            if ((!is_pause_mode) && (paused == 2))
            {
                paused = 10;
            }

            //========================================================================
            // непосредственно цикл меню если is_menu_mode =true файловое меню
            if (is_menu_mode)// файловое меню
            {
                hardAY_off();// отклбчение звука железного AY
             //   g_delay_ms(10);

                //	tap_loader_active=false;// рудимент от аудио загрузки
 
               if (!init_fs)
                {
                    g_delay_ms(10);
                    init_fs = init_filesystem();
                    N_files = read_select_dir(cur_dir_index);
                    if (N_files == 0)  init_fs == false;
                }
                //++++++++++++++++++++++++++++++++++++++++++
               if (is_new_screen)
               { 
                     if (!init_fs)
                 {
                    memset(g_gbuf, COLOR_BACKGOUND, sizeof(g_gbuf));
                    MessageBox("SD Card not found!!!", "    Please REBOOT   ", CL_LT_YELLOW, CL_RED, 0);
                    continue;
                 } 
                      draw_main_window();// рисование рамок
                      draw_file_window();// рисование каталога файлов

 
                     if (init_fs)
                    {
                       N_files = read_select_dir(cur_dir_index);

                        if (N_files == 0)
                        {
                            init_fs = false;
                        }
                        else
                        {
                          //  printf("file: %d,%d\n",cur_file_index,cur_file_index_old);
                          //  draw_file_window();
                            cur_file_index_old = -1;
                        }
                      
                    }  
                 
 
                   //++++++++++++++++++++++++++++++++++++++++++++++++++++++    
                }  // if ((is_new_screen))
                  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++

                if (init_fs)
                {
                   //  if (!decode_key_joy()) continue;   
                //   decode_key_joy() ;

                 //    if (((KEY_SPACE) | (data_joy == 0x10)) && (init_fs)) //  нажатие пробела запуск после сброса [B] joy
                   if ((KEY_SPACE)  && (init_fs)) //  нажатие пробела запуск после сброса [B] joy
                    {        
                       // off_any_key() ;// отжатие любой клавиши
                     
                        strcpy(conf.activefilename, dir_patch); // strcpy
                        strcat(conf.activefilename, "/");
                        strcat(conf.activefilename, files[cur_file_index]);

                        afilename[0] = 0;
                        strcat(afilename, files[cur_file_index]);

                        //  const char *ext = get_file_extension(afilename);

                        const char *ext = get_file_extension(conf.activefilename);
                        if (strcasecmp(ext, "trd") == 0) //   запуск после сброса
                        {
                            // file_select_trdos();
                            // Копируем строку длиною не более 10 символов из массива src в массив dst1.
                            // strncpy (dst1, src,3);

                            MessageBox(" RUNING TRD FILE ", "", CL_WHITE, CL_BLUE, 4);



                            #if defined(TRDOS_0)
                            conf.sclConverted = 1; // к диску а подключен TRD образ
                            strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                            OpenTRDFile(conf.activefilename, 0);    
                            write_protected = false; // защита записи отключена для TRD
                            #endif

                             #if defined(TRDOS_1)
                             file_type = 0; //trd
                             conf.sclConverted = 1; // к диску а подключен TRD образ
                             strncpy(conf.DiskName[0], files[cur_file_index], LENF);
                             OpenTRDFile(conf.activefilename,0);
                             write_protected = false; // защита записи отключена для TRD
                             #endif


                            zx_machine_reset(1);// включить загрузку файла при reset 1 раз

                            is_new_screen = false;
                            is_menu_mode = false;
                            im_z80_stop = false;

                            hardAY_on();
                            continue; 
                        }
                        if (strcasecmp(ext, "scl") == 0) //  запуск после сброса
                        {
                            MessageBox(" RUNING SCL FILE ", "", CL_WHITE, CL_BLUE, 4);
                            write_protected = true; // защита записи для SCL
                            conf.sclConverted = 0;
                            file_type = 1;// scl
                            strncpy(conf.DiskName[0], files[cur_file_index], LENF); // disk A
                            Run_file_scl(conf.activefilename, 0);
                            zx_machine_reset(1);// включить загрузку файла при reset 1 раз
                            im_z80_stop = false;
                            is_menu_mode = false;
                            is_new_screen = false;
                            hardAY_on();
                            continue;
                        }
                            continue;
                    }// end KEY_SPACE

                 //    if (((KEY_ENTER) | (data_joy == 0x20)) && (init_fs))// нажатие enter
                       if ((KEY_ENTER) && (init_fs))// нажатие enter
                      
                    {
                         //    off_any_key() ;// отжатие любой клавиши
                    //  g_delay_ms(500);
                        // printf("file_type:%x\n",files[cur_file_index][13]);
                        if (files[cur_file_index][LENF1])
                        { // выбран каталог
                        
                     //      printf("cur_file_index:%d  dir_index:%d\n",cur_file_index,cur_dir_index);
                            if (cur_file_index == 0)
                            { // на уровень выше
                                if (cur_dir_index)
                                {
                                 
                                    cur_dir_index--;
                                    N_files = read_select_dir(cur_dir_index);
                                    cur_file_index = 0;
                                   // draw_text_len(2 + FONT_W, 2 * FONT_H , " ****     ", CL_TEST, COLOR_BORDER, 14);
                                    continue;
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
                                continue;
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

                          //  const char *ext = get_file_extension(afilename);

                            const char *ext = get_file_extension(conf.activefilename);

                         //   G_PRINTF_DEBUG("current file ext=%s\n", ext);
                         //   G_PRINTF_DEBUG("current file select=%s\n", conf.activefilename);

                            if (strcasecmp(ext, "z80") == 0)
                            {

                                im_z80_stop = true;
                                while (im_z80_stop)
                                {
                                    sleep_ms(10);
                                    if (im_ready_loading)
                                    {
                                        // sleep_ms(10);
                                        zx_machine_reset(3);
                                        AY_reset(); // сбросить AY

                                        if (load_image_z80(conf.activefilename))
                                        {
                                            memset(temp_msg, 0, sizeof(temp_msg));
                                            sprintf(temp_msg, " Loading file:%s", afilename);
                                            MessageBox("Z80", temp_msg, CL_WHITE, CL_BLUE, 2);
                                            conf.activefilename[0] = 0;
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            paused = 0;
                                            is_menu_mode = false;
                                            hardAY_on();
                                            // printf("load_image_z80 - OK\n");
                                            break;
                                        }
                                        else
                                        {
                                            MessageBox("Error loading snapshot!!!", afilename, CL_YELLOW, CL_LT_RED, 1);
                                            // printf("load_image_z80 - ERROR\n");
                                            last_action = time_us_32();
                                            draw_file_window();
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            paused = 0;
                                            break;
                                        }
                                        // AY_reset();// сбросить AY
                                    }
                                }

                                continue;
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
                                        zx_machine_reset(3);
                                        AY_reset(); // сбросить AY
                                        if (load_image_sna(conf.activefilename))
                                        {
                                            memset(temp_msg, 0, sizeof(temp_msg));
                                            sprintf(temp_msg, " Loading file:%s", afilename);
                                            MessageBox("SNA", temp_msg, CL_WHITE, CL_BLUE, 2);
                                            conf.activefilename[0] = 0;
                                            im_z80_stop = false;
                                            im_ready_loading = false;
                                            hardAY_on();
                                            paused = 0;
                                            is_menu_mode = false;
                                            // printf("load_image_sna - OK\n");

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
                                            paused = 0;
                                            break;
                                        }
                                        // AY_reset();// сбросить AY
                                    }
                                }
                                continue;
                            }
                            else if (strcasecmp(ext, "scr") == 0)
                            {
                                // G_PRINTF_DEBUG("current file select=%s\n",conf.activefilename);
                                if (LoadScreenshot(conf.activefilename, true))
                                {
                                    is_menu_mode = false;
                                    continue;
                                }
                                else
                                {
                                    MessageBox("Error loading screen!!!", afilename, CL_YELLOW, CL_LT_RED, 1);
                                    // printf("LoadScreenshot - ERROR\n");
                                    break;
                                }
                            }
                            else if (strcasecmp(ext, "tap") == 0)
                            {

                                paused = 0;

                                Set_load_tape(conf.activefilename, current_lfn);
                                strcpy(temp_msg, current_lfn);
                                MessageBox("    TAPE    ", temp_msg, CL_WHITE, CL_BLUE, 4);
                                //     zx_machine_reset();

                                im_z80_stop = false;
                                is_menu_mode = false;
                                 hardAY_on();
                                continue;
                            }
                            // TRDOS обработка

                            if (strcasecmp(ext, "trd") == 0)
                            {
                                file_select_trdos();
                                // continue;
                            }
                            // SCL обработка

                            if (strcasecmp(ext, "scl") == 0)
                            {
                                MessageBox("SCL files are mounted", "   only on Drive A:", CL_WHITE, CL_BLUE, 4);// 3 delay 250 4 -1000
                                write_protected = true; // защита записи для SCL
                                conf.sclConverted = 0;
                                file_type = 1;//scl
                              //  strncpy(DiskName[0], current_lfn, 16);
                                strncpy(conf.DiskName[0], files[cur_file_index], LENF);// disk A
                                Run_file_scl(conf.activefilename, 0);

                                draw_main_window(); // восстановление текста
                                draw_file_window();

                                // g_delay_ms(200);
                                last_action = time_us_32();

                                is_new_screen = 1;

                                continue;
                            }
                        }
                    }

                    int num_show_files = 21; // количество файлов при показе


                    // стрелки вверх вниз
                 //   if (((kb_st_ps2.u[2] & KB_U2_DOWN) | (data_joy == 0x04)) && (cur_file_index < (N_files)))
                    if ((kb_st_ps2.u[2] & KB_U2_DOWN) && (cur_file_index < (N_files)))
                    {
                     // sleep_ms(delay_key);

                        cur_file_index++;
                        last_action = time_us_32();
                        
                    }
                   // if (((kb_st_ps2.u[2] & KB_U2_UP) | (data_joy == 0x08)) && (cur_file_index > 0))
                   if ((kb_st_ps2.u[2] & KB_U2_UP ) && (cur_file_index > 0))
                    {
                      //  sleep_ms(delay_key);
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
                    if (((kb_st_ps2.u[2] & KB_U2_PAGE_DOWN) | (data_joy == 0x01)) && (cur_file_index < (N_files)))
                    {
                        cur_file_index += num_show_files;
                        last_action = time_us_32();
                        
                    }
                    if (((kb_st_ps2.u[2] & KB_U2_PAGE_UP) | (data_joy == 0x02)) && (cur_file_index > 0))
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
                            continue;
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
                        draw_text_len(FONT_W + 2, FONT_H - 1, dir_patch + 1, COLOR_UP, COLOR_BORDER, 38);// путь папки в шапке
                    }
                    else
                    {
                        draw_text_len(FONT_W + 2, FONT_H - 1, "                                     ", CL_TEST, COLOR_BORDER, 38);
                    }

                    for (int i = 0; i < num_show_files; i++)
                    {
                        uint8_t color_text = CL_GREEN;
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
                            draw_text_len(2 + FONT_W, 2 * FONT_H + i * FONT_H, " ", color_text, color_bg, NUMBER_CHAR);
                            continue;
                        }

                        if (files[i + shift_file_index][LENF1])
                        {

                            draw_text_len(2 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text_d, color_bg, NUMBER_CHAR); // get_file_from_dir("0:/z80",i)//?????
                        }
                        else
                        {

                            draw_text_len(2 + FONT_W, 2 * FONT_H + i * FONT_H, files[i + shift_file_index], color_text, color_bg, NUMBER_CHAR); // get_file_from_dir("0:/z80",i)//???????
                        }
                    }
               //    strcpy(current_lfn, get_lfn_from_dir(dir_patch, files[cur_file_index])); // имя длинное должно быть
                   strcpy(current_lfn, get_current_altname(dir_patch, files[cur_file_index]));

                    // draw_rect(10+FONT_W*13,17,3,5,0xf,true);
                    int file_inx = cur_file_index - 1;
                    if (file_inx == -1)
                        file_inx = 0;
                    if (file_inx == N_files)
                        file_inx += 1;
       
                    int shft = 156 * (file_inx) / (N_files <= 1 ? 1 : N_files - 1);

                    if (strcasecmp(files[cur_file_index], "..") == 0)
                    {
                        cur_file_index_old = cur_file_index;
                    
                    }


                 if (cur_file_index)  
                 {
//======================================================================================================================
               strncpy(temp_msg, get_lfn_from_dir(dir_patch, files[cur_file_index]),48); // имя длинное должно быть
				//draw_text_len(2*FONT_W,2*FONT_H+pos*FONT_H,temp_msg,COLOR_SELECT_TEXT,COLOR_SELECT,13);
//G_PRINTF_DEBUG("cur_file_index=%d\n", cur_file_index);
//G_PRINTF_DEBUG("files[cur_file_index]=%s\n", files[cur_file_index]);
				draw_text_len(16+FONT_W*14,16, temp_msg,CL_INK,COLOR_BACKGOUND,22); // длинное имя длжно быть
                for (size_t i = 0; i < 22; i++)
                {
                   temp_msg[i] = temp_msg[i+22];
                }
   
                draw_text_len(16+FONT_W*14,26, temp_msg,CL_INK,COLOR_BACKGOUND,22); // длинное имя длжно быть
 //=====================================================================================================================
                 }
                   else
                   {
                    draw_rect(17+FONT_W*14,16,182,20,COLOR_PIC_BG,true) ;
                   }

                    if ((cur_file_index > 0) && (cur_file_index_old == -1))
                    {
                        last_action = time_us_32();
                      
                    }
                }
                //++++++++++++++++++++++++++++++++++++++++++++++++++++++
            }
            //-------------------------------------------------------------------------------
            if ((!is_menu_mode))
            {  
                if (((kb_st_ps2.u[3] & KB_U3_F2) | (joy_key_ext== 0x82)) )  save_slot();   // [START]+стрелка влево - вход в меню SAVE
                if (((kb_st_ps2.u[3] & KB_U3_F3) | (joy_key_ext == 0x81)) )  load_slot();   // [START]+стрелка вправо - вход в меню LOAD
                if (kb_st_ps2.u[3] & KB_U3_F5)   save_all(); // запись всей памяти и файла конфигурации
           
            }

            if ((is_emulation_mode) && (paused == 2))
            {
                // printf("im_z80_stop:%d\n",im_z80_stop);
                // printf("Paused\n");
                zx_machine_enable_vbuf(false);
                MessageBox("PAUSE", "Screen refresh stopped", CL_LT_GREEN, CL_BLUE, 0);
                continue;
            }

            if (paused == 1)
            {
                // printf("1 paused:%d\n",paused);

                im_z80_stop = true;

                while (im_z80_stop)
                {
                    sleep_ms(10);
                    if (im_ready_loading)
                    {
                        for (uint8_t i = 0; i < 16; i++)
                        {
                            AY_select_reg(i);
                            sound_reg_pause[i] = AY_get_reg();
                            AY_set_reg(0);
                        }
                        sound_reg_pause[16] = beep_data_old;
                        sound_reg_pause[17] = beep_data;
                        resetAY();
                        break;
                    }
                }
                zx_machine_enable_vbuf(false);
                paused = 2;
                if (is_emulation_mode)
                {
                    is_emulation_mode = false;
                    continue;
                }
                else
                {
                    go_menu_mode = true;
                    continue;
                }
                // printf("1 paused:%d\n",paused);
            }
            if (paused == 10)
            {
                printf("2 paused:%d\n",paused);
                resetAY();
                for (uint8_t i = 0; i < 16; i++)
                {
                    AY_select_reg(i);
                    AY_set_reg(sound_reg_pause[i]);
                }
                beep_data_old = sound_reg_pause[16];
                beep_data = sound_reg_pause[17];
                im_z80_stop = false;

                paused = 0;
                // printf("2 paused:%d\n",paused);
                is_emulation_mode = true;
                zx_machine_enable_vbuf(true);
                continue;
            }

            if (is_emulation_mode)
            { // Emulation mode
                // process_menu_mode = false;
                zx_machine_enable_vbuf(true);
                if (im_z80_stop)
                {
                    // printf("Run:%d\n",im_z80_stop);
                    im_z80_stop = false;
                }
                // zx_machine_set_vbuf(g_gbuf);
                if (need_reset_after_menu)
                {
                    zx_machine_reset(3);
                }

                convert_kb_u_to_kb_zx(&kb_st_ps2, zx_input.kb_data);

                if (conf.joyMode == 0)
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
                    }; //|(data_joy==0x01)
                       // if ((data_joy==0x40)){zx_input.kb_data[6]|=1<<0;};
                }
                if (conf.joyMode == 1)
                {
                    // memset(kb_st_ps2.u,0,sizeof(kb_st_ps2.u));
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
                    // printf("data_joy%d\n",data_joy);
                }
                if (conf.joyMode == 2)
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
                if (conf.joyMode == 3) 
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



                /*--Emulator reset--*/
                if (KEY_RESET_ZX)
                {
                    // G_PRINTF_INFO("restart\n");

        	im_z80_stop = true;
        	is_menu_mode = true;
            hardAY_off();
            MessageBox(" ZX SPECTRUM RESET ", "", CL_WHITE, CL_RED, 2);
            
            zx_machine_reset(3);
            im_z80_stop = false;
            is_menu_mode = false;
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();


                }


            } // Emulation mode
              // zx_machine_input_set(&zx_input);
            // process_menu_mode = false;

        } // else//if (decode_PS2())

        zx_machine_input_set(&zx_input);

        if ((is_menu_mode) && (init_fs))
        {

            if ((last_action > 0) && (time_us_32() - last_action) > SHOW_SCREEN_DELAY * 1000)
            {

                 last_action = 0;
            //    CLEAR_INFO; // Фон отображения информации о файле
                 const char* ext = get_file_extension(files[cur_file_index]);
              //  const char *ext = get_file_extension(current_lfn);
               // printf("ext:%s\n", ext);
               // G_PRINTF_DEBUG("files[cur_file_index]=%s\n", files[cur_file_index]);
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

                    continue;
                }
                //-----------------------------------------------------
                //-----------------------------------------------
                // SCL INFO
                if (strcasecmp(ext, "scl") == 0)
                {
					strncpy(temp_msg,current_lfn,22);
				
				//draw_text_len(2*FONT_W,2*FONT_H+pos*FONT_H,temp_msg,COLOR_SELECT_TEXT,COLOR_SELECT,13);
				//draw_text_len(18+FONT_W*14,16, temp_msg,CL_RED,COLOR_BACKGOUND,22);
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					//strcat(conf.activefilename,current_lfn);
					cur_file_index_old=cur_file_index;		



				if(!ReadCatalog_scl(conf.activefilename,current_lfn,false)){


				 } 


                 continue;
                 }
//-----------------------------------------------------
				if(strcasecmp(ext, "z80")==0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					//strcat(conf.activefilename,current_lfn);
					cur_file_index_old=cur_file_index;					
				//	printf("LoadScreenshot: %s\n",conf.activefilename);
					//if(LoadScreenFromZ80Snapshot(conf.activefilename)) printf("show - OK \n"); else printf("screen not found");
					if(!LoadScreenFromZ80Snapshot(conf.activefilename)){
						CLEAR_INFO;
					}
					continue;
				} else
				if(strcasecmp(ext, "scr") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					//strcat(conf.activefilename,current_lfn);
				//	printf("LoadScreenshot: %s\n",conf.activefilename);
					CLEAR_INFO;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenshot(conf.activefilename,false)){
						CLEAR_INFO;
					//	draw_mur_logo();
					} 
					continue;
				} else
				if(strcasecmp(ext, "tap") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
				//	strcat(conf.activefilename,current_lfn);
				//	printf("LoadScreenshot: %s\n",conf.activefilename);
					CLEAR_INFO;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenFromTap(conf.activefilename))
						CLEAR_INFO;;
					continue;
				} 
				if(strcasecmp(ext, "sna") == 0) {
					strcpy(conf.activefilename,dir_patch);
					strcat(conf.activefilename,"/");
					strcat(conf.activefilename,files[cur_file_index]);
					//strcat(conf.activefilename,current_lfn);
					//printf("LoadScreenshot: %s\n",conf.activefilename);
					CLEAR_INFO;;
					cur_file_index_old=cur_file_index;
					if(!LoadScreenFromSNASnapshot(conf.activefilename)){
						CLEAR_INFO;;
					}
					continue;
				} 
				else {
					if (cur_file_index_old==-1) CLEAR_INFO;//Фон отображения скринов и информации очистка	
					 else CLEAR_INFO;
					cur_file_index_old=cur_file_index;    
				}
			}


		}


            led_trdos();


      } // while(1)
    software_reset();
}
//==========================================================================
//------------------------------------------
void file_select_trdos(void) // 
{
	paused = 0;
	//g_delay_ms(20);
	// is_menu_mode=false;
	is_menu_mode = true;
	
is_new_screen = true;
	//     MenuTRDOS(); // меню выбора и подключения образов trd
	uint8_t Drive = MenuBox_trd(64, 54, 22, 7, "Drive TR-DOS", 4, 0, 1);
	if (Drive < 5)
	{
		// Копируем строку длиною не более 10 символов из массива src в массив dst1.
		// strncpy (dst1, src,3);
        strncpy(conf.DiskName[Drive], files[cur_file_index], LENF);

        #if defined(TRDOS_0)
		if (Drive == 0) conf.sclConverted=1; // к диску а подключен TRD образ
		OpenTRDFile(conf.activefilename, Drive);
        write_protected = false; // защита записи отключена для TRD
        #endif

        #if defined(TRDOS_1)
          if (Drive == 0) conf.sclConverted=1; // к диску а подключен TRD образ
          OpenTRDFile(conf.activefilename,Drive);
          write_protected = false; // защита записи отключена для TRD
        //  WD1793_Init();
        #endif

	}
//is_new_screen = true;

	draw_main_window(); // восстановление текста
	draw_file_window();

    g_delay_ms(200);
	last_action = time_us_32();


}
////// TRDOS end
//++++++++++++++++++++++++++++++++++++++++++
//================================================================
//uint8_t config_data[32];
//-----------------------
void  config_init(void)
{

    enable_tape = false; // tap файл не подключен при запуске

   // char file_name_image[25];
    sprintf(temp_msg, "0:/zxcnf150.dat");//zxconfig.dat
    int fd = sd_open_file(&f, temp_msg, FA_READ);
    if (fd != FR_OK)
    {
        sd_close_file(&f);
        config_defain();
        return;
    }
    UINT bytesRead;
 //   fd = sd_read_file(&f, config_data, 32, &bytesRead);
      fd =  f_read(&f, &conf ,sizeof(conf),&bytesRead);//sd_read_file(&f, *conf ,sizeof(conf) , &bytesRead);
 //   if (bytesRead != 32) // если ошибка то конфиг по умолчанию
    if (fd)  // если ошибка то конфиг по умолчанию
    {
      sd_close_file(&f);
      config_defain();
      return ;
   
    }
    sd_close_file(&f);
//config_defain();

    conf.turbo=0; // при включении TURBO OFF!
    turbo_switch();



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

    int fd = sd_mkdir("0:/");
    if ((fd != FR_OK) && (fd != FR_EXIST))
    {
        MessageBox("      Error saving config      ", "", CL_LT_YELLOW, CL_RED, 1);
        return false;
    }

    MessageBox("       Saving config      ", "", CL_WHITE, CL_BLUE, 2);

 //   char file_name_image[25];
    sprintf(temp_msg, "0:/zxcnf150.dat");//zxconfig.dat

    fd = sd_open_file(&f, temp_msg, FA_CREATE_ALWAYS | FA_WRITE);
    if (fd != FR_OK)
    {
        sd_close_file(&f);
        return false;
    }
    UINT bytesWritten;
    fd = sd_write_file(&f, &conf, sizeof(conf), &bytesWritten);
    if (bytesWritten != 16)
    {
        sd_close_file(&f);
        return false;
    }
    sd_close_file(&f);

    return true;
}
//=========================================================
void help_zx(void)
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_off();


	draw_rect(0, 20, 318, 200, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(0, 20, 318, 200, CL_GRAY, false);				   // рамка 1
	draw_rect(0 + 2, 20 + 2, 318 - 4, 200 - 4, CL_GRAY, false); // рамка 2

	draw_rect(0 + 3, 20 + 3, 318 - 6, 8, CL_GRAY, true);			 // шапка меню
	draw_text(0 + 10, 20 + 3, "ZXSpeccy  Help", CL_BLACK, CL_GRAY); // шапка меню


	// меню выбора setup

   
     MenuBox_help(7, 24, 16, 17, menu_help, 17, 0, 1);
 
            im_z80_stop = false;
            is_menu_mode = false;
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();

}
//=========================================================
// turbo/normal
void turbo_switch(void)
{
    if (conf.turbo > 2)
        conf.turbo = 0;

     switch (conf.turbo)
     {
     case 0:
        ticks_per_cycle = CPU_KHZ / 3500; // 108
      //  ticks_per_frame = 71680;          // 71680- Пентагон //70908 - 128 +2A
     //   T_per_line =  224*1;
        break;
     case 1:
        ticks_per_cycle = CPU_KHZ / (3500*2);// 5250; // 
      //  ticks_per_frame = 71680;//*1.2 ;        // 71680- Пентагон //70908 - 128 +2A 1.5
     //   T_per_line = (224*1);
        break;   
      case 2:
        ticks_per_cycle = CPU_KHZ / 7000; //7000; // 54
      //  ticks_per_frame = 71680 ;    //=107520//120000;//71680*2 ;// 71680- Пентагон //70908 - 128 +2A
     //   T_per_line = 224*1;
        break;          

     default:
        break;
     }

}
//=========================================================
// скорость SD карты
void sd_speed(uint8_t x)
{
           uint spi_sd;
           switch (x)
           {
           case 0:
            spi_sd  = 25*1000000; 
            break;
           case 1:
            spi_sd = 12*1000000; 
            break;
           case 2:
            spi_sd = 10*1000000; 
            break;
            case 3:
            spi_sd  = 8*1000000; 
            break;
           default:
           spi_sd = 12*1000000; 
            break;
           }
spi_baudrate_sd(spi_sd) ;

}
//=========================================================
void setup_zx(void)
{
	im_z80_stop = true;
	is_menu_mode = true;
    hardAY_off();

#define w1 290
#define h1 180
#define x1 18
#define y1 20

	draw_rect(x1, y1, w1, h1, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(x1, y1, w1, h1, CL_GRAY, false);				   // рамка 1
	draw_rect(x1 + 2, y1 + 2, w1 - 4, h1 - 4, CL_GRAY, false); // рамка 2

	draw_rect(x1 + 3, y1 + 3, w1 - 6, 8, CL_GRAY, true);			 // шапка меню
    draw_text(x1 + 10, y1 + 3, FW_VERSION, CL_BLACK, CL_GRAY); // шапка меню
    
	char str[10];
	snprintf(str, sizeof str, "%dMHz", (int)(clock_get_hz(clk_sys) / 1000000));
	draw_text(x1 + 240, y1 + 3, str, CL_BLACK, CL_GRAY); // CPU

	if (PSRAM_AVAILABLE)
    {
         snprintf(temp_msg, sizeof temp_msg, "PSRAM %dMb", psram_size);
		draw_text(x1 + 160, y1 + 3, temp_msg, CL_BLACK, CL_GRAY);
    }
    else
	{
		conf.mashine = 0;
		draw_text(x1 + 160, y1 + 3, "NO PSRAM ", CL_BLACK, CL_GRAY);
	}


	// меню выбора setup
    while (1)
    {  
        draw_rect(30, 28+16, 240,140, CL_BLACK, true);				   // рамка 3 фон
        draw_text(x1 + 120, y1 + 27, menu_ram[conf.mashine], CL_GRAY, CL_BLACK);
        draw_text(x1 + 120, y1 + 37, menu_sound[conf.ay], CL_GRAY, CL_BLACK);
        draw_text(x1 + 120, y1 + 47,  menu_speed[conf.turbo], CL_GRAY, CL_BLACK);
        draw_text(x1 + 120, y1 + 97, menu_joy[conf.joyMode], CL_GRAY, CL_BLACK);
        //uint spi_get_baudrate(const spi_inst_t *spi);
        sprintf(temp_msg,"%d MBd",spi_baudrate()/1000000);
        draw_text(x1 + 128, y1 + 67, temp_msg, CL_GRAY, CL_BLACK); // fatfs_sd.c  uint x = spi_init(FATFS_SPI, FATFS_SPI_BRG  );
        if (conf.sound_ffd) draw_text(x1 + 128, y1 + 77, "OFF", CL_GRAY, CL_BLACK); 
        else draw_text(x1 + 128, y1 + 77, "ON", CL_GRAY, CL_BLACK); 

/*      
        if (conf.turbo) draw_text(x1 + 128, y1 + 47, "ON", CL_GRAY, CL_BLACK); 
        else draw_text(x1 + 128, y1 + 47, "OFF", CL_GRAY, CL_BLACK);  */

        draw_text(x1 + 120, y1 + 87, menu_autorun[conf.autorun], CL_GRAY, CL_BLACK); 
        
        //conf.ampl_tab
        draw_text(x1 + 120, y1 + 57, menu_ampl[conf.ampl_tab], CL_GRAY, CL_BLACK);







        
     static  uint8_t numsetup = 13;

// Только для режима TFT 11 пунктов меню
//#if defined(TFT)
     numsetup = MenuBox_bw(30, 28, 16, 14, menu_setup,14, numsetup, 1);

// для других режимов меню короче 10 пунктов
//#else
 //    numsetup = MenuBox_bw(30, 28, 16, 10, menu_setup, 10, numsetup, 1);
//#endif
//---
        if (PSRAM_AVAILABLE)
        {
            if (numsetup == M_RAM)
            {
                uint8_t x = MenuBox(90, 52, 17, 8, "RAM Seting", menu_ram, 8, conf.mashine, 1);
               if (x==0xff) continue;
                conf.mashine  = x;
                init_extram(conf.mashine);
               // zx_machine_init();
                continue;
            }
        }
        if (numsetup == M_SOUND)
        {
            uint8_t x = MenuBox(90, 52, 16, 6, "Sound Seting", menu_sound, 6, conf.ay, 1);
            // 0 soft AY
            // 1 soft TS
            // 2 hard AY
            // 3 hard TS
            // 4 i2s TS
           if (x==0xff) continue;
           if (conf.ay  == x) continue;// то же что и было
           uint8_t y = conf.ay;
            conf.ay  = x;

            select_audio(conf.ay); // переключение режимов вывода звука
            // сохранение и перезагрузка
             save_config();
            if ((conf.ay/2)  == (y/2)) continue;
             MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);
             software_reset();
            continue;
        }
if (numsetup == M_JOY) 
        {
           uint8_t x = MenuBox(74, 52, 18, 4, "Joystick",menu_joy, 4, conf.joyMode, 1);
           if (x==0xff) continue;
           conf.joyMode = x;
           continue;
        }

if (numsetup == M_SPI)
        {
          uint8_t x = MenuBox(94, 52, 14, 4, "SPI Baudrate",menu_spi, 4, conf.spi_bd, 1);
           if (x==0xff) continue;
           conf.spi_bd = x;
           sd_speed(conf.spi_bd);// 0..3 def 1
           continue;
        }

 if (numsetup == M_NOISE_FDD) //  sound - noise fdd
        {
           conf.sound_ffd  ^= true;
           continue;
        }

        if (numsetup == M_TURBO) // переключение NORMAL/TURBO
        {
            uint8_t x = MenuBox(94, 92, 17, 3, "Speed Mode", menu_speed, 3, conf.turbo, 1);
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
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();
            return;
            // g_delay_ms(1000);
        }
        if (numsetup == M_HARD_RESET) // Hard reset
        {
            MessageBox("  HARD RESET  ", "", CL_WHITE, CL_RED, 2);

            software_reset();
            im_z80_stop = false;
            is_menu_mode = false;
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();
            return;
 
        }

        if (numsetup == M_EXIT) // Exit
        {
            im_z80_stop = false;
            is_menu_mode = false;
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();
            return;

        }
        if (numsetup == 0xff) // exit
        {
            numsetup = 13;
            im_z80_stop = false;
            is_menu_mode = false;
            is_pause_mode = false;
            is_new_screen = false;
            hardAY_on();
            return;
            // g_delay_ms(1000);
        }

if (numsetup == M_AY_TABLE)
        {
            uint8_t x = MenuBox(90, 52, 14, 7, "AY Amplitudes", menu_ampl, 7, conf.ampl_tab , 1);

           if (x==0xff) continue;
            conf.ampl_tab  = x;
            init_vol_ay(conf.vol_bar);
          //  select_audio(conf.ay); // переключение режимов вывода звука
            continue;
        }






// Только для режима TFT 10-й пункт меню
#if defined(TFT)
    
if (numsetup == M_TFT_BRIGHT)
        {
          uint8_t x = MenuBox_tft(74, 84, 21, 3, "Bright TFT", 1);
           if (x==0xff) continue;
           continue;
        }

#endif
//---



        

    }
}
//==================================================================================================================================================
void slot_screen(uint8_t cPos)
{
    sprintf(conf.activefilename, "0:/save/%d_slot.Z80", cPos);
    if (!LoadScreenFromZ80Snapshot(conf.activefilename))
    {
        draw_rect(17 + FONT_W * 14, 16, 182, 191, COLOR_PIC_BG, true);
        draw_rect(17 + FONT_W * 14, 208, 182, 22, COLOR_BACKGOUND, true); // Фон отображения скринов
        draw_text(180, 77, "EMPTY SLOT", CL_GRAY, CL_BLACK);
    }
}
//===================================================================================================================================================
uint8_t MenuBox_sv(uint8_t xPos, uint8_t yPos, uint8_t lPos, uint8_t hPos, char *text,  uint8_t Pos, uint8_t cPos, uint8_t over_emul)
{
  if (over_emul)
    zx_machine_enable_vbuf(false);
  uint16_t lFrame = (lPos * FONT_W) + 10;
  uint16_t hFrame = ((1 + hPos) * FONT_H) + 20;
  // draw_rect(xPos+3,yPos+2,lFrame+3,hFrame+3,CL_GRAY,true);// тень
  draw_rect(xPos - 2, yPos - 2, lFrame + 4, hFrame + 4, CL_BLACK, false); // рамка 1
  draw_rect(xPos - 1, yPos - 1, lFrame + 2, hFrame + 2, CL_GRAY, false);  // рамка 2
  draw_rect(xPos, yPos, lFrame, hFrame, CL_BLACK, true);                  // рамка 3 фон
  draw_rect(xPos, yPos, lFrame, 7, CL_GRAY, true);                        // шапка меню
  draw_text(xPos + 10, yPos + 0, text, CL_PAPER, CL_INK);                 // шапка меню


draw_line(15 + FONT_W * 14, 14, 15 + FONT_W * 14, 236, CL_INK);// рамка картинки

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
		hardAY_off();
        uint8_t num = MenuBox_sv(38+16, 7, 31, 25, "SAVE",  25, 0, 1);

		if (num == 0xff) // exit
		{
			im_z80_stop = false;
			is_menu_mode = false;
            hardAY_on();
			return;
		}
    
		sprintf(save_file_name_image, "0:/save/%d_slot.Z80", num);
		sleep_ms(10);
		int fd = sd_mkdir("0:/save");
		if ((fd != FR_OK) && (fd != FR_EXIST))
		{
			MessageBox(" Error saving ","", CL_LT_YELLOW, CL_RED, 2);
		}
		else
		{

			MessageBox(" Quick saving... ","", CL_WHITE, CL_BLUE, 0);
			save_image_z80(save_file_name_image);
            if (num==0) save_config(); //если нулевой слот то сохраняем ещё и всю конфигурацию


		}
		
		im_z80_stop = false;
		is_menu_mode = false;
        hardAY_on();
		sleep_ms(3000);
	
		return;
	}
    //=================================================================================
	void save_all(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
		hardAY_off();
        
    
		sprintf(save_file_name_image, "0:/save/0_slot.Z80");
	//	sleep_ms(10);

 		int fd = sd_mkdir("0:/save");

		if ((fd != FR_OK) && (fd != FR_EXIST))
		{
			MessageBox(" Error saving ","", CL_LT_YELLOW, CL_RED, 2);
		}  
		else
		{

			MessageBox(" SAVE ALL RAM & CONFIG ","", CL_WHITE, CL_BLUE, 0);
			save_image_z80(save_file_name_image);
            sleep_ms(1000);
            save_config(); //если нулевой слот то сохраняем ещё и всю конфигурацию
		}
		
		im_z80_stop = false;
		is_menu_mode = false;
        hardAY_on();
		sleep_ms(1000);
	
		return;
	}
	//=================================================================================
	void load_slot(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
		im_ready_loading = false;
        hardAY_off();

		uint8_t num = MenuBox_sv(38+16, 7, 31, 25, "LOAD",  25, 0, 1);

		if (num == 0xff) // exit
		{
			im_z80_stop = false;
			is_menu_mode = false;
            hardAY_on();
          
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


        hardAY_on();
	}
//==============================================================================================
// AUTORUN
//==============================================================================================
void load_all(void)
	{
		im_z80_stop = true;
		is_menu_mode = true;
		im_ready_loading = false;
        hardAY_off();   
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

        hardAY_on();
	}
//===============================================================
    void load_trd(void)
    {

        hardAY_on();

        if (conf.sclConverted == 0)
        {
            write_protected = true; // защита записи для SCL
                                    //  strncpy(conf.DiskName[0], files[cur_file_index], LENF);// disk A
             strcpy(conf.activefilename, conf.Disks[0]);// disk A   
               file_type = 1;// scl            
            Run_file_scl(conf.activefilename, 0);
        }
        else // trd
        {
            conf.sclConverted = 1; // к диску а подключен TRD образ
            file_type = 0;// trd
                                   //	strncpy(conf.DiskName[0],"autorun.trd", LENF);
                                   //	 strcpy(conf.activefilename, "0:/autorun.trd"); // strcpy= "0:/autorun.trd";
                                   //  strncpy(conf.DiskName[0], files[cur_file_index], LENF);// disk A
           // if (conf.Disks[0][0] ==0 ) zx_cpu_ram[0]=zx_rom_bank[0]; // диска нет 128 BASIC  
               strcpy(conf.activefilename, conf.Disks[0]);// disk A
                       OpenTRDFile(conf.activefilename, 0);        
          //  OpenTRDFile(conf.activefilename, 0);
            write_protected = false; // защита записи отключена для TRD
        }
        im_z80_stop = false;
        is_menu_mode = false;
        zx_machine_enable_vbuf(true);
    }
//=========================================================
void disk_autorun(void)
{
	if (conf.autorun == 0)
{
   /*      sleep_ms(5000);
		im_z80_stop = false;
		is_menu_mode = false;
        zx_machine_enable_vbuf(true);
 */

		return;
}
	if (conf.autorun == 1) 
	{
	load_trd();
	return;
	}
	if (conf.autorun == 2)
	{
		//zx_machine_reset();
	//	sprintf(temp_msg, "0:/save/0_slot.Z80");
	//	if (load_image_z80(temp_msg))
			/* 			MessageBox("Loading slot...", temp_msg, CL_WHITE, CL_BLUE, 2);
					else
						MessageBox(" Error loading ", "", CL_LT_YELLOW, CL_RED, 2); */
			// im_z80_stop = false;
			// is_menu_mode = false;
//config_init(); //если нулевой слот то загружаем ещё и всю конфигурацию

//printf("QS SLOT1\n" );

		//	zx_machine_enable_vbuf(true);
	//	 sleep_ms(3000);
         load_all();

		return;
	}
}

//===============================================================
// индикатор работы trdos

#if defined(TRDOS_0) 
void led_trdos(void)
{
    gpio_put(LED_PIN, VG_PortFF & 0b01000000);

    if (!vbuf_en)
        return; // если экран эмуляции отключен то не шумим ffd
    if (VG_PortFF & 0b01000000)
    {
        icon[0] = 0x1F;
        draw_text_len(0, 240-7, icon, CL_LT_GREEN, CL_BLUE, 1); // 232
    }

    else
    {
           icon[0] = 0x20;
            draw_text_len(0,240-7,icon,CL_LT_GREEN ,CL_BLACK,1);//232
    }
    {
        //        if (trdos)
        {
            //    icon[0] = 0x1F;
            //     	draw_text_len(0,0,icon,CL_LT_GREEN ,CL_BLUE,1);//232
            //    ws2812_set_rgb(0, 4, 0);
        }
    }
}
#endif




#if defined(TRDOS_1) 
void led_trdos(void)
{
    
    
    gpio_put(LED_PIN, Requests  & 0b01000000);

    if (!vbuf_en)
        return; // если экран эмуляции отключен то не шумим ffd
  //  if ((GetWD1793_Status()==1))

  if (Requests & 0b01000000)  
    {
        icon[0] = 0x1F;
        draw_text_len(0, 240-8, icon, CL_LT_GREEN, CL_BLUE, 1); // 232
    }

  /*   else
    {
           icon[0] = 0x20;
            draw_text_len(0,240-7,icon,CL_LT_GREEN ,CL_BLACK,1);//232
    } */
    {
        //        if (trdos)
        {
            //    icon[0] = 0x1F;
            //     	draw_text_len(0,0,icon,CL_LT_GREEN ,CL_BLUE,1);//232
            //    ws2812_set_rgb(0, 4, 0);
        }
    }
}
#endif

// end  





//===============================================================

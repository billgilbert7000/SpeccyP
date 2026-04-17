#include "config.h"  
#include <stdio.h>
#include "hardware/gpio.h"


//-----------------
/* #if defined(NO_PSRAM_21)  

void config_defain(void)
{
         conf.version =CONFIG_VERSION;
        conf.autorun =0;
      //  conf.type_sound_filter= 1;// 0 off фильтр частот
        conf.turbo = 0;// turbo/normal
        conf.mashine = PENT128;
        conf.type_sound = 3;// hard turbo sound
        conf.joyMode = 0;
        conf.sound_fdd = 0;
        conf.spi_bd = 1;
        conf.tft_bright = 70;
       
        conf.autorun = 0; //off   1 trdos
        conf.DiskName[4][LENF+1]; // имя диска инфо
        conf.vol_ay = 15; //громкость soft AY
        conf.vol_i2s = 50; //громкость i2s
        
        conf.activefilename[0] = 0;

              conf.turbo=0; // при включении TURBO OFF!
      conf.tape_mode=0; // 0=fast, 1=slow
      turbo_switch();
}

#else */
//----------------------------------------------------------
// Конфигурация по умолчанию
//----------------------------------------------------------
#ifdef DEVICE_DEFAULT
void config_defain(void)
{

//conf.shift_img=12589;///(((16+40)*224)+48);

        conf.version =CONFIG_VERSION;
        conf.tft=TFT_9345I;// st7789

        conf.tft_invert=0;// TFT_INV=0 или 1

        conf.tft_rotate  = 0;//переворот TFT 
        conf.tft_rgb = 1; // Добавлен бит BGR (0x08)  00001000
        conf.vout=VIDEO_AUTO; // AUTO по умолчанию
   
        conf.hdmi_fdiv = 1.0; // 1.0->60Hz (cpu=252MHz)
        if (CPU_MHZ==378)
        conf.hdmi_fdiv = 1.5; // 1.5->60Hz (cpu=378MHz)

        conf.autorun =0;//off   1 trdos

        conf.mashine = PENT128;
        conf.trdos_version = 0;// версия TR-DOS
        conf.cpu_version = 0; // версия cpu z80

        conf.type_sound = 3;// 1 = soft turbo sound 3 = i2s turbo sound
        conf.vol_ay = DEFAULT_VOLUME_PWM; //громкость soft AY
        conf.vol_i2s = DEFAULT_VOLUME_I2S; //громкость i2s
        conf.vol_load = 16; //громкость аудио загрузки
        conf.joyMode = 0;
		conf.mouse_dpi = 1;
        conf.sound_fdd = 1;
		conf.audio_buster = AUDIO_BUSTER_DEFAULT;
       
        conf.spi_bd = 1;
        conf.tft_bright = 70;
         
       // conf.DiskName[4][LENF+1]=0; // имя диска инфо
       conf.DiskName[0][0]=0; // имя диска инфо
       conf.DiskName[1][0]=0; // имя диска инфо
       conf.DiskName[2][0]=0; // имя диска инфо
       conf.DiskName[3][0]=0; // имя диска инфо

        conf.pallete = 0;//номер палитры
        conf.activefilename[0] = 0;

      conf.turbo=0; // при включении TURBO OFF!
      conf.tape_mode=0; // 0=fast, 1=slow
}
#endif

#ifdef DEVICE_HDMI90_I2S
void config_defain(void)
{
        conf.version =CONFIG_VERSION;
        conf.vout=VIDEO_HDMI;// Видевыход 0-AUTO 1-VGA 2-HDMI 3-TFT 
        conf.tft=0;// ili9341
        conf.tft_invert=0;// TFT_INV=0 или 1
        conf.tft_rotate  = 0;//переворот TFT 
        conf.tft_rgb = 1; // Добавлен бит BGR (0x08)  00001000
        conf.tft_bright = 70;
        conf.autorun = 0; //off   1 trdos
        conf.turbo = 0;// turbo/normal
        conf.mashine = PENT128;
        conf.trdos_version = 0;// версия TR-DOS
        conf.cpu_version = 0; // версия cpu z80

        conf.type_sound = 3;// i2s turbo sound
        conf.joyMode = 0;
		conf.mouse_dpi = 1;
        conf.sound_fdd = 1;
		conf.audio_buster = AUDIO_BUSTER_DEFAULT;
        conf.hdmi_fdiv = 1.0;// 1.0-> 90Hz  (cpu=378MHz)
        conf.spi_bd = 1;
     
       // conf.DiskName[4][LENF+1]=0; // имя диска инфо
       conf.DiskName[0][0]=0; // имя диска инфо
       conf.DiskName[1][0]=0; // имя диска инфо
       conf.DiskName[2][0]=0; // имя диска инфо
       conf.DiskName[3][0]=0; // имя диска инфо
        conf.vol_ay = DEFAULT_VOLUME_PWM; //громкость soft AY
        conf.vol_i2s = DEFAULT_VOLUME_I2S; //громкость i2s
        conf.vol_load = 16; //громкость аудио загрузки
        conf.pallete = 0;//номер палитры
        conf.activefilename[0] = 0;

      conf.turbo=0; // при включении TURBO OFF!
      conf.tape_mode=0; // 0=fast, 1=slow
}
#endif

#ifdef DEVICE_PI_CARD
void config_defain(void)
{
        conf.version =CONFIG_VERSION;
         conf.vout=VIDEO_HDMI;// Видевыход 0-AUTO 1-VGA 2-HDMI 3-TFT  //2- только HDMI
        conf.tft=0;// ili9341
        conf.tft_invert=0;// TFT_INV=0 или 1
        conf.tft_rotate  = 0;//переворот TFT 
        conf.tft_rgb = 1; // Добавлен бит BGR (0x08)  00001000
        conf.tft_bright = 70;
        conf.autorun = 0; //off   1 trdos
        conf.turbo = 0;// turbo/normal
        conf.mashine = PENT128;
        conf.trdos_version = 0;// версия TR-DOS
        conf.cpu_version = 0; // версия cpu z80
        
        conf.type_sound = 3;// i2s turbo sound
        conf.joyMode = 0;
		conf.mouse_dpi = 1;
        conf.sound_fdd = 1;
		conf.audio_buster = AUDIO_BUSTER_DEFAULT;
        conf.hdmi_fdiv = 1.5;// 1.0-> 90Hz  (cpu=378MHz)
        conf.spi_bd = 1;
        
       // conf.DiskName[4][LENF+1]=0; // имя диска инфо
       conf.DiskName[0][0]=0; // имя диска инфо
       conf.DiskName[1][0]=0; // имя диска инфо
       conf.DiskName[2][0]=0; // имя диска инфо
       conf.DiskName[3][0]=0; // имя диска инфо
        conf.vol_ay = DEFAULT_VOLUME_PWM; //громкость soft AY
        conf.vol_i2s = DEFAULT_VOLUME_I2S; //громкость i2s
        conf.vol_load = 16; //громкость аудио загрузки
        conf.pallete = 0;//номер палитры
        conf.activefilename[0] = 0;

      conf.turbo=0; // при включении TURBO OFF!
      conf.tape_mode=0; // 0=fast, 1=slow
}
#endif

//======================================================
//========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =====================
//======================================================

 uint8_t g_gbuf[V_BUF_SZ];
// uint8_t g_gbuf[];
 uint8_t color_zx[16];
 bool vbuf_en;// экран эмуляции
//**********************************************
//Выравнивание структур данных:
__attribute__((aligned(4))) uint8_t RAM[16384*8]; //Реальная память куском 128Кб
//------------------------------------------------
// Joystick
 uint8_t joy_key_ext;
  bool joy_connected;
   uint8_t data_joy;
 uint8_t old_data_joy;
//--------------------------------------------------
// fdd & trdos
 uint32_t sclDataOffset;
//unsigned char* track0;
uint8_t track0 [0x0900];

 bool write_protected;
 uint8_t DriveNum;
 uint8_t sound_track;

uint8_t dirs[DIRS_DEPTH][LENF];
uint8_t sd_buffer[SD_BUFFER_SIZE]; //буфер для работы с файлами
char files[MAX_FILES][LENF];
char dir_patch[DIRS_DEPTH*(LENF+16)];
int init_fs;
uint8_t NoDisk;
uint8_t file_type[5];
uint8_t file_attr[5];

//---------------------------------
// tape loader
 uint32_t seekbuf;
 char TapeName[17];
 bool enable_tape;
 //-------------------------------
//переменные состояния спектрума
 uint8_t zx_Border_color;
//---------------------------------

uint32_t size_psram;
bool    psram_type;
uint8_t type_psram;

//---------------------------------
// other
 char temp_msg[80]; // Буфер для вывода строк

 bool main_nmi_key = false;
//=====================================================
// данные конфигурационного файла
 struct data_config conf;
//void config_defain(void);// процедура конфигурации по умолчанию

//----------------------------------------------------------
bool trdos=0;

 //uint64_t    t2=0;// tr-dos
 uint64_t tindex =0;

//===========================================================
// сообщения внизу и громкость
  uint16_t wait_msg;
  uint8_t msg_bar; 
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

uint8_t mouse[8] ;
bool flag_usb_kb;
//--------------------------------------------
// изображение видеодрайвер
 int T_per_line; 
 char dir_patch_info [DIRS_DEPTH*(LENF+16)];
//---------------------------------------------
//u_int16_t keytime;// нажатие кнопки при автозапуске 
int  ScreenShot_Y=40; // координата Y для вывода скриншота

//---------- PSRAM -----------------  
 bool psram_avaiable;
//----------------------------------------------------
///////////////// nmi
 uint8_t z80_pc;

 extern uint8_t hardAY_on_off;
 extern uint8_t beep_data_old;
 extern uint8_t beep_data;

 //=================================================================
 // Автодетект видеовыхода

 uint8_t video_autodetect(){
	uint8_t readings;
	uint8_t pull_up=0;
	uint8_t pull_down=0;
	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_IN);
		gpio_pull_up(idx);
	}
	sleep_ms(10);
	
	for(uint8_t rep=0;rep<5;rep++){
		readings=0;
		for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
			readings|=((uint8_t)gpio_get(idx))<<(idx-6);
		}
		sleep_ms(10);
		pull_up+=readings;
	}

	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_deinit(idx);
		sleep_ms(1);
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_OUT);
	}

	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_IN);
		gpio_pull_down(idx);
	}
	sleep_ms(10);
	for(uint8_t rep=0;rep<5;rep++){
		readings=0;
		for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
			readings|=((uint8_t)gpio_get(idx))<<(idx-6);
		}
		sleep_ms(10);
		pull_down+=readings;
	}

	for(uint8_t idx=beginVideo_PIN;idx<(beginVideo_PIN+6);idx++){
		gpio_deinit(idx);
		sleep_ms(1);
		gpio_init(idx);
		gpio_set_dir(idx,GPIO_OUT);
	}

 //conf.tft = 0;

     conf.pullup_pin_video = pull_up;
	if((pull_up==0x13)&&(pull_down==0x00)){ //pull_up=0xFF tft ips  pull_up=0x13 tft ips  
     //   conf.tft = 2;
		return VIDEO_TFT;
	} else 

	if((pull_up==0xFF)&&(pull_down==0x00)){ //pull_up=0xFF tft ips  pull_up=0x13 tft ips  
     //   conf.tft = 0;
		return VIDEO_TFT;
	} else 
	if((pull_up==0x00)&&(pull_down==0x00)){
		return VIDEO_VGA;
	} else
	if((pull_up>0x00)&&(pull_down>0x00)){
		return VIDEO_HDMI;
	}
    return VIDEO_TFT;
//	return VIDEO_VGA;
}
//===============================================================================
// DeepSeek
/*
#include "ff.h"
#include <string.h>
#include <stdio.h>

//----------------------------------------------------------
// Сохранение конфигурации с комментариями
//----------------------------------------------------------
void config_save(const char *filename) {
    FIL fil;
    FRESULT fr;
    
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK) return;

    // Системные параметры
    f_printf(&fil, "[System]\n");
    f_printf(&fil, "# Версия конфигурации (1-65535)\n");
    f_printf(&fil, "version = %u\n\n", conf.version);
    
    f_printf(&fil, "# Частота CPU Z80 (Гц)\n");
    f_printf(&fil, "z80_freq_hz = %lu\n\n", conf.z80_freq_hz);

    // Видео параметры
    f_printf(&fil, "[Video]\n");
    f_printf(&fil, "# Тип видеовыхода (0-AUTO, 1-VGA, 2-HDMI, 3-TFT)\n");
    f_printf(&fil, "vout = %u\n\n", conf.vout);
    
    f_printf(&fil, "# Тип дисплея (0-ili9341, 1-st7789)\n");
    f_printf(&fil, "tft = %u\n\n", conf.tft);
    
    f_printf(&fil, "# Инверсия цветов (0-OFF, 1-ON)\n");
    f_printf(&fil, "tft_invert = %s\n\n", conf.tft_invert ? "ON" : "OFF");
    
    f_printf(&fil, "# Поворот дисплея (0-0°, 1-180°)\n");
    f_printf(&fil, "tft_rotate = %s\n\n", conf.tft_rotate ? "ON" : "OFF");
    
    f_printf(&fil, "# Цветовой режим (0-RGB, 1-BGR)\n");
    f_printf(&fil, "tft_rgb = %s\n\n", conf.tft_rgb ? "BGR" : "RGB");
    
    f_printf(&fil, "# Яркость дисплея (0-100)\n");
    f_printf(&fil, "tft_bright = %u\n\n", conf.tft_bright);

    // Аудио параметры
    f_printf(&fil, "[Audio]\n");
    f_printf(&fil, "# Усилитель звука (0-65535)\n");
    f_printf(&fil, "audio_buster = %u\n\n", conf.audio_buster);
    
    f_printf(&fil, "# Громкость AY (0-100)\n");
    f_printf(&fil, "vol_ay = %u\n\n", conf.vol_ay);
    
    f_printf(&fil, "# Громкость I2S (0-100)\n");
    f_printf(&fil, "vol_i2s = %u\n\n", conf.vol_i2s);
    
    f_printf(&fil, "# Звук FDD (0-OFF, 1-ON)\n");
    f_printf(&fil, "sound_fdd = %u\n\n", conf.sound_fdd);

    // Настройки машины
    f_printf(&fil, "[Machine]\n");
    f_printf(&fil, "# Тип ZX Spectrum (0-255)\n");
    f_printf(&fil, "mashine = %u\n\n", conf.mashine);
    
    f_printf(&fil, "# Режим AY (0-255)\n");
    f_printf(&fil, "ay = %u\n\n", conf.type_sound);
    
    f_printf(&fil, "# Автозагрузка (0-OFF, 1-ON)\n");
    f_printf(&fil, "autorun = %u\n\n", conf.autorun);
    
    f_printf(&fil, "# Турбо режим (0-OFF, 1-ON)\n");
    f_printf(&fil, "turbo = %u\n\n", conf.turbo);
    
    // Периферия
    f_printf(&fil, "[Periphery]\n");
    f_printf(&fil, "# Режим джойстика (0-255)\n");
    f_printf(&fil, "joyMode = %u\n\n", conf.joyMode);
    
    f_printf(&fil, "# DPI мыши (100-16000)\n");
    f_printf(&fil, "mouse_dpi = %u\n\n", conf.mouse_dpi);
    
    f_printf(&fil, "# Скорость SPI (0-255)\n");
    f_printf(&fil, "spi_bd = %u\n\n", conf.spi_bd);
    
    f_printf(&fil, "# Палитра (0-255)\n");
    f_printf(&fil, "pallete = %u\n\n", conf.pallete);
    
    f_printf(&fil, "# Конвертация SCL (0-OFF, 1-ON)\n");
    f_printf(&fil, "FileAutorunType = %s\n\n", conf.FileAutorunType ? "true" : "false");

    // Хранилище
    f_printf(&fil, "[Storage]\n");
    for (int i = 0; i < 4; i++) {
        f_printf(&fil, "# Имя диска %d\n", i+1);
        f_printf(&fil, "DiskName%d = \"%s\"\n", i, conf.DiskName[i]);
        
        f_printf(&fil, "# Путь к диску %d\n", i+1);
        f_printf(&fil, "Disks%d = \"%s\"\n\n", i, conf.Disks[i]);
    }
    
    f_printf(&fil, "# Активный файл\n");
    f_printf(&fil, "activefilename = \"%s\"\n", conf.activefilename);

    f_close(&fil);
}

//----------------------------------------------------------
// Загрузка конфигурации
//----------------------------------------------------------
void config_load(const char *filename) {
    FIL fil;
    char line[256];
    char *key, *value;
    int index;
    
    if (f_open(&fil, filename, FA_READ) != FR_OK) {
    //    config_default();
        return;
    }

    while (f_gets(line, sizeof(line), &fil)) {
        // Пропуск комментариев и пустых строк
        if (line[0] == '#' || line[0] == '\n' || line[0] == '[') continue;
        
        // Разделение ключ-значение
        key = strtok(line, " =\t");
        value = strtok(NULL, "\"\n\r\t");
        
        if (!key || !value) continue;

        // Парсинг параметров
        if (strcmp(key, "version") == 0) conf.version = atoi(value);
        else if (strcmp(key, "z80_freq_hz") == 0) conf.z80_freq_hz = atol(value);
        else if (strcmp(key, "vout") == 0) conf.vout = atoi(value);
        else if (strcmp(key, "tft") == 0) conf.tft = atoi(value);
        else if (strcmp(key, "tft_invert") == 0) conf.tft_invert = (strcmp(value, "ON") == 0) ? 1 : 0;
        // ... аналогично для остальных параметров
        
        // Обработка строковых массивов
        if (sscanf(key, "DiskName%d", &index) == 1 && index >= 0 && index < 4) {
            strncpy(conf.DiskName[index], value, LENF);
            conf.DiskName[index][LENF] = '\0';
        }
        else if (sscanf(key, "Disks%d", &index) == 1 && index >= 0 && index < 4) {
            strncpy(conf.Disks[index], value, DIRS_DEPTH*(LENF+16));
            conf.Disks[index][DIRS_DEPTH*(LENF+16)-1] = '\0';
        }
        else if (strcmp(key, "activefilename") == 0) {
            strncpy(conf.activefilename, value, sizeof(conf.activefilename));
            conf.activefilename[sizeof(conf.activefilename)-1] = '\0';
        }
    }
    
    f_close(&fil);
}
*/
//---------------------------------------------------------------------
#if defined  GENERAL_SOUND

uint8_t tx_buffer[64];// буфер picobus

bool flag_gs;

uint8_t command_gs=0xff;    // Порт 1
uint8_t data_zx=0xff;
uint8_t data_gs=0xff;
uint8_t status_gs=0xff;     // Порт 4: D0 - флаг команд, D7 - флаг данных

#ifdef Z_CONTROLER 
uint8_t z_controler_cs = 0xff;
#endif

#endif

// Blink LED GPIO 25  for test

#ifdef LEDBLINK
void led_blink(void)
    {
        #if LED_BOARD != 255
    	static bool led_indicator = 0;
        gpio_put(LED_BOARD, led_indicator);
        led_indicator = !led_indicator;
        #endif

    }
     #endif
#include "config.h"  
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/vreg.h"

uint8_t __in_flash() table_voltage[] = {55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,150,160,165};

//----------------------------------------------------------
// Конфигурация по умолчанию
//----------------------------------------------------------
#ifdef DEVICE_DEFAULT
void config_defain(void)
{
        conf.version =CONFIG_VERSION;
        conf.voltage = VOLTAGE;// Possible voltage values that can be applied to the regulator

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
      conf.beep_mode=0; // 0 - pwm  , 1 - gpio
}
#endif

#ifdef DEVICE_HDMI90_I2S
void config_defain(void)
{
        conf.version =CONFIG_VERSION;
        conf.voltage = VOLTAGE;// Possible voltage values that can be applied to the regulator
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
      conf.beep_mode=0; // 0 - pwm  , 1 - gpio
}
#endif

#ifdef DEVICE_PI_CARD
void config_defain(void)
{
        conf.version =CONFIG_VERSION;
        conf.voltage = VOLTAGE;// Possible voltage values that can be applied to the regulator
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
      conf.beep_mode=0; // 0 - pwm  , 1 - gpio
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
#ifdef RP2350_256K
__attribute__((aligned(4))) uint8_t RAM[16384*16]; //Реальная память куском 256Кб
#else
__attribute__((aligned(4))) uint8_t RAM[16384*8]; //Реальная память куском 128Кб
#endif

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
  uint8_t beep_pin;
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

//####################################################   конфиг
// Вспомогательные функции для парсинга INI
static char* trim_whitespace(char *str) {
    char *end;
    
    // Удаление пробелов в начале
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') {
        str++;
    }
    
    if (*str == 0) return str;
    
    // Удаление пробелов в конце
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = 0;
        end--;
    }
    
    return str;
}

static char* get_value(char *line) {
    char *delim = strchr(line, '=');
    if (delim) {
        return trim_whitespace(delim + 1);
    }
    return NULL;
}

static bool parse_uint8(const char *str, uint8_t *value) {
    if (!str) return false;
    char *endptr;
    long val = strtol(str, &endptr, 0);
    if (endptr == str || val < 0 || val > 255) return false;
    *value = (uint8_t)val;
    return true;
}

static bool parse_uint32(const char *str, uint32_t *value) {
    if (!str) return false;
    char *endptr;
    unsigned long val = strtoul(str, &endptr, 0);
    if (endptr == str) return false;
    *value = (uint32_t)val;
    return true;
}

static bool parse_float(const char *str, float *value) {
    if (!str) return false;
    char *endptr;
    float val = strtof(str, &endptr);
    if (endptr == str) return false;
    *value = val;
    return true;
}

//#############################################################

// Сохранение конфигурации в INI файл
bool config_ini_save(const char *filename) {
    FIL file;
    FRESULT res;
    UINT bytes_written;
    int offset;
    res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
     //   printf("Config save: cannot create file %s (error %d)\n", filename, res);
        return false;
    }
    
    //Voltage
    uint8_t u = 130;
    if (conf.voltage == 15) u=130;
    else if (conf.voltage==16) u=135;
    else if (conf.voltage==17) u=140;
    else if (conf.voltage==18) u=150;
    else if (conf.voltage==19) u=160;

    // Заголовок файла
    offset = snprintf(sd_buffer, sizeof(sd_buffer),
        "; SpeccyP Configuration\n"
        "; =====================\n"
        "[system]\n"
        "; Version (do not modify)\n"
        "version = %llu\n\n"

        "; Voltage 1.30V=130 ,1.35V=135, 1.40V=140, 1.50V=150, 1.60V=160 \n"
        "voltage = %u\n\n"

        "[video]\n"
        "; 0=AUTO 1=VGA, 2=HDMI, 3=TFT\n"
        "video_out = %u\n"
        "; TFT type: 0=ILI9341, 1=ST7789, 2=ILI9341_IPS\n"
        "tft_type = %u\n"
        "; TFT inversion: 0=normal, 1=inverted\n"
        "tft_invert = %u\n"
        "; TFT rotation: 0=0 degrees, 192=180 degrees\n"
        "tft_rotate = %u\n"
        "; TFT color mode: 0=RGB, 1=BGR\n"
        "tft_rgb = %u\n"
        "; TFT backlight  0-100%%\n"
        "tft_brightness = %u\n"
        "; HDMI frequency divider (1.0=90Hz, 1.5=60Hz)\n"
        "hdmi_divider = %.2f\n",

        conf.version,
        u,               // conf.voltage,
        conf.vout,
        conf.tft,
        conf.tft_invert,
        conf.tft_rotate,
        conf.tft_rgb,
        conf.tft_bright,
        conf.hdmi_fdiv

    );
    
    f_write(&file, sd_buffer, offset, &bytes_written);

    f_close(&file);
    
  //  printf("Config saved to %s\n", filename);
    return true;
}

// Загрузка конфигурации из INI файла
bool config_ini_load(const char *filename) {
    FIL file;
    FRESULT res;
   
    //char line[512];
    char section[32] = "";
    
    // Проверка существования файла
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
     //   printf("Config file %s not found, creating default\n", filename);
        config_defain();
        config_ini_save(filename);
        return true;
    }
    
    // Загружаем значения по умолчанию
  //  config_defain();
    
    // Чтение файла построчно
    while (f_gets(sd_buffer, 512, &file)) {
        char *trimmed = trim_whitespace(sd_buffer);
        
        // Пропуск пустых строк и комментариев
        if (trimmed[0] == '\0' || trimmed[0] == ';' || trimmed[0] == '#') {
            continue;
        }
        
        // Определение секции
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (end) {
                *end = '\0';
                strncpy(section, trimmed + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }
        
        // Парсинг ключ=значение
        char *delim = strchr(trimmed, '=');
        if (!delim) continue;
        
        *delim = '\0';
        char *key = trim_whitespace(trimmed);
        char *value = trim_whitespace(delim + 1);
        
        // Обработка значений по секциям
        if (strcmp(section, "system") == 0) {
            if (strcmp(key, "version") == 0) NULL ;//parse_uint8(value, (uint8_t*)&conf.version);
            else if (strcmp(key, "voltage") == 0)// parse_uint8(value, &conf.voltage);
            //Voltage
            {
               uint8_t u; 
               parse_uint8(value, &u);
               conf.voltage=VOLTAGE;
               if (u == 130) conf.voltage=15;
               else if (u==135) conf.voltage=16;
               else if (u==140) conf.voltage=17;
               else if (u==150) conf.voltage=18;
               else if (u==160) conf.voltage=19;
            }
        }

        else if (strcmp(section, "video") == 0) {
            if (strcmp(key, "video_out") == 0) parse_uint8(value, &conf.vout);
            else if (strcmp(key, "tft_type") == 0) parse_uint8(value, &conf.tft);
            else if (strcmp(key, "tft_invert") == 0) parse_uint8(value, &conf.tft_invert);
            else if (strcmp(key, "tft_rotate") == 0) parse_uint8(value, &conf.tft_rotate);
            else if (strcmp(key, "tft_rgb") == 0) parse_uint8(value, &conf.tft_rgb);
            else if (strcmp(key, "tft_brightness") == 0) parse_uint8(value, &conf.tft_bright);
            else if (strcmp(key, "hdmi_divider") == 0) parse_float(value, &conf.hdmi_fdiv);
        }

    }
    
    f_close(&file);
 //   printf("Config loaded from %s\n", filename);
    return true;
}
//##################################################################
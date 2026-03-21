


#include "string.h"
#include "stdbool.h"

#include "zx_machine.h"
#include "../screen_util.h"
#include "util.h"
#include "../aySoft.h"
#include "../util_tap.h"
// rom
#include "rom/rom48original.h"//rom 48k original zx spectrum 48
#include "rom/rom48k.h"// 48 kb
#include "rom/rom128k.h"// 128 kb

#include "rom/trdos504t.h"// rom trdos 504
#include "rom/trdos505d.h"// rom trdos 505d


//#include "rom/divmmc.h"// divmmc
//#include "rom/test.h"// тестовое ПЗУ
//#include "rom/basic48lg.h"//rom 48k basic48 Looking Glass ROM
//#include "rom/trdos503.h"// rom trdos 503

#include "rom/navigator_sm508.h"// сервис монитор навигатора

#include "rom/trdos604q.h" // rom navigator
#include "rom/rom48Q.h"// 48 kb   quorum
#include "rom/rom128Q.h"// 128 kb quorum


//#include "rom/service.h"// сервис монитор пентагон 

#include "rom/romScorpion295.h"// Scorpion ZS256
//#include "rom/romScorpion.h"// Scorpion ZS256 Old

//###
#include <Z/constants/pointer.h> /* Z_NULL */
#include "Z80.h"
//###

#include "../g_config.h"
#include "hardware/structs/systick.h"
//#include "../util_tap.h"

#include "../usb_key.h"// это добавить


#ifdef MURM1
#include "psram_spi.h"
#endif


#include "../config.h"

//##########################################################



char tmp_opcode[16];
uint16_t dis_adres;
uint16_t address_pc;

unsigned long prev_ticks, cur_ticks;


//bool z80_gen_nmi_from_main = false;
bool im_z80_stop = false;
bool im_ready_loading = false;
//bool vbuf_en=true;

////////////////////////////////

uint8_t *zx_ram_bank[8]; // Хранит адреса 8ми банков памяти
uint8_t *zx_cpu_ram[4];  // Адреса 4х областей памяти CPU при использовании страниц uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))



// tr-dos
extern uint8_t wd1793_PortFF;
//bool trdos=0;
//uint8_t VG_PortFF = 0x00;
////////////////////////////////

uint32_t zx_img_ptr ;
///////////////////////////////
uint32_t ticks_per_cycle;//
uint32_t ticks_per_frame;// 71680- Пентагон //70908 - 128 +2A
	//смещение начала изображения от прерывания
//int shift_img;//=(16+40)*224+42;////8888;////Пентагон=(16+40)*224+48;

int inx_tick_int;
//uint8_t ticks_per_int;
bool int_enable;
///////////////////////////////
uint8_t port_ff =0xff;
//uint8_t zx_machine_last_out_7ffd;
uint8_t zx_machine_last_out_fffd;
uint8_t zx_7ffd_lastOut=0;
uint8_t zx_1ffd_lastOut=0;
uint8_t zx_0000_lastOut=0x20;
uint8_t zx_aff7_lastOut=0;
// Пентагон с кеш
uint8_t cash_f=0;
//uint8_t dos=1;
uint8_t rom;
uint8_t zx_RAM_bank_7ffd =0x00000000;
uint8_t zx_RAM_bank_1ffd =0x00000000;
uint8_t zx_RAM_bank_dffd =0x00000000;
uint16_t zx_RAM_bank_ext8 =0x00;
uint32_t zx_RAM_bank_active=0;
uint8_t pent_config=0;

 Z80 cpu;


  ZX_Input_t zx_input;
bool zx_state_48k_MODE_BLOCK=false;


static uint32_t zx_colors_2_pix32[384];//предпосчитанные сочетания 2 цветов
static uint8_t* zx_colors_2_pix=(uint8_t*)&zx_colors_2_pix32;//предпосчитанные сочетания 2 цветов


//uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
uint8_t* zx_video_ram;//4 области памяти CPU

 //uint8_t* zx_ram_bank[8];//Хранит адреса 8ми банков памяти
uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

typedef struct zx_vbuf_t
{
	uint8_t* data;
	bool is_displayed;
}zx_vbuf_t;

zx_vbuf_t zx_vbuf[ZX_NUM_GBUF];
zx_vbuf_t* zx_vbuf_active;

uint8_t atr0;
uint8_t atr1;
//выделение памяти может быть изменено в зависимости от платформы
//uint8_t RAM[16384*8]; //Реальная память куском 128Кб
//uint8_t RAM[16384*8]; //Реальная память куском 32Кб
//zx_cpu_ram_5 [16384];
//zx_cpu_ram_7 [16384];

//uint8_t VBUFS[ZX_SCREENW*ZX_SCREENH*ZX_NUM_GBUF*ZX_BPP/8];
//=================================================
// переменные экраной области
	uint8_t* p_zx_video_ram=NULL;
	uint8_t* p_zx_video_ramATTR=NULL;
	uint8_t* p_zx_video_ram5=NULL;
	uint8_t* p_zx_video_ramATTR5=NULL;
	uint8_t* p_zx_video_ram7=NULL;
	uint8_t* p_zx_video_ramATTR7=NULL;
    uint64_t inx_tick_screen_ff;// счетчик тактов экрана для порта FF
//###############################################


//#########################################################################
//######################################################################



//порт атрибутов
uint8_t port_atr(void)
{//zx_Border_color =0xaa; //71680 
	 uint16_t line = (inx_tick_screen_ff / 280) - 63;
	 if (line >= 192) return 0xFF;// T/228-63

    uint16_t halfpix = inx_tick_screen_ff % 280;// остаток деления 
	 if ((halfpix >= 128) || (halfpix & 0x04)) return 0xFF;

    int hpoffset = (halfpix >> 2) + ((halfpix >> 1) & 0x01);;
   
   // if (halfpix & 0x01) return line;// возвращает номер отображаемой строки
   //   return line;// возвращает номер отображаемой строки

            uint8_t c=*(p_zx_video_ramATTR+hpoffset);//получение значения переменной на которую указывает указатель
		//	uint8_t b=*p_zx_video_ram+hpoffset;//получение значения переменной на которую указывает указатель

    if (halfpix & 0x01) return c;// возвращает значение атрибута 
  return c;// b возвращает значение бита в экранной области

}
//=================================================
uint8_t fast(zx_keyboardDecode)(uint8_t addrH)
{
	
	//быстрый опрос
	
	switch (addrH)
	{
		case 0b11111111: return 0xff;break;
		case 0b11111110: return ~zx_input.kb_data[0];break;
		case 0b11111101: return ~zx_input.kb_data[1];break;
		case 0b11111011: return ~zx_input.kb_data[2];break;
		case 0b11110111: return ~zx_input.kb_data[3];break;
		case 0b11101111: return ~zx_input.kb_data[4];break;
		case 0b11011111: return ~zx_input.kb_data[5];break;
		case 0b10111111: return ~zx_input.kb_data[6];break;
		case 0b01111111: return ~zx_input.kb_data[7];break;
		
	}
	
	//несколько адресных линий в 0 - медленный опрос
	uint8_t dataOut=0;
	
	for(uint8_t i=0;i<8;i++)
	{
		if ((addrH&1)==0) dataOut|=zx_input.kb_data[i];//работаем в режиме нажатая клавиша=1
		addrH>>=1;
	};
	
	return ~dataOut;//инверсия, т.к. для спектрума нажатая клавиша = 0;
};
//=================================================================================
// TR-DOS
//====================
void fast (trdos_out)(uint8_t port, uint8_t val)
	{
         #ifdef  RTC_SMUC// теневой порт
         if (port16  == 0xFFBA) {out_GSP(RTC_WRITE_OUT_FFBA,  val); return;}//rtc_write=(val>>7); return;} // D7 
         if (port16  == 0xDFBA) {out_GSP(RTC_WRITE_OUT_DFBA,  val); return;}//данные регистра часов
         #endif

        if ((port & 0x1f) > 0)
		{
	
			uint8_t btemp = (port >> 4);
			if (btemp < 8)
            {
		 	if ((port == 0x1f) && ((val == 0xFC) || (val == 0x1C))) val = 0x18;//???
            WD1793_Write((port >> 5) & 0b11, val); // Write to 0x7F 0x5F 0x3F 0x1F port

			}
	      else
			{
				if (btemp == 0x0F)
                wd1793_PortFF = val;
			}
         return;	// если tr-dos то порты только его
						   
		}

	} // tr-dos
//=================================================================================
// Scorpion 7ffd 1ffd
void fast(rom_select)(void)
{
//#define ROM128 0
//#define ROM48  1
//#define ROM_TRDOS 2
//#define ROM_SM  3


switch (conf.mashine)
{
//	u_int8_t  dsg;
case NOVA256:
	//	rom_n =  ((zx_7ffd_lastOut & 0x10)>>4) | ((zx_0000_lastOut & 0x20)>>5) ;// 0001.0000 0000.1000 0000.0100 0000.0010 0000.0001
	//	zx_cpu_ram[0] =  zx_rom_bank[ table_nova256 [rom_n] ];

if ((zx_0000_lastOut&0b00100000) == 0)
{
	rom=3;
 zx_cpu_ram[0] = zx_rom_bank[3]; 
}
	else 
	{
		rom=(zx_7ffd_lastOut & 0x10)>>4; 
		zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
	} 
	return;
	break;

case SCORP256:

if ((zx_1ffd_lastOut & 0x02) == 0x02)
{
	rom=3;
 zx_cpu_ram[0] = zx_rom_bank[3]; 
 //trdos = true;
}
	else 
	{
		rom=(zx_7ffd_lastOut & 0x10)>>4;
	 zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
	// trdos = false;
	}
	return;
	break;


default:
         rom=(zx_7ffd_lastOut & 0x10)>>4;
         zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 

	return;
	break;

} 
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//=================================================================================
//функции чтения памяти и ввода-вывода
    // страница 0 psram  // 0x00000
	// страница 1 psram  // 0x04000
	// страница 2   ram  // 0x08000   0x8000-0xbfff
	// страница 3 psram  // 0x0c000   0xc000-0xffff
	// страница 4 psram  // 0x10000   0xc000-0xffff
	// страница 5   ram  // 0x14000   0xc000-0xffff  экранная область
	// страница 6 psram  // 0x18000   0xc000-0xffff
    // страница 7   ram  // 0x1c000   0xc000-0xffff  вторая экранная область
    // страница 8 psram  // 0x20000   0xc000-0xffff
    // ...
	// страница 0xff psram  // 0x3fc000   0x3fc000-0x7fffff 8Mb
//###
/* byte (*RdZ80)(word) ; // определение указателя на функцию
void (*WrZ80)(word,byte) ; // определение указателя на функцию
byte (*InZ80)(register word); // определение указателя на функцию
void (*OutZ80)(register word,register byte); // определение указателя на функцию */
//###
// чтение из памяти 128 стандарт
inline static zuint8 fast(read_z80)(Machine *self, zuint16 addr)
{
  //  return 0;
    //uint8_t x = (addr >> 14);
   
	return zx_cpu_ram[(zuint8)(addr >> 14)][addr & 0x3fff];
}

//---------------------------------------------------------------------------------------------------------------
// запись в память 128 стандарт
//inline static 
inline static void fast(write_z80)(Machine *self, uint16_t addr, uint8_t val)
{
   // return;
	uint8_t x = (addr >> 14);
    if(x == 0) return; // ПЗУ
	zx_cpu_ram[x][addr & 0x3fff] = val;												 
}

//######################################################################################
//PSRAM_BOARD // для расширенной памяти на rp2040 и rp2350 с PSRAM на плате MURM1
#ifdef MURM1
// чтение из памяти
inline static uint8_t fast(read_z80_ext)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
         return (zx_1ffd_lastOut & 0x01)           // Быстрая проверка флага
                ? zx_ram_bank[0][masked_addr]      // RAM
                : zx_cpu_ram[0][masked_addr];      // ROM
	    }
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
        {
    	//	 if (zx_RAM_bank_active > 7)// 0b 1111 1000
		if (zx_RAM_bank_active & 0xf8)  return read8psram((uint32_t)(zx_RAM_bank_active << 14) | masked_addr); 
        }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//##########################################################################################
// запись в память
inline static void fast(write_z80_ext)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
             if(zx_1ffd_lastOut & 0x01) {zx_ram_bank[0][masked_addr] = val;}
            return;  // Выход для ROM/RAM банка 0
        }
  
    if(x == 3) 
        {
		if (zx_RAM_bank_active & 0xf8) // >7
		    {
            write8psram((uint32_t)(zx_RAM_bank_active << 14) | (masked_addr), val);
		    return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
//##################################################################################
// управление 512 банками памяти 8 Мегабайт
// чтение из памяти 8Mb
inline static uint8_t fast(read_z80_ext8)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) return zx_cpu_ram[0][addr & 0x3fff]; // ПЗУ
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
    if (zx_RAM_bank_active > 7)
	//	if (zx_RAM_bank_active & 0x01f8) // 0b 1111 1000
		{
			return read8psram(((zx_RAM_bank_active) << 14) | masked_addr); // 0xc000-0xffff память больше 128
		}
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//##########################################################################################
// запись в память 8Mb
inline static void fast(write_z80_ext8)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0)  return;  // Выход для ROM/RAM банка 0
    if(x == 3) 
        {
            if (zx_RAM_bank_active > 7)
            //	if (zx_RAM_bank_active & 0x01f8) //
		    {
                write8psram(((zx_RAM_bank_active) << 14) | masked_addr, val); // 0xc000-0xffff память больше 128
		        return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
//##################################################################################
// Пентагон 512 с КЕШ
//##################################################################################
// чтение из памяти Пентагон 512 с кеш 
inline static uint8_t fast(read_z80_cash)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
            if ((cash_f) == 0)
            {
              return zx_cpu_ram[0][masked_addr]; // ПЗУ	
            }
            if (zx_7ffd_lastOut & 0x10)
            return read8psram((uint32_t)(510*16384) + addr); // ОЗУ CASH первые 16Kb
            else
            return read8psram((uint32_t)(511*16384) + addr ); // ОЗУ CASH  вторые 16Kb     
	    }
        
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
        {
    	//	 if (zx_RAM_bank_active > 7)// 0b 1111 1000
		if (zx_RAM_bank_active & 0xf8)  return read8psram((uint32_t)(zx_RAM_bank_active << 14) | masked_addr); 
        }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//###########################################################################################
// запись в память Пентагон 512 с кеш
inline static void fast(write_z80_cash)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
            if (cash_f == 0) return; // ПЗУ 
            if (zx_7ffd_lastOut & 0x10)
            {
               write8psram((510 << 14) | masked_addr, val); // ОЗУ CASH первые 16Kb
               return; // ОЗУ
            }
            else
            {
               write8psram((511 << 14) | masked_addr, val);// ОЗУ CASH  вторые 16Kb
               return; // ОЗУ
            }
        }
  
    if(x == 3) 
        {
		if (zx_RAM_bank_active & 0xf8) // >7
		    {
            write8psram((uint32_t)(zx_RAM_bank_active << 14) | (masked_addr), val);
		    return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
#endif
//#################################################################################

//---------------------------------------------------------------------------------
//PSRAM_BUTTER // для расширенной памяти на rp2350 с PSRAM бутерброд 
/*
Особенности работы:
Битовая магия:
Каждый block занимает биты 14-22 адреса
offset занимает биты 0-13 адреса
PSRAM_BASE добавляет старшие биты (0x11000000)
Автоматическое выравнивание: */
/**
 * @def Базовые константы для работы с PSRAM
 */
#define BLOCK_SHIFT  14          // Сдвиг для блоков (2^14 = 16KB размер блока)
#define BLOCK_MASK   0x1FF       // Маска для 512 блоков (9 бит)
#define OFFSET_MASK  0x3FFF      // Маска для смещения в блоке (14 бит = 16384 байт)
extern volatile uint8_t * const PSRAM_DATA ;

// чтение из памяти
inline static uint8_t fast(_read_z80_ext)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
         return (zx_1ffd_lastOut & 0x01)           // Быстрая проверка флага
                ? zx_ram_bank[0][masked_addr]      // RAM
                : zx_cpu_ram[0][masked_addr];      // ROM
	    }
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
        {
    	//	 if (zx_RAM_bank_active > 7)// 0b 1111 1000
		if (zx_RAM_bank_active & 0xf8)  return PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr];
        }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//---------------------------------------------------------------------------------------------------------------
// запись в память
inline static void fast(_write_z80_ext)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
             if(zx_1ffd_lastOut & 0x01) {zx_ram_bank[0][masked_addr] = val;}
            return;  // Выход для ROM/RAM банка 0
        }
  
    if(x == 3) 
        {
		if (zx_RAM_bank_active & 0xf8) // >7
		    {
            PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr]=val;
		    return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
//#############################################################################################
// управление 512 банками памяти 8 Мегабайт
// чтение из памяти 8Mb
inline static uint8_t fast(_read_z80_ext8)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) return zx_cpu_ram[0][addr & 0x3fff]; // ПЗУ
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
    if (zx_RAM_bank_active > 7)
	//	if (zx_RAM_bank_active & 0x01f8) // 0b 1111 1000
		{
            return PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr]; // 0xc000-0xffff память больше 128
		}
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//##############################################################################################
// запись в память 8Mb
inline static void fast(_write_z80_ext8)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0)  return;  // Выход для ROM/RAM банка 0
    if(x == 3) 
        {
            if (zx_RAM_bank_active > 7)
            //	if (zx_RAM_bank_active & 0x01f8) //
		    {
                PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr]=val; // 0xc000-0xffff память больше 128
		        return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
//################################################################################
// Пентагон 512 с КЕШ
//################################################################################
// чтение из памяти Пентагон 512 с кеш
inline static uint8_t fast(_read_z80_cash)(Machine *self, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
            if ((cash_f) == 0)
            {
              return zx_cpu_ram[0][addr]; // ПЗУ	
            }
            if (zx_7ffd_lastOut & 0x10)
            return PSRAM_DATA[(510 << 14) | masked_addr];// ОЗУ CASH первые 16Kb
            else
            return PSRAM_DATA[(511 << 14) | masked_addr];// ОЗУ CASH  вторые 16Kb     
	    }
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
        {
    	//	 if (zx_RAM_bank_active > 7)// 0b 1111 1000
		if (zx_RAM_bank_active & 0xf8)  return PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr];
        }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//---------------------------------------------------------------------------------------------------------------
// запись в память Пентагон 512 с кеш
inline static void fast(_write_z80_cash)(Machine *self, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
        {
            if (cash_f == 0) return; // ПЗУ 
            if (zx_7ffd_lastOut & 0x10)
            {
               PSRAM_DATA[(510 << 14) | masked_addr]=val;// ОЗУ CASH первые 16Kb
               return; // ОЗУ
            }
            else
            {
               PSRAM_DATA[(511 << 14) | masked_addr]=val;// ОЗУ CASH первые 16Kb
               return; // ОЗУ
            }
        }
  
    if(x == 3) 
        {
		if (zx_RAM_bank_active & 0xf8) // >7
		    {
                PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr]=val;// 0xc000-0xffff память больше 128
		    return;
		    }
		}  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
//---------------------------------------------------------------------------------
//#########################################################
//#define PIN_ZX_LOAD (22)
//функции ввода звука спектрума

bool fast (hw_zx_get_bit_LOAD)()
{
    bool x;
/*     if (tap_loader_active){ // Переменная определена в util_tap.h
        if (TapeStatus==TAPE_LOADING){
            out = TAP_Read(); // Переменная определена в util_tap.h
		}	
	} else { */
      x = gpio_get(PIN_ZX_LOAD);
//	};
    valLoad=conf.vol_load * x; 
    return x;
};
//###############################################
// IN
//###############################################

inline static uint8_t fast(in_z80)(Machine *self, uint16_t port16) {
	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16&0x00ff;

//return 0;

    #if defined(GENERAL_SOUND)
    if (portL == 0xB3) return in_GSP(GS_READ_IN_B3); 
    if (portL == 0xBB) return in_GSP(GS_STATUS_IN_BB); 
    #endif 
    #if defined(Z_CONTROLER)
    if (portL == 0x57) return in_GSP(ZC_READ_IN_57); 
    if (portL == 0x77) return in_GSP(ZC_READ_IN_77); 
    #endif
    #if defined(RTC_NOVA)
    if (portL == 0x89) return in_GSP(RTC_READ_IN_89); 
    #endif

    #ifdef MIDI    
    if (port16 == 0xa1cf ) 	return in_GSP(MIDI_IN); 
    #endif

	if (trdos) // если это tr-dos
	{
          	if (portL == 0xFF)       return Requests;
            //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
            if ((portL & 0x7F) == portL) return WD1793_Read((portL>>5) & 0b11); // Read from 0x7F to 0x1F port
             
            #if defined RTC_SMUC  // теневой порт
            if (port16  ==  0xDFBA) { return in_GSP(RTC_READ_IN_DFBA);}//чтение порта часов
            if (port16  == 0x5FBA) return 0b01101000;//SMUK_VER;
            #endif
           return 0xFF;  
	} // end tr-dos

	if (port16&1)
	{
		// МЫШЬ
			if (port16 == 0xfadf) return mouse[1]; //#FADF - поpт  кнопок
			if (port16 == 0xfbdf) return mouse[2]; //#FBDF - поpт X-кооpдинаты;
			if (port16 == 0xffdf) return mouse[3]; //#FFDF - поpт У-кооpдинаты.
        //Kempston джойстик    
		    if (portL==0x1f) return (zx_input.kempston | joy_k);
            
        #ifdef  TURBOSOUND         
        if ((port16 & 0xc002) == 0xc000) 	return in_GSP(TS_READ_IN_FFFD); 
        #else  
		if ((port16 & 0xc002) == 0xc000) 	return AY_in_FFFD(); 
        #endif    
        



	}
	else
    {
		//загрузка с магнитофона и опрос клавиатуры
		if (hw_zx_get_bit_LOAD())  return zx_keyboardDecode(portH);
		else return(zx_keyboardDecode(portH) & 0b10111111);	
    }

  if (portL== 0xFF) return port_atr();
	return 0xFF;
}

//###################################################################
// IN pentagom CASH
//###################################################################
inline static uint8_t fast(in_z80_cash)(Machine *self,uint16_t port16) {
	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
    
     /* Включение кеш*/
	 if (portL == 0xFB)
	    {	
            cash_f = 1;
			return 0xff;
        }
     /* Выключение кеш*/
	 if (portL == 0x7B)
	    {	
            cash_f = 0;
			return 0xff;
        }


    #if defined(GENERAL_SOUND)
    if (portL == 0xB3) return in_GSP(GS_READ_IN_B3); 
    if (portL == 0xBB) return in_GSP(GS_STATUS_IN_BB); 
    #endif 
    #if defined(Z_CONTROLER)
    if (portL == 0x57) return in_GSP(ZC_READ_IN_57); 
    if (portL == 0x77) return in_GSP(ZC_READ_IN_77); 
    #endif
    #if defined(RTC_NOVA)
    if (portL == 0x89) return in_GSP(RTC_READ_IN_89); 
    #endif



	if (trdos) // если это tr-dos
	{
          	if (portL == 0xFF)       return Requests;
            //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
            if ((portL & 0x7F) == portL) return WD1793_Read((portL>>5) & 0b11); // Read from 0x7F to 0x1F port
             
            #if defined RTC_SMUC  // теневой порт
            if (port16  ==  0xDFBA) { return in_GSP(RTC_READ_IN_DFBA);}//чтение порта часов
            if (port16  == 0x5FBA) return 0b01101000;//SMUK_VER;
            #endif
           return 0xFF;  
	} // end tr-dos

	if (port16&1)
	{
		// МЫШЬ
			if (port16 == 0xfadf) return mouse[1]; //#FADF - поpт  кнопок
			if (port16 == 0xfbdf) return mouse[2]; //#FBDF - поpт X-кооpдинаты;
			if (port16 == 0xffdf) return mouse[3]; //#FFDF - поpт У-кооpдинаты.
        //Kempston джойстик    
		    if (portL==0x1f) return (zx_input.kempston | joy_k);
            
        #ifdef  TURBOSOUND         
        if ((port16 & 0xc002) == 0xc000) 	return in_GSP(TS_READ_IN_FFFD); 
        #else  
		if ((port16 & 0xc002) == 0xc000) 	return AY_in_FFFD(); 
        #endif            

	}
	else
   {
		//загрузка с магнитофона и опрос клавиатуры

		if (hw_zx_get_bit_LOAD())
		
		{
			
			uint8_t out_data=zx_keyboardDecode(portH);
			
			return out_data;
		}
		else
		{
			uint8_t out_data=zx_keyboardDecode(portH);
			return(out_data&0b10111111);
		};
		
    }

  if (portL== 0xFF) return port_atr();
	return 0xFF;
}
//-------------------------------------
// IN MURMOZAVR 8Mb
inline static uint8_t fast(in_z80_p8)(Machine *self, uint16_t port16) {
	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;

    if (port16==0xaff7) return zx_aff7_lastOut ;
    if (port16==0x7ffd) return zx_7ffd_lastOut ;

    #if defined(GENERAL_SOUND)
    if (portL == 0xB3) return in_GSP(GS_READ_IN_B3); 
    if (portL == 0xBB) return in_GSP(GS_STATUS_IN_BB); 
    #endif 
    #if defined(Z_CONTROLER)
    if (portL == 0x57) return in_GSP(ZC_READ_IN_57); 
    if (portL == 0x77) return in_GSP(ZC_READ_IN_77); 
    #endif
    #if defined(RTC_NOVA)
    if (portL == 0x89) return in_GSP(RTC_READ_IN_89); 
    #endif


	if (trdos) // если это tr-dos
	{
          	if (portL == 0xFF)       return Requests;
            //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
            if ((portL & 0x7F) == portL) return WD1793_Read((portL>>5) & 0b11); // Read from 0x7F to 0x1F port
             
            #if defined RTC_SMUC  // теневой порт
            if (port16  ==  0xDFBA) { return in_GSP(RTC_READ_IN_DFBA);}//чтение порта часов
            if (port16  == 0x5FBA) return 0b01101000;//SMUK_VER;
            #endif
           return 0xFF;  
	} // end tr-dos

	if (port16&1)
	{
		// МЫШЬ
			if (port16 == 0xfadf) return mouse[1]; //#FADF - поpт  кнопок
			if (port16 == 0xfbdf) return mouse[2]; //#FBDF - поpт X-кооpдинаты;
			if (port16 == 0xffdf) return mouse[3]; //#FFDF - поpт У-кооpдинаты.
        //Kempston джойстик    
		    if (portL==0x1f) return (zx_input.kempston | joy_k);
            
        #ifdef  TURBOSOUND         
        if ((port16 & 0xc002) == 0xc000) 	return in_GSP(TS_READ_IN_FFFD); 
        #else  
		if ((port16 & 0xc002) == 0xc000) 	return AY_in_FFFD(); 
        #endif            

	}
	else
    {
		//загрузка с магнитофона и опрос клавиатуры

		if (hw_zx_get_bit_LOAD())
		
		{
			
			uint8_t out_data=zx_keyboardDecode(portH);
			
			return out_data;
		}
		else
		{
			uint8_t out_data=zx_keyboardDecode(portH);
			return(out_data&0b10111111);
		};
		
    }

  if (portL== 0xFF) return port_atr();
	return 0xFF;
}
//-------------------------------------------------
// расширенная память и так далее
inline void fast (zx_machine_set_7ffd_out)(uint8_t val)// переключение банков памяти по 7ffd
{
		// для pentagon512
        //  zx_RAM_bank_active=  (val&0b00000111);
        //	uint8_t x = (val&0b11000000);
        //	x = x>>3; // сместить x на 3 бит вправо 
         //   zx_RAM_bank_active = (zx_RAM_bank_active|x);
         //  zx_RAM_bank_active = (((val&0b11000000)>>3)|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
         //  if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //           76543210  5 bit
		//if (val&32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
        //------------------------------------------------------------------
		// для pentagon1024
        //   zx_RAM_bank_active = (((val&0b11100000)>>2)|(val&0b00000111)); // d0 d1 d2  и d5 d6 d7 7ffd
        //   // 5bit = 1 48k mode block отсутствует в pentagon1024
        //------------------------------------------------------------------
       if (zx_state_48k_MODE_BLOCK) return; //  48k mode 
	   
        zx_7ffd_lastOut=val;

    switch (pent_config)
	{
//-----------------------------------------------------------------------------------
	case PENT128 /* pentagon 128  и остальные кому нужно*/:
	  zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
	
	  zx_RAM_bank_active = zx_RAM_bank_7ffd |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	 	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
       rom_select(); // переключение ПЗУ по портам и по сигналу DOS

	return; // выход нафиг
//-----------------------------------------------------------------------------------
	case NOVA256/* nova 256 */:
	   zx_RAM_bank_active  = (((val&0b10000000))|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //        76543210  5 bit
        zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
       rom_select(); // переключение ПЗУ по портам и по сигналу DOS

	return; // выход нафиг	
//--------------------------------------------------------------------------------
     case SCORP256 /* Scorpion 256 */:
	  
          if (val & 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
          
          zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd


          zx_RAM_bank_active = zx_RAM_bank_7ffd |  zx_RAM_bank_1ffd ;


	      zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	
	      if (val & 0x08) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

           rom_select(); // переключение ПЗУ по портам и по сигналу DOS

     return; // выход нафиг
//------------------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

// разные пентагоны 512 и 1024

	case PENT512/* pentagon 512 */:
	   zx_RAM_bank_active  = (((val&0b11000000))|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
       zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
		break;	
	case PENT1024/* pentagon 1024 */:                  
        zx_RAM_bank_active  = (((val&0b11100000)>>2)|(val&0b00000111)); // d0 d1 d2  и d5 d6 d7 7ffd
        zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	    zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	  // zx_RAM_bank_7ffd  = (((val&0b11100000))|(val&0b00000111)); // d0 d1 d2  и d5 d6 d7 7ffd
        // 5bit = 1 48k mode block отсутствует в pentagon1024
		break;	

     case SPEC48/* Spectrum 48  */:
        zx_RAM_bank_active = 0x00; 
        zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode 
        zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
        zx_video_ram=zx_ram_bank[5];  
		rom=1;
        zx_cpu_ram[0]=zx_rom_bank[1]; 
		break;


	default:
			zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	     zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
		break;
	}

	if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

              rom_select(); // переключение ПЗУ по портам 

};
//------------------------------------------------------------------------------
uint8_t zx_machine_get_7ffd_lastOut(){return zx_7ffd_lastOut;}
uint8_t zx_machine_get_zx_RAM_bank_active(){return zx_RAM_bank_active;}
uint8_t zx_machine_get_1ffd_lastOut(){return zx_1ffd_lastOut;}
uint8_t zx_machine_get_rom(){return rom;}
//################################################################################
// OUT!
//################################################################################
// ZX Spectrum 48
inline static void fast(spec48)(Machine *self,uint16_t port16, uint8_t val)
{
	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;

	if (trdos) {trdos_out(portL,val); return;}// если это tr-dos

	if (port16 & 1) 
	{

	#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
    #endif
    #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
    #endif
    #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
    #endif

	}
	else
	{
		//hw_zx_set_snd_out(val  & 0b00010000);					// D4 beep
		hw_beep_out(val  & 0b00010000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end ZX Spectrum 48

//####################################################################################################################
// ZX Spectrum 128
inline static void fast(spec128)(Machine *self, uint16_t port16, uint8_t val)
{
   // return;
//	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;
 
    if (trdos) {  trdos_out(portL,val); return;}// если это tr-dos

	#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
    #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
    #endif
    #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
    #endif
    #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
    #else
       // if ((portL == 0x88 ) || (portL == 0x89 )) return;//для работы кое чего
        if ((portL & 0xFE) == 0x88) return;//для работы кое чего
    #endif

    #ifdef MIDI    
    if (port16 == 0xa0cf ) 	{out_GSP(MIDI_OUT,val);   return;};  // out 0xa0cf 
    #endif


////#######     нужно для демки которая переключает память нестандартно по порту #FC BIN 1111 1100 A0=0

 
	if (((not_port16 & 0x8002) == 0x8002))//7ffd
    //  if (port16  == 0x7ffd)//7ffd
		// 0111 1111  1101
		// A1=0   A15=0	
		{
			 if (zx_state_48k_MODE_BLOCK) return; //  48k mode 
			//zx_machine_set_7ffd_out(val);
             zx_7ffd_lastOut=val;     	//переключение банка памяти	

        zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
       // zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	    zx_RAM_bank_active = zx_RAM_bank_7ffd ;
	    zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	    if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];  
          
		  rom=(zx_7ffd_lastOut & 0x10)>>4;  // переключение ПЗУ
	     zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4];  // переключение ПЗУ
		//return; ////#######     нужно для демки которая переключает память нестандартно по порту #FC BIN 1111 1100 A0=0
        } 


////######
	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //psram_avaiable = false;
	{

    #ifdef  TURBOSOUND   
	// чип AY
    if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
    #else    
    if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
    #endif

	}
    
	else
	{
	//  hw_zx_set_snd_out(val  & 0b00010000);					// D4 beep
    	hw_beep_out(val & 0b00010000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end ZX Spectrum 128
//#########################################################################
// extram128
//#########################################################################
inline static void fast(extram128)(Machine *self,uint16_t port16, uint8_t val)
{
//	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;
    
    if (trdos) {  trdos_out(portL,val); return;}// если это tr-dos

	#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
    #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
    #endif
    #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
    #endif
    #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
    #endif

//------------------------------------------------------------------------------
//  OUT на разные нестандартные порты звука и так далее
/* if (port == 0x0F) {SoundLeft_A = val;}
if (port == 0x1F) {SoundLeft_B = val;}
if (port == 0x4F) {SoundRight_A = val;}  
if (port == 0x5F) {SoundRight_B = val;}
if (port == 0xFB) {SoundRight_B = val;SoundLeft_B = val;SoundRight_A = val;SoundLeft_A = val;} */

//  if (port == 0xFB) {Covox = val;} 

/* .  Попробуй так сделать (SoundLeft_A*SoundLeft_B ) и в ШИМ или I2C. Может уменьшить до 12 разрядов. */

// END  OUT на разные нестандартные порты звука и так далее
//-------------------------------------------------------------------------------

		if (((not_port16 & 0x8002) == 0x8002))//7ffd
		{
            zx_machine_set_7ffd_out(val); 
			//return;	 ////#######     нужно для демки которая переключает память нестандартно по порту #FC BIN 1111 1100 A0=0
		}; 



	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //psram_avaiable = false;
	{

    #ifdef  TURBOSOUND   
	// чип AY
    if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
    #else    
    if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
    #endif

	}
	else
	{
		//hw_zx_set_snd_out(val  & 0b00010000);					// D4 beep
		hw_beep_out(val  & 0b00010000);					// 01000

		
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end extram128
//###########################################################################
// SCORPION ZS256
//###########################################################################
inline static void fast(extram_1ffd)(Machine *self,uint16_t port16, uint8_t val)
{
	//uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;


        if (trdos) {  trdos_out(portL,val); return;}// если это tr-dos

		#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
        #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
        #endif
        #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
        #endif
        #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
        #endif


         if (port16 == 0x1ffd)
		// 0001 1111  1111 1101
		// A1=0  A15 = 0 A14 = 0 A13=0
		// if (((not_port16 & 0xe002) == 0xe002)&&((port16&0x1000)==0x1000))//   #1ffd
		//   if (((not_port16 & 0xe002) == 0xe002))//   #1ffd

		{
            zx_1ffd_lastOut=val;
			zx_RAM_bank_1ffd = ((val & 0x10) >> 1); // d4  1ffd scorpion 0000 x000
            zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_1ffd ;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_7ffd];

            rom_select(); // переключение ПЗУ по портам  
			return;
		}

		if (((not_port16 & 0x8002) == 0x8002))//7ffd
		// 0111 1111  1101
		// A1=0   A15=0	
		{
            zx_machine_set_7ffd_out(val); 
            // return;	//	
		};



	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //psram_avaiable = false;
	{

        #ifdef  TURBOSOUND   
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
    	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
        #else    
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	    if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
        #endif

	}
	else
	{
	//	hw_zx_set_snd_out(val & 0x10);					// D4 beep
		hw_beep_out(val  & 0b00010000);	
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
	
}
// end extram_1ffd   // SCORPION ZS256
//###############################################################################
//  GMX 2048
//###############################################################################
inline static void fast(extram_gmx)(Machine *self,uint16_t port16, uint8_t val)
{
	//uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;

	if (trdos) {trdos_out(portL,val); return;}// если это tr-dos

		#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
        #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
        #endif
        #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
        #endif
        #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
        #endif


		if (port16 == 0x1ffd)
		// 0001 1111  1111 1101
		// A1=0  A15 = 0 A14 = 0 A13=0
		// if (((not_port16 & 0xe002) == 0xe002)&&((port16&0x1000)==0x1000))//   #1ffd
		//   if (((not_port16 & 0xe002) == 0xe002))//   #1ffd

		{
			zx_RAM_bank_1ffd = ((val & 0b000010000) >> 1); // d4  1ffd scorpion 0000 1000
			zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_7ffd];
            rom_select(); // переключение ПЗУ по портам и по сигналу DOS
			return;
		}

		if (port16 == 0xdffd)
		// 1101 1111  xxxx 1101
		// A1=0  A15 = 1 A14 = 1 A13=0
		//  if (((not_port16 & 0x2002) == 0x2002)&&((port16&0xc000)==0xc000))//   #dffd
		// if (((not_port16 & 0x2002) == 0x2002))//   #dffd

		{
			zx_RAM_bank_dffd = ((val & 0b00000111) << 4); // d0 d1 d2   dffd proffi 0xxx 0000
			zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_7ffd]; // биты  0111 0000
			return;
		}  



	  	//if (port16==0x7FFD)
          if (((not_port16 & 0x8002) == 0x8002))//7ffd
          {
              zx_machine_set_7ffd_out(val);  
              //return;	
          };


	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //psram_avaiable = false;
	{


        #ifdef  TURBOSOUND   
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
    	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
        #else    
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	    if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
        #endif

	}

	else
	{
	//	hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_beep_out(val  & 0b00010000);	
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end extram_gmx

//###########################################################################
// MmurmoZavr 8Mb OUT#AFF7
//###########################################################################
inline static void fast(extram_p8)(Machine *self, uint16_t port16, uint8_t val)
{
	//uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;

	if (trdos) {trdos_out(portL,val); return;}// если это tr-dos

		#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
        #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
        #endif
        #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
        #endif
        #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
        #endif

            if (port16 == 0xaff7) // #AFF7       
		{
            zx_aff7_lastOut=val;
			zx_RAM_bank_ext8 = (val & 0x00ff)<<3;
			
			zx_RAM_bank_active = (uint32_t)zx_RAM_bank_ext8 | (uint32_t)zx_RAM_bank_7ffd;
			return;
		}  

			/*  для расширения до 4Мб по порту 7ffd и aff7
			uint16_t zx_RAM_bank_aff7 = ((val & 0b00011111)<<3); //  cдвиг влево на 3 бита
			uint16_t zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_aff7; // 7ffd 0000 0xxx 
																	           // aff7 yyyy y000
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active]; //zx_RAM_bank_active 0x00 ... 0xff (256*16kb)
			*/
            /*  для расширения до 8Мб по порту 7ffd и aff7
			uint16_t zx_RAM_bank_aff7 = ((val & 0b00111111)<<3); //  cдвиг влево на 3 бита
			uint16_t zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_aff7; // 7ffd 0000 0xxx 
																	           // aff7 yyyy y000
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active]; // zx_RAM_bank_active 0x000 ... 0x1ff (512*16kb)
			*/
		//if (port16 == 0x7FFD) // 0011 1000  dffd
		 if (((not_port16 & 0x8002) == 0x8002)) // 7ffd  1100 0111

		{
			//zx_machine_set_7ffd_out(val);

			 zx_7ffd_lastOut=val;
          //zx_RAM_bank_ext8 =0;
	       zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
           if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
	
	    zx_RAM_bank_active = (uint32_t)zx_RAM_bank_ext8 | (uint32_t)zx_RAM_bank_7ffd;
        //  if (zx_RAM_bank_ext8==0)
	            zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];

	      if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
 
         //  rom_select(); // переключение ПЗУ по портам и по сигналу DOS  
		   	rom=(zx_7ffd_lastOut & 0x10)>>4; 
		zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
			//return; // 
		};



	if (port16 & 1) // 
	{
        #ifdef  TURBOSOUND   
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
    	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
        #else    
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	    if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
        #endif
	}
	else
	{
		//hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_beep_out(val  & 0b00010000);	

		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}



//===========================================================================
// NOVA 256
//===========================================================================
inline static void fast(nova_256)(Machine *self, uint16_t port16, uint8_t val)
{
//	uint8_t portH = port16 >> 8;
	uint8_t portL = (uint8_t)port16;
	uint16_t not_port16 = ~port16;

// QUORUM
 if (portL == 0x00) {
//	if ((val&0b00100000) == 0) zx_cpu_ram[0] = zx_rom_bank[3]; 
//	else  
	zx_0000_lastOut = val;	// QUORUM
	rom_select(); // переключение ПЗУ по портам и по сигналу DOS
	return;
} 
// QUORUM

	if (trdos) {trdos_out(portL,val); return;}// если это tr-dos

		#ifdef GENERAL_SOUND   
        if (portL == 0xB3) {out_GSP(GS_WRITE_OUT_B3,  val);   return;}// передача данных в GS
        if (portL == 0xBB) {out_GSP(GS_COMMAND_OUT_BB,val);   return;}// передача команды в GS
        #else
		//SAA1099
		if(port16 == 0x01FF){saa1099_write(1,val);return;}					
		if(port16 == 0x00FF){saa1099_write(0,val);return;}
        #endif
        #ifdef Z_CONTROLER 
        if (portL == 0x57) {out_GSP(ZC_WRITE_OUT_57,  val); return;}// передача данных в SD карту
        if (portL == 0x77) {out_GSP(ZC_WRITE_OUT_77,val);z_controler_cs = val; return;}//управление SD   SD_SPI_CS0_PIN val&0x02
        #endif
        #ifdef  RTC_NOVA
        if (portL  ==  0x88 ) {out_GSP(RTC_WRITE_OUT_88,  val); return;}//номер регистра часов
        if (portL  ==  0x89 ) {out_GSP(RTC_WRITE_OUT_89,  val); return;}//данные регистра часов
        #endif




	if (port16 & 1) // 
	{

		 if (((not_port16 & 0x8002) == 0x8002)) // 7ffd  1100 0111
		{
			zx_machine_set_7ffd_out(val);
			//return; // 
		};


        #ifdef  TURBOSOUND   
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{out_GSP(TS_WRITE_OUT_FFFD, val);   return;}   // OUT(#FFFD),val
    	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{out_GSP(TS_WRITE_OUT_BFFD, val);   return;}    
        #else    
        if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xc000) == 0xc000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	    if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
        #endif

	}
	else
	{
		//hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_beep_out(val  & 0b00010000);	
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
	
}
// end nova_256
//##############################################################################
//### Настройки и функции для эмулятора Z80 REDCODE Manuel Sainz 

Machine machine;  // Создаем экземпляр машины
Machine* z1 = &machine;  // Создаем указатель на машину


Device *machine_find_device(Machine *self, zuint16 port)
{
    zusize index = 0;

    for (; index < self->device_count; index++)
        if (self->devices[index].assigned_port == port)  // Исправлено: . вместо ->
            return &self->devices[index];                // Исправлено: возвращаем адрес элемента массива

    return Z_NULL;
}

inline static void fast(halt_z80)(Machine *self, zuint8 signal)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();


}
inline static zuint8 fast(inta_callback)(Machine *self, zuint16 address)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();
 //int_enable = false;
 //z80_int(&z1->cpu, false);// INT OFF

        // Сброс линии INT после обработки
   //     if (int_enable && !(z1->cpu.request & Z80_REQUEST_INT)) {
    //        z80_int(&z1->cpu, Z_FALSE);
           // z1->int_pending = 0;
     //      int_enable = false;
    //    }




return 0xff;
}

inline static zuint8 fast(nop_callback)(Machine *self, zuint16 address)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();
 //z80_int(&z1->cpu, false);// INT OFF
return 0x00;
}
//#define Z80_CHIP Z80_MODEL_ZILOG_NMOS
//#define Z80_CHIP Z80_MODEL_ZILOG_CMOS
//#define Z80_CHIP Z80_MODEL_NEC_NMOS
//#define Z80_CHIP Z80_MODEL_ST_CMOS
//########################

//
void select_cpu_z80(Machine *self) {
    switch (conf.cpu_version)
    {
    case 0: self->cpu.options = Z80_MODEL_ZILOG_NMOS; return;
    case 1: self->cpu.options = Z80_MODEL_ZILOG_CMOS; return;   
    case 2: self->cpu.options = Z80_MODEL_NEC_NMOS; return;
    case 3: self->cpu.options = Z80_MODEL_ST_CMOS; return; 
    case 4: self->cpu.options = 0; return; 
    self->cpu.options = Z80_MODEL_ZILOG_NMOS; return;
    }
    }
//###########################################
// инициализация Z80 
// Pentagon 48
void machine_Spectrum_48(Machine *self)
        {
        self->cpu.context      = self;
        self->cpu.fetch_opcode = (Z80Read )read_z80;
        self->cpu.fetch        = (Z80Read )read_z80;
        self->cpu.nop          = (Z80Read )read_z80;
        self->cpu.read         = (Z80Read )read_z80;
        self->cpu.write        = (Z80Write)write_z80;
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)spec48;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;

		pent_config = SPEC48;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// инициализация Z80 defain Pentagon 128
void machine_Pentagon_128(Machine *self)
        {
        self->cpu.context      = self;
        self->cpu.fetch_opcode = (Z80Read )read_z80;
        self->cpu.fetch        = (Z80Read )read_z80;
        self->cpu.nop          = (Z80Read )read_z80;
        self->cpu.read         = (Z80Read )read_z80;
        self->cpu.write        = (Z80Write)write_z80;
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)spec128;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;//= (Z80Halt)halt_z80;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;

		pent_config = PENT128;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        conf.shift_img=12585;
        }
// инициализация  Pentagon 512
void machine_Pentagon_512(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext;
        self->cpu.fetch        = (Z80Read )read_z80_ext;
        self->cpu.nop          = (Z80Read )read_z80_ext;
        self->cpu.read         = (Z80Read )read_z80_ext;
        self->cpu.write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext;
        self->cpu.fetch        = (Z80Read )_read_z80_ext;
        self->cpu.nop          = (Z80Read )_read_z80_ext;
        self->cpu.read         = (Z80Read )_read_z80_ext;
        self->cpu.write        = (Z80Write)_write_z80_ext;
        }
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)extram128;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;

		pent_config = PENT512;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// инициализация  Pentagon 512 CASH     
void nmi_Pentagon_512_cash(Machine *self)
{
    zx_7ffd_lastOut = zx_7ffd_lastOut | 0x10;
	// zx_7ffd_lastOut = zx_7ffd_lastOut & 0xef;
    cash_f = 1;
}
//
void machine_Pentagon_512_cash(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_cash;
        self->cpu.fetch        = (Z80Read )read_z80_cash;
        self->cpu.nop          = (Z80Read )read_z80_cash;
        self->cpu.read         = (Z80Read )read_z80_cash;
        self->cpu.write        = (Z80Write)write_z80_cash;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_cash;
        self->cpu.fetch        = (Z80Read )_read_z80_cash;
        self->cpu.nop          = (Z80Read )_read_z80_cash;
        self->cpu.read         = (Z80Read )_read_z80_cash;
        self->cpu.write        = (Z80Write)_write_z80_cash;
        }
        self->cpu.in           = (Z80Read )in_z80_cash;//machine_cpu_in;
        self->cpu.out          = (Z80Write)extram128;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = (Z80Read )nmi_Pentagon_512_cash;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
        
		pent_config = PENT512;
		ticks_per_frame=71680 ;// 71680- Пентагон 
        }
// инициализация  Pentagon 1024
void machine_Pentagon_1024(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext;
        self->cpu.fetch        = (Z80Read )read_z80_ext;
        self->cpu.nop          = (Z80Read )read_z80_ext;
        self->cpu.read         = (Z80Read )read_z80_ext;
        self->cpu.write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext;
        self->cpu.fetch        = (Z80Read )_read_z80_ext;
        self->cpu.nop          = (Z80Read )_read_z80_ext;
        self->cpu.read         = (Z80Read )_read_z80_ext;
        self->cpu.write        = (Z80Write)_write_z80_ext;
        }
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)extram128;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
        
		pent_config = PENT1024;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// Scorpion ZS256    
void nmi_Scorpion_256(Machine *self)
{
    rom=3;
    zx_cpu_ram[0] = zx_rom_bank[3];


}     
void machine_Scorpion_256(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext;
        self->cpu.fetch        = (Z80Read )read_z80_ext;
        self->cpu.nop          = (Z80Read )read_z80_ext;
        self->cpu.read         = (Z80Read )read_z80_ext;
        self->cpu.write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext;
        self->cpu.fetch        = (Z80Read )_read_z80_ext;
        self->cpu.nop          = (Z80Read )_read_z80_ext;
        self->cpu.read         = (Z80Read )_read_z80_ext;
        self->cpu.write        = (Z80Write)_write_z80_ext;
        }
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)extram_1ffd;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = (Z80Read )nmi_Scorpion_256;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
       
        pent_config = SCORP256;
		ticks_per_frame=71680  ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        conf.shift_img=8939;
        }
// Scorpion GMX 2048        
void machine_Scorpion_GMX(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext;
        self->cpu.fetch        = (Z80Read )read_z80_ext;
        self->cpu.nop          = (Z80Read )read_z80_ext;
        self->cpu.read         = (Z80Read )read_z80_ext;
        self->cpu.write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext;
        self->cpu.fetch        = (Z80Read )_read_z80_ext;
        self->cpu.nop          = (Z80Read )_read_z80_ext;
        self->cpu.read         = (Z80Read )_read_z80_ext;
        self->cpu.write        = (Z80Write)_write_z80_ext;
        }
        self->cpu.in           = (Z80Read )in_z80;//machine_cpu_in;
        self->cpu.out          = (Z80Write)extram_gmx;//machine_cpu_out;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
        
		pent_config = PENT128;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// NOVA 256 Кворум 256       
void nmi_NOVA_256(Machine *self)
{
    rom=3;  
    zx_cpu_ram[0] = zx_rom_bank[3];
    zx_0000_lastOut = 0; 
}
//
void machine_NOVA_256(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext;
        self->cpu.fetch        = (Z80Read )read_z80_ext;
        self->cpu.nop          = (Z80Read )read_z80_ext;
        self->cpu.read         = (Z80Read )read_z80_ext;
        self->cpu.write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext;
        self->cpu.fetch        = (Z80Read )_read_z80_ext;
        self->cpu.nop          = (Z80Read )_read_z80_ext;
        self->cpu.read         = (Z80Read )_read_z80_ext;
        self->cpu.write        = (Z80Write)_write_z80_ext;
        }
        self->cpu.in           = (Z80Read )in_z80;
        self->cpu.out          = (Z80Write)nova_256;
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = (Z80Read )nmi_NOVA_256;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
       
		pent_config = NOVA256;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// MurmoZavr 8Mb      
void machine_MurmoZavr(Machine *self)
        {
        self->cpu.context      = self;
        #ifdef MURM1
        if (psram_type)
        {
        self->cpu.fetch_opcode = (Z80Read )read_z80_ext8;
        self->cpu.fetch        = (Z80Read )read_z80_ext8;
        self->cpu.nop          = (Z80Read )nop_callback;
        self->cpu.read         = (Z80Read )read_z80_ext8;
        self->cpu.write        = (Z80Write)write_z80_ext8;
        }
        else
        #endif
        {
        self->cpu.fetch_opcode = (Z80Read )_read_z80_ext8;
        self->cpu.fetch        = (Z80Read )_read_z80_ext8;
        self->cpu.nop          = (Z80Read )nop_callback;
        self->cpu.read         = (Z80Read )_read_z80_ext8;
        self->cpu.write        = (Z80Write)_write_z80_ext8;
        }
        self->cpu.in           = (Z80Read )in_z80_p8;
        self->cpu.out          = (Z80Write)extram_p8;// aff7 + Pentagon 128
        self->cpu.halt         = Z_NULL;
        self->cpu.nmia         = Z_NULL;
        self->cpu.inta         = Z_NULL;//= (Z80Read )inta_callback;
        self->cpu.int_fetch    = Z_NULL;
        self->cpu.ld_i_a       = Z_NULL;
        self->cpu.ld_r_a       = Z_NULL;
        self->cpu.reti         = Z_NULL;
        self->cpu.retn         = Z_NULL;
        self->cpu.hook         = Z_NULL;
        self->cpu.illegal      = Z_NULL;
        
		pent_config =  PENT8M;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion

        }        
//##########################################################
void machine_power(Machine *self, zbool state)
        {
        if (state)
                {
                self->cycles = 0;
                }

        z80_power(&self->cpu, state);
        }
//###########################################################
void machine_reset(Machine *self)
        {
        z80_instant_reset(&self->cpu);
        }        
//##########################################################
#include "image.h"
#include "util_z80.h"
//==============================================================================
void init_rom_ram(uint8_t rom_x)
{
		// настройка ОЗУ 
        zx_cpu_ram[1] = zx_ram_bank[5]; // 0x4000 - 0x7FFF
	    zx_cpu_ram[2] = zx_ram_bank[2]; // 0x8000 - 0xBFFF
	    zx_cpu_ram[3] = zx_ram_bank[0]; // 0xC000 - 0x7FFF
	    zx_video_ram = zx_ram_bank[5];

/*         Фон (биты 5–3): 111 (белый).

        Текст (биты 2–0): 000 (чёрный).
        
        Яркость (бит 6): 0 (обычная яркость).
        
        Мигание (бит 7): 0 (выключено).

 */

     memcpy(zx_ram_bank[5], cat_img, 6144+768);

    // Копируем пиксельные данные (первые 6144 байта)
   // memcpy(zx_ram_bank[5], cat_img, 6144);

    // Копируем атрибуты (следующие 768 байт)
  //  memcpy(zx_ram_bank[5] + 0x1800, cat_img+ 6144, 768);

 switch (conf.mashine)
 {
 case NOVA256:
	    zx_rom_bank[0]=&ROM_128Q[0];//128k 
	    zx_rom_bank[1]=&ROM_48Q[0*16384];//48k 
		zx_rom_bank[2]=&ROM_Qtr[0*16384];//TRDOS 6.04
	    zx_rom_bank[3]=&ROM_Qsm[0*16384];//NAVIGATOR
		rom=3;
	    zx_cpu_ram[0]=zx_rom_bank[3]; // 0x0000 - 0x3FFF с какой банки стартовать

	zx_RAM_bank_active =0x00;
	zx_RAM_bank_7ffd =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;
    zx_RAM_bank_ext8 =0x00;


	zx_state_48k_MODE_BLOCK = false;

	zx_vbuf[0].is_displayed = true;
	zx_vbuf[0].data = g_gbuf;
	zx_vbuf_active = &zx_vbuf[0];

 return; // выход нафиг больше тут делать нечего

break;
#ifndef NO_GMX
    case GMX2048 :
    zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48K[0*16384];//48k 
        if (conf.trdos_version==0) zx_rom_bank[2]=&ROM_TRDOS_504T[0*16384];//TRDOS 5.04T
        else zx_rom_bank[2]=&ROM_TRDOS_505D[0*16384];//TRDOS 5.05D
		zx_rom_bank[3]=&ROM_Qsm[0*16384];//SERVICE PENTAGON
        rom=0;
	    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF с какой банки стартовать
break;
#endif
 case SCORP256:
        zx_rom_bank[0]=&ROM_SCORPION[0x0000];//128k 
	    zx_rom_bank[1]=&ROM_SCORPION[0x4000];//48k 
		zx_rom_bank[2]=&ROM_SCORPION[0xc000];//TRDOS 5.03
	    zx_rom_bank[3]=&ROM_SCORPION[0x8000];//SM
		rom=0;
	    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF с какой банки стартовать
      //  psram_cleanup(); очистка PSRAM SPI
	break;
//--------------------------
 case SPEC48:
        zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48K_ORIGINAL[0*16384];//48k 
        if (conf.trdos_version==0) zx_rom_bank[2]=&ROM_TRDOS_504T[0*16384];//TRDOS 5.04T
        else zx_rom_bank[2]=&ROM_TRDOS_505D[0*16384];//TRDOS 5.05D
        zx_rom_bank[3]=&ROM_Qsm[0*16384];//SERVICE PENTAGON
		rom=1;
	
		if (rom_x ==0) // первый запуск при включении или hard reset
        {
        switch (conf.autorun)
		{
		case 0     /* OFF */:
		    rom=1;
			zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF 48 BASIC 0  с какой банки стартовать
		break;

		case 1     /*TR-DOS */:
			if (conf.Disks[0][0] ==0 )
			  zx_cpu_ram[0]=zx_rom_bank[1]; // диска нет 48 BASIC 
			else 
			{
			rom=2;
			zx_cpu_ram[0]=zx_rom_bank[2]; // диск есть TR-DOS  
			}
		break;

		case 2     /* QS SLOT 0*/:
		    rom=1;
			zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF 48 BASIC 0  с какой банки стартовать
		break;

		default:
		    rom=0;
		    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 48 BASIC 0  с какой банки стартовать	
			break;
		}
		}

        if (rom_x ==1) // загрузка с вставленной дискетой по SPACE
		{
		 rom=2;
		 zx_cpu_ram[0]=zx_rom_bank[2]; // 0x0000 - 0x3FFF TR-DOS    запуск trd по   SPACE
		}

		if (rom_x ==3) // просто reset в  48 BASIC
		{
		 rom=0;
		 zx_cpu_ram[0]=zx_rom_bank[0]; //  48 BASIC
		}


     	zx_7ffd_lastOut=0x10;
        zx_RAM_bank_1ffd =0x00;
        zx_RAM_bank_dffd =0x00;
        zx_RAM_bank_active =0x00; 
        zx_RAM_bank_ext8 =0x00;

    	zx_vbuf[0].is_displayed = true;
	    zx_vbuf[0].data = g_gbuf;
    	zx_vbuf_active = &zx_vbuf[0];
    
	   zx_state_48k_MODE_BLOCK=true;
     return; // выход нафиг больше тут делать нечего 48 режим
//--------------------------	
     default:
       zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	//	zx_rom_bank[0]=&ROM_TEST[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48K[0*16384];//48k 
        if (conf.trdos_version==0) zx_rom_bank[2]=&ROM_TRDOS_504T[0*16384];//TRDOS 5.04T
        else zx_rom_bank[2]=&ROM_TRDOS_505D[0*16384];//TRDOS 5.05D

       // zx_rom_bank[3]=&ROM_SV[0*16384];//SERVICE PENTAGON
		zx_rom_bank[3]=&ROM_Qsm[0*16384];//SERVICE PENTAGON
        rom=0;
	    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF с какой банки стартовать
	  break;
 }
//-------------------------------------------
		if (rom_x ==0) // первый запуск при включении или hard reset
        {
        switch (conf.autorun)
		{
		case 0     /* OFF */:
		     rom=0;
			zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
			break;    


		case 1     /*TR-DOS */:
		
			if (conf.Disks[0][0] ==0 ) 
			{
				rom=0;
				zx_cpu_ram[0]=zx_rom_bank[0]; // диска нет 128 BASIC 
				zx_7ffd_lastOut=0x00;
			}
			else 
			{
				rom=2;
			zx_cpu_ram[0]=zx_rom_bank[2]; // диск есть TR-DOS  
			zx_7ffd_lastOut=0x10;//0x10
			//trdos=true;
			}
           if (conf.mashine== SCORP256) // загрузка только через меню  TODO!
			{
				rom=0;
				zx_cpu_ram[0]=zx_rom_bank[0]; // диска нет 128 BASIC 
				zx_7ffd_lastOut=0x10;
			}

		break;
		case 2     /* QS SLOT 0*/:
		    rom=0;
			zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
		break;

		default:
		    rom=0;
		    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
			break;
		}
		}
        if (rom_x ==1) // загрузка с вставленной дискетой по SPACE
		{
		 rom=2;
		 zx_cpu_ram[0]=zx_rom_bank[2]; // 0x0000 - 0x3FFF TR-DOS    запуск trd по   SPACE
		 zx_7ffd_lastOut=0x10;//0x10
		// trdos=true;
           if (conf.mashine== SCORP256) rom_x=3;// загрузка только через меню TODO!
		}

		if (rom_x ==3) // просто reset в  128 BASIC
		{
		 rom=0;
		 zx_cpu_ram[0]=zx_rom_bank[0]; //  128 BASIC
		 zx_7ffd_lastOut=0x00;
		}

	
        zx_1ffd_lastOut=0; 

		zx_RAM_bank_active =0x00;
		zx_RAM_bank_7ffd =0x00;
		zx_RAM_bank_1ffd =0x00;
		zx_RAM_bank_dffd =0x00;
		zx_RAM_bank_ext8 =0x00;
		zx_state_48k_MODE_BLOCK = false;

    	zx_vbuf[0].is_displayed = true;
	    zx_vbuf[0].data = g_gbuf;
	    zx_vbuf_active = &zx_vbuf[0];
}
//==============================================================================
void zx_machine_init()
{    
	//привязка реальной RAM памяти к банкам
  	for(int i=0;i<8;i++)
	{
		zx_ram_bank[i]=&RAM[i*0x4000];
	} 
       memset(&RAM,0x00, 131072);	// стирание памяти 128kB
	

	//z80_init(&cpu);

     init_mashine_and_extram(conf.mashine);// <= это уже тут   machine_Pentagon_128(z1);  // инициализация процессора
   
     machine_power(z1, Z_TRUE);  // Включаем питание машины
     zx_machine_reset(0);// 0-первый запуск  1- запуск trd по SPACE  3-просто reset в BASIC128

   //  machine_reset(z1);// Используем  для сброса регистров

};


void fast(zx_machine_input_set)(ZX_Input_t* input_data){memcpy(&zx_input,input_data,sizeof(ZX_Input_t));};

void zx_machine_reset(uint8_t rom_x)
{
	AY_reset();

    init_rom_ram(rom_x);

   //  machine_reset(z1);// Используем  для сброса регистров
     

	zx_RAM_bank_active =0x00;
	zx_RAM_bank_7ffd =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;
    zx_RAM_bank_ext8 =0x00;
    zx_aff7_lastOut=0;

    zx_1ffd_lastOut=0x00;
    zx_0000_lastOut = 0x00;// QUORUM

    WD1793_Init();

   //memset(&RAM,0x00, 131072);	// стирание памяти 128kB
    cash_f = 0;// отключение кеш для Пентагон 512 CASH

    seekbuf =0;// обнуление счетчика tape при сбросе
    init_vol_ay(); 

     machine_reset(z1);// Используем  для сброса регистров


};
//-------------------------------------------------------------------------
uint8_t* fast(zx_machine_screen_get)(uint8_t* current_screen)
{
		return zx_vbuf[0].data; //если буфер 1, то вариантов нет
};
//##########################################################################
void fast(zx_machine_flashATTR)(void)
{
	static bool stateFlash=true;
	stateFlash^=1;
	#if ZX_BPP==4
		if (stateFlash) memcpy(zx_colors_2_pix+512,zx_colors_2_pix,512); else memcpy(zx_colors_2_pix+512,zx_colors_2_pix+1024,512);
		#else
		if (stateFlash) memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix,512*2); else memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix+1024*2,512*2);
	#endif
}
//##########################################################################
//инициализация массива предпосчитанных цветов
void init_zx_2_pix_buffer()
{
	for(uint16_t i=0;i<384;i++)
	{
		uint8_t color=(uint8_t)i&0x7f;
		uint8_t color0=(color>>3)&0xf;
		uint8_t color1=(color&7)|(color0&0x08);
		
		if (i>128)
		{
			//инверсные цвета для мигания
			uint8_t color_tmp=color0;
			color0=color1;
			color1=color_tmp;			
		}
		
		for(uint8_t k=0;k<4;k++)
		{
			switch (k)
			{
				case 0:
				
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color0:(zx_color[color0]<<8)|zx_color[color0];
				
				break;

				case 2:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color0<<4)|color1:(zx_color[color0]<<8)|zx_color[color1];
				
				break;

				case 1:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color0:(zx_color[color1]<<8)|zx_color[color0];
				
				break;
				case 3:
				zx_colors_2_pix[i*4+k]=(ZX_BPP==4)?(color1<<4)|color1:(zx_color[color1]<<8)|zx_color[color1];
				
				
				break;
			}
		}
		
	}
	
}


//------------------------------------------
extern uint16_t beepPWM;
//------------------------------------------
// реальный INT 50 HZ через таймер пико
/* void zx_generator_int(void)
{
  // beepPWM = 0 ;// обнуление звука бипера на всякий случай
	if (int_enable) return;
    if (conf.turbo == 1) 
    {
   int_enable=true;// Генерация прерывания INT Z80  50Гц при TURBO
   z80_int(&z1->cpu, Z_TRUE);
} 
   #ifdef LEDBLINK
 //   led_blink();
    #endif
} */
//----------------------------------------
uint8_t* active_screen_buf=NULL;

// главный цикл выполнения команд Z80
void fast(zx_machine_main_loop_start)()
{
	uint64_t z80_cycles;
	//переменные для отрисовки экрана
	const int sh_y=56;
	const int sh_x=104;
	uint64_t inx_tick_screen=0;// счетчик тактов экрана
	//uint64_t tick_cpu=0; // Количество тактов до выполнения команды Z80
	uint32_t x=0;
	uint32_t y=0;
	
	init_zx_2_pix_buffer();
	uint8_t* p_scr_str_buf=NULL;
    uint8_t dt_cpu;
	uint64_t d_dst_time_ticks=0; // Количесто тактов реального процессора на текущую выполненную команду Z80
	uint64_t t0_time_ticks=0;    // Количество реальных тактов процессора после запуска машины Z80
    ticks_per_cycle=Z80_3500;//
	//ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A

	systick_hw->csr = 0x05;
	systick_hw->rvr = 0xFFFFFF;

	active_screen_buf=g_gbuf;
	p_scr_str_buf=active_screen_buf; 

	//вспомогательный индекс такта внутри картинки
	int draw_img_inx=0;

	//работа с аттрибутами цвета
	register uint8_t old_c_attr=0;
	register uint8_t old_zx_pix8=0;
	register uint32_t colorBuf;
	
	while(1){
		
	 	while (im_z80_stop){
			sleep_ms(1);
			if (!im_ready_loading) im_ready_loading = true;
	
		//	cpu.Int_pending = false; НОВЫЙ ЭМУЛЯТОР 
		}
 

///////////////////////////////////////////////////////////////////////////////////////////////////////
// tape load
if (enable_tape)
{
if (Z80_PC(z1->cpu) == 0x0556)  tape_load(); // вход в меню tape // CALL 1366 (0x0556)
////if (Z80_PC(z1->cpu) == 0x0562)  tape_load_0562(); // вход в меню tape // CALL 0x0562
if (Z80_PC(z1->cpu) == 0x056a)  tape_load_056a(); // вход в меню tape // CALL 0x056a

}
///////////////////////////////////////////////////////////////////////////////////////////////////////
		// tr-dos

		if ((zx_1ffd_lastOut & 0x02)== 0x00) // 0000 00x0  0x02 если не теневик 
      {
	
		if (!trdos) // если еще не в trdos то вход
		{
			if (((Z80_PC(z1->cpu) & 0x3D00) == 0x3D00) && ((zx_7ffd_lastOut & 0x10) == 0x10) )// trdos работает с BASIC48 D4 = 1     
			                                                                     
			{
			trdos = true;

           rom=2;
			zx_cpu_ram[0]=zx_rom_bank[2];// tr-dos
			
            }
			
		}
	  
      }
	  
		if ((Z80_PC(z1->cpu) > 0x3FFF) && (trdos))// выход из trdos если в RAM
		{
		 trdos = false;
         rom_select(); // переключение ПЗУ по портам  
		}
///////////////////////////////////////////////////////////////////////////////

		//=======================================================================================
			// Если нажали клавишу NMI // QUORUM // SCORPION
		//	if (main_nmi_key)
		//	{
                
         //   	 main_nmi_key = false;

//z80_run(&z1->cpu, 1);
         //   }
		//    if (conf.mashine == NOVA256)
	    	//	{
			//		rom=3;
			// НОВЫЙ ЭМУЛЯТОР	
           // 	zx_cpu_ram[0] = zx_rom_bank[3]; zx_0000_lastOut = 0; z80_nmi(&z1->cpu);
        //     } // QUORUM

        //   if (conf.mashine == SCORP256)
	    //		{
		//			rom=3;
				// НОВЫЙ ЭМУЛЯТОР	
         //       	 zx_cpu_ram[0] = zx_rom_bank[3];  z80_nmi(&z1->cpu);
           //      } // 


         //   if (conf.mashine == PENT_512CASH) // Пентагон 512 с кеш
	    //		{
					// zx_7ffd_lastOut = zx_7ffd_lastOut | 0x10;
					// zx_7ffd_lastOut = zx_7ffd_lastOut & 0xef;
				//	  cash_f = 1;
				// НОВЫЙ ЭМУЛЯТОР		
               //   z80_nmi(&z1->cpu);
          //       } // 
			

/*             if (conf.mashine == PENT1024) // Пентагон 1024
	    		{
				 //	 zx_7ffd_lastOut = zx_7ffd_lastOut | 0x10;
					// zx_7ffd_lastOut = zx_7ffd_lastOut & 0xef;
										rom=3;
					 zx_cpu_ram[0] = zx_rom_bank[3]; 
					  z80_gen_nmi(&cpu); } //  
			}

*/
		//=======================================================================================	

if (trdos) WD1793_Execute();

///////////////////////////////////////////////////////////////////////////////////////////////////////
		

		// Цикл ождания пока количество потраченных тактов реального процессора
		// меньше количества расчетных тактов реального процессора на команду Z80
 
	//if (conf.turbo==0)
    { 
    
   while (((get_ticks()-t0_time_ticks)&0xffffff)<d_dst_time_ticks);
           }

   /*         
if (   (inx_tick_screen<32) &&  (int_enable)) 
    {// вызов INT в обычном режиме
   //  z80_int(&z1->cpu, true);// Генерация прерывания Z80
     //if (conf.turbo != 1) 
    // z80_int(&z1->cpu, true);// Генерация прерывания INT Z80; 
    #ifdef LEDBLINK
    led_blink();
    #endif
    } // вызов INT в обычном режиме

/*     if (conf.turbo == 1) 
    {
    if (int_enable) z80_int(&z1->cpu, true);// Генерация прерывания INT Z80 в TURBO режиме;
    } 
  //  

*/

      t0_time_ticks=(t0_time_ticks+d_dst_time_ticks)&0xffffff;  


  //   gpio_put(LED_BOARD, 1);
   //    if (int_enable) z80_int(&z1->cpu, true);// Генерация прерывания INT Z80
     if (/* (inx_tick_screen<32)&& */(int_enable))
    {
        z80_int(&z1->cpu, Z_TRUE);
    //    int_enable = false;
     //  dt_cpu = z80_run(&z1->cpu, 1);
      // z80_int(&z1->cpu, false);// INT OFF
      }
     //  else
     
        dt_cpu = z80_run(&z1->cpu, 1);
//gpio_put(LED_BOARD, 0);

        // Сброс линии INT после обработки
 /*        if (int_enable ) {
            static uint8_t x = 0;
            if (x>5) 
            {
            z80_int(&z1->cpu, Z_FALSE);
            x=0;
            int_enable = false;
            }
            x++;
        } */


        // Сброс линии INT после обработки
        if (int_enable && !(z1->cpu.request & Z80_REQUEST_INT)) {
            z80_int(&z1->cpu, Z_FALSE);
            int_enable = false;
        }
 


    	d_dst_time_ticks=dt_cpu* ticks_per_cycle   ;// Расчетное количесто тактов реального процессора на выполненную команду Z80
	
		inx_tick_screen+=dt_cpu;//Увеличиваем на количество тактов Z80 на текущую выполненную команду.

   //   if (trdos) WD1793_Execute();


	 	// начало 
		 inx_tick_screen_ff=inx_tick_screen;



		 if (inx_tick_screen>=  ticks_per_frame)      // Если прошла 1/50 сек, 71680 тактов процессора Z80
			{
               
	        	//if (conf.turbo != 1) 
               // {
                int_enable=true; // включение INT NORMAL 50 Гц или FAST 100 Гц
           // z80_int(&z1->cpu, Z_TRUE);
              //  }
             
                //  z80_int(&z1->cpu, true);// Генерация прерывания INT Z80; 
              //  z80_run(&z1->cpu, 1);//28
              //  z80_int(&z1->cpu, false);// INT OFF

                 
		 	inx_tick_screen-=ticks_per_frame; //Такты Z80 1/50 секунды если здесь поставить =0 то в BREAKSPACE DEMO НЕ БУДЕТ КРЫЛЬЕВ!
		 	x=0;y=0;
			draw_img_inx=0; //??????????
			p_scr_str_buf=active_screen_buf; 
			
			if (inx_tick_screen==0)  //  0 ?????
			    continue;

		};

		if (!vbuf_en) continue;

		//новая прорисовка
		register int img_inx=(inx_tick_screen-conf.shift_img);

  	//	if (img_inx<0 || (img_inx>=(T_per_line*240))){ //область изображения, если вне, то не рисуем
	 if  (img_inx>=(53760))  continue; //область изображения, если вне, то не рисуем
			
	  
		

		//смещения бордера
		const int dy=24;
		const int dx=32;
		
		for(;draw_img_inx<img_inx;){

		//	if (x==T_per_line*2) {
		 	if (x==448) {
				x=0;
				y++;
				int ys=y-dy;//номер строки изображения
				uint32_t  img_ptr=(((ys&0b11000000)|((ys>>3)&7)|((ys&7)<<3))<<5);
				uint32_t  img_ptr_attr=(6144+((ys<<2)&0xFFE0));
				p_zx_video_ram5=zx_ram_bank[5]+img_ptr;
				//указатель на начало строки байтов цветовых аттрибутов screen 5
				p_zx_video_ramATTR5=zx_ram_bank[5]+img_ptr_attr;
		
				p_zx_video_ram7=zx_ram_bank[7]+img_ptr;
				//указатель на начало строки байтов цветовых аттрибутов screen 7
				p_zx_video_ramATTR7=zx_ram_bank[7]+img_ptr_attr;

			}; 
             
                if (zx_7ffd_lastOut&8) 
				{
                    p_zx_video_ramATTR =  p_zx_video_ramATTR7;
					p_zx_video_ram = p_zx_video_ram7;
				}
               else 
				 {
                    p_zx_video_ramATTR = p_zx_video_ramATTR5;
					p_zx_video_ram = p_zx_video_ram5;
				 }

          


			if (x>=(SCREEN_W)||y>=(SCREEN_H))
			{
				x+=8;
				draw_img_inx+=4;
				continue;
			} 
		
//----------------------------------------------------------------

#ifdef TEST
if (wait_msg !=0)
{
		if (((y >= 240-12) && (y <= 240)))
			{
				int i_c;
				if (x < dx)
					i_c = MIN((dx - x) / 2, img_inx - draw_img_inx);
				else
					i_c = MIN((SCREEN_W - x) / 2, img_inx - draw_img_inx);
				draw_img_inx += i_c;
				x += i_c << 1;
				continue;
			}
}			
#endif
			//----------------------------------------------------------------------------------------

			if((y<dy)||(y>=192+dy)||(x>=256+dx)||(x<dx)){//условия для бордера
				int i_c;
				if (x<dx) i_c=MIN((dx-x)/2,img_inx-draw_img_inx);
				else i_c=MIN((SCREEN_W-x)/2,img_inx-draw_img_inx);
				
				 register uint8_t bc=zx_Border_color;     
				for(int i=i_c;i--;) *p_scr_str_buf++=bc;
				draw_img_inx+=i_c;
				x+=i_c<<1;
				continue;
			}

*p_zx_video_ramATTR5++;
*p_zx_video_ram5++;
*p_zx_video_ramATTR7++;
*p_zx_video_ram7++;

            uint8_t c_attr=*p_zx_video_ramATTR;
			uint8_t zx_pix8=*p_zx_video_ram;

			
			if (old_c_attr!=c_attr)//если аттрибуты цвета не поменялись - используем последовательность с прошлого шага
			{
				colorBuf=*((zx_colors_2_pix32+c_attr));
				old_c_attr=c_attr;			
			}
			
			
			//вывод блока из 8 пикселей
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0xc0)>>3);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x30)>>1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x0c)<<1);
			*p_scr_str_buf++=colorBuf>>(((zx_pix8)&0x03)<<3);
			x+=8;
			draw_img_inx+=4;
			
		};


		continue; 

	}//while(1)
};


void zx_machine_enable_vbuf(bool en_vbuf){
	vbuf_en=en_vbuf;
};

//====================================================================
//
//void (*out_z80)(); // определение указателя на функцию записи в порт IN Z80
//void (*in_z80)(); // определение указателя на функцию чтения из порта OUT Z80

//byte (*RdZ80)(uint8_t);// определение указателя на функцию чтения памяти

void init_mashine_and_extram(uint8_t config_mashine) // инициализация кофигурации портов переключения памяти
{
//shift_img=(16+40)*224+48;////8888;////Пентагон=(16+40)*224+48;
/*
Длина кадра в тактах процессора: 320*224=71680
16 строк кадрового синхроимпульса
16 невидимых строк верхнего бордюра
48 видимых строк верхнего бордюра
192 строки растрового экрана
48 строк нижнего бордюра
(16+64)*224 = 17920й такт — начало первой строки экрана с растровой картинкой
17920+32 = 17952й такт — начало вывода левого бордюра первой растровой строки
17920+32+36 = 17988й такт — начало вывода растровой картинки первой растровой строки
*/
//conf.shift_img=(((16+40)*224)+48);//
    conf.shift_img=12582;
    main_nmi_key = false;
    select_cpu_z80(z1);

	switch (config_mashine)
	{
	case PENT128 :
        machine_Pentagon_128(z1);
		break; //

	case SPEC48:
		machine_Spectrum_48(z1);
		break; //

	case PENT512:
         machine_Pentagon_512(z1);
		break; //

	case PENT1024:
    machine_Pentagon_1024(z1);
		break; //
	case SCORP256:
        main_nmi_key = true;
        machine_Scorpion_256(z1);   
		break; //
 
    // #ifndef NO_GMX     
	case GMX2048:
        machine_Scorpion_GMX(z1);
		break; //
    //  #endif    

	case NOVA256:
          main_nmi_key = true;
          machine_NOVA_256(z1);
		break; //

	case PENT8M:
          machine_MurmoZavr(z1);
		break; //

	case PENT_512CASH :// Пентагон 512 с кеш
        main_nmi_key = true;
        machine_Pentagon_512_cash(z1);
		break; //

	default:
        machine_Pentagon_128(z1);
		break;
	}

}
//########################
typedef enum {
    INT_DISABLED,      // DI активен
    INT_ENABLED,       // EI активен, готов к прерываниям
} InterruptState;
//
InterruptState get_interrupt_state(Z80* cpu) {
    if (cpu->iff1 == 0) {
        return INT_DISABLED;
    }
    
    // В реальном железе после EI есть задержка в одну инструкцию
    // В эмуляторе это может быть отдельным флагом или логикой
    // Проверяем, не была ли только что выполнена команда EI
    
    // Если ваш эмулятор отслеживает EI delay:
   // if (cpu->ei_executed && !cpu->ei_delay_passed) {
    //     return INT_EI_DELAY;
    // }
    
    return INT_ENABLED;
}
void     print_interrupt_status(Machine* z1) {
        switch (get_interrupt_state(&z1->cpu)) {
        case INT_DISABLED:
        // printf("DI активен - прерывания ЗАПРЕЩЕНЫ\n");
        draw_text(0 + 10, 10+ 9*14, "DI",CL_RED , CL_BLACK); // 
            break;
        case INT_ENABLED:
        //    printf("EI активен - прерывания РАЗРЕШЕНЫ\n");
        draw_text(0 + 10, 10+ 9*14, "EI",CL_GREEN , CL_BLACK); // 
            break;
    }
}    
//########################
//=========================================================
void disassembler(void) // END
{

   // write_z80(z1,0x81fe,0x18); //0x38
 //write_z80(z1,0x8207,0x00); //0x76
 //address_pc =Z80_PC(z1->cpu);
	draw_rect(0, 10, 318, 210, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(0, 10, 318, 210, CL_GRAY, false);				   // рамка 1
	///draw_rect(0 + 2, 10 + 2, 318 - 4, 200 - 4, CL_GRAY, false); // рамка 2

	//draw_rect(0 + 3, 10 + 3, 318 - 6, 8, CL_GRAY, true);			 // шапка меню
	//draw_text(0 + 10, 10 + 3, "ZX Keyboard  [ESC] Exit", CL_BLACK, CL_GRAY); // шапка меню
       uint16_t y=9;
      snprintf(temp_msg, sizeof temp_msg, "PC %04X", Z80_PC(z1->cpu));
      draw_text(0 + 10, 10 + y, temp_msg,CL_GRAY , CL_BLACK); //

       snprintf(temp_msg, sizeof temp_msg, "SP %04X", Z80_SP(z1->cpu));
      draw_text(0 + 10, 10 + y*2, temp_msg,CL_GRAY , CL_BLACK); //    

       snprintf(temp_msg, sizeof temp_msg, "IX %04X", Z80_IX(z1->cpu));
      draw_text(0 + 10, 10 + y*3, temp_msg,CL_GRAY , CL_BLACK); //    

        snprintf(temp_msg, sizeof temp_msg, "IY %04X", Z80_IY(z1->cpu));
      draw_text(0 + 10, 10 + y*4, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "HL %04X",  Z80_HL(z1->cpu) );
      draw_text(0 + 10, 10 + y*5, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "DE %04X",  Z80_DE(z1->cpu));
      draw_text(0 + 10, 10 + y*6, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "BC %04X",  Z80_BC(z1->cpu) );
      draw_text(0 + 10, 10 + y*7, temp_msg,CL_GRAY , CL_BLACK); // 

        snprintf(temp_msg, sizeof temp_msg, "AF %04X",  Z80_AF(z1->cpu));
      draw_text(0 + 10, 10 + y*8, temp_msg,CL_GRAY , CL_BLACK); // 

       snprintf(temp_msg, sizeof temp_msg, "HL'%04X",  Z80_HL_(z1->cpu));
      draw_text(0 + 10, 10 + y*9, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "DE'%04X",  Z80_DE_(z1->cpu));
      draw_text(0 + 10, 10 + y*10, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "BC'%04X",  Z80_BC_(z1->cpu));
      draw_text(0 + 10, 10 + y*11, temp_msg,CL_GRAY , CL_BLACK); // 

        snprintf(temp_msg, sizeof temp_msg, "AF'%04X",  (Z80_AF_(z1->cpu)));
      draw_text(0 + 10, 10 + y*12, temp_msg,CL_GRAY , CL_BLACK); // 

     
        snprintf(temp_msg, sizeof temp_msg, "IR %02X%02X",  z1->cpu.i,z1->cpu.r);
        draw_text(0 + 10, 10+ y*13, temp_msg,CL_GRAY , CL_BLACK); // 



        print_interrupt_status(z1);
       
        snprintf(temp_msg, sizeof temp_msg, "IM%01X",  z1->cpu.im);
        draw_text(10+18, 10+ y*14, temp_msg,CL_GRAY , CL_BLACK); // 


       draw_text(0 + 10, 10+ y*16, "ports",CL_GREEN , CL_BLACK); //
        snprintf(temp_msg, sizeof temp_msg, "7FFD %02X",  zx_7ffd_lastOut );
      draw_text(0 + 10, 10+ y*17, temp_msg,CL_GRAY , CL_BLACK); // 
	    snprintf(temp_msg, sizeof temp_msg, "1FFD %02X",  zx_1ffd_lastOut );
      draw_text(0 + 10, 10+ y*18, temp_msg,CL_GRAY , CL_BLACK); // 

	   draw_text(0 + 10, 10+ y*19, "pages",CL_GREEN , CL_BLACK); //
	    snprintf(temp_msg, sizeof temp_msg, "ROM %02X",  rom );
        switch (rom)
		{
		case 0:
			snprintf(temp_msg, sizeof temp_msg, "ROM B128");
			break;		
		case 1:
			snprintf(temp_msg, sizeof temp_msg, "ROM B48 ");
			break;
		case 2:
			snprintf(temp_msg, sizeof temp_msg, "ROM DOS ");
			break;
		case 3:
			snprintf(temp_msg, sizeof temp_msg, "ROM SM ");
			break;						
		default:
			break;
		}
      draw_text(0 + 10, 10+ y*20, temp_msg,CL_GRAY , CL_BLACK); // 		   
	 
	    snprintf(temp_msg, sizeof temp_msg, "RAM %04X",  zx_RAM_bank_active  );
      draw_text(0 + 10, 10+ y*21, temp_msg,CL_GRAY , CL_BLACK); // 


           
//list_disassm(uint16_t address_pc );



}
//---------------------------------------------------------
 void list_disassm()
 {
	address_pc=dis_adres;
	uint16_t y=9;
    uint8_t color_adress; 
    uint8_t color_dis; 
 for (int i = 0; i < 21; i++)
            {         

                 snprintf(temp_msg, sizeof temp_msg, "%04X ",  address_pc );
                    if (address_pc==Z80_PC(z1->cpu))//dis_adres =Z80_PC(z1->cpu);
                    {
                           color_adress = CL_RED ; //  address
                           color_dis = CL_LT_RED ; //  dissasm
                    }      
                    else 
                    {
                      color_adress = CL_CYAN ; 
                      color_dis = CL_LT_CYAN; 
                    }
                     draw_text(0 + 70, 20+ y*i, temp_msg,color_adress , CL_BLACK); //  address
                  
				// address_pc = address_pc +
				
				 switch (opcode_z80())
				 {
				 case 1:
				snprintf(temp_msg, sizeof temp_msg, "%02X            ", read_z80(z1,address_pc) );//1 byte
				address_pc=address_pc+1;
					break;
				 case 2:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X        ", read_z80(z1,address_pc),read_z80(z1,address_pc+1) );//2 byte
				address_pc=address_pc+2;
					break;	
				 case 3:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X%02X    ", read_z80(z1,address_pc),read_z80(z1,address_pc+1),read_z80(z1,address_pc+2) );//3 byte
				address_pc=address_pc+3;
					break;	
				 case 4:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X%02X%02X", read_z80(z1,address_pc),read_z80(z1,address_pc+1),read_z80(z1,address_pc+2),read_z80(z1,address_pc+3) );//4byte
				address_pc=address_pc+4;
					break;															 
				 default:
					break;
				 }


				  draw_text(0 + 100, 20+ y*i, temp_msg,CL_GRAY , CL_BLACK); // 

                  draw_text_len(0 + 160, 20+ y*i, tmp_opcode,color_dis , CL_BLACK,14); // 
			}



}
void list_dump()
 {
	address_pc=dis_adres;
	uint16_t y=9;
 for (int i = 0; i < 21; i++)
            {         
                 snprintf(temp_msg, sizeof temp_msg, "%04X ",  address_pc );
                     draw_text(0 + 70, 20+ y*i, temp_msg,CL_CYAN , CL_BLACK); //  address
                  
				// address_pc = address_pc +
				
			//	
				 {
					uint8_t b[9];
                    uint8_t s[9];
                    uint8_t c;
                    for (int j = 0; j < 8; j++)
					{
                    c = read_z80(z1,address_pc+j);
                     b[j]=c;
                     if (c<0x20) c = 176;
                      if (c=='%') c = 176;
                    if (c>128) c = 176;
                      s[j]=c;
					}
                    s[8]=0;
					b[8]=0;
				 snprintf(temp_msg, sizeof temp_msg, "%02X %02X %02X %02X %02X %02X %02X %02X  %s",
				 b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],s
	
				  );//8byte
/* 				snprintf(temp_msg, sizeof temp_msg, "%02X %02X %02X %02X %02X %02X %02X %02X",
				 cpu.read_byte(0,address_pc),cpu.read_byte(0,address_pc+1),cpu.read_byte(0,address_pc+2),cpu.read_byte(0,address_pc+3),
				 cpu.read_byte(0,address_pc+4),cpu.read_byte(0,address_pc+5),cpu.read_byte(0,address_pc+6),cpu.read_byte(0,address_pc+7)
				  );//8byte */
				address_pc=address_pc+8;

				 }


				  draw_text(0 + 100, 20+ y*i, temp_msg,CL_GRAY , CL_BLACK); // 

               //   draw_text_len(0 + 160, 20+ y*i, tmp_opcode,CL_LT_CYAN , CL_BLACK,16); // 
			}



}

//---------------------------------------------------------
// opcode name z80
	const char __in_flash() *opcode_tab[]={
//const static char* mnem[256] {
    "NOP", // 00
    "LD BC,%02X%02X", // 01
    "LD (BC),A", // 02
    "INC BC", // 03
    "INC B", // 04
    "DEC B", // 05
    "LD B,%02X", // 06
    "RLCA", // 07
    "EX AF,AF'", // 08
    "ADD HL,BC", // 09
    "LD A,(BC)", // 0A
    "DEC BC", // 0B
    "INC C", // 0C
    "DEC C", // 0D
    "LD C,%02X", // 0E
    "RRCA", // 0F

    "DJNZ %04X", // 10
    "LD DE,%02X%02X", // 11
    "LD (DE),A", // 12
    "INC DE", // 13
    "INC D", // 14
    "DEC D", // 15
    "LD D,%02X", // 16
    "RLA", // 17
    "JR %04X", // 18
    "ADD HL,DE", // 19
    "LD A,(DE)", // 1A
    "DEC DE", // 1B
    "INC E", // 1C
    "DEC E", // 1D
    "LD E,%02X", // 1E
    "RRA", // 1F

    "JR NZ %04X", // 20
    "LD HL,%02X%02X", // 21
    "LD (%02X%02X),HL", // 22
    "INC HL", // 23
    "INC H", // 24
    "DEC H", // 25
    "LD H,%02X", // 26
    "DAA", // 27
    "JR Z,%04X", // 28
    "ADD HL,HL", // 29
    "LD HL,(%02X%02X)", // 2A
    "DEC HL", // 2B
    "INC L", // 2C
    "DEC L", // 2D
    "LD L,%02X", // 2E
    "CPL", // 2F

    "JR NC %04X", // 30
    "LD SP,%02X%02X", // 31
    "LD (%02X%02X),A", // 32
    "INC SP", // 33
    "INC (HL)", // 34
    "DEC (HL)", // 35
    "LD (HL),%02X", // 36
    "SCF", // 37
    "JR C,%04X", // 38
    "ADD HL,SP", // 39
    "LD A,(%02X%02X)", // 3A
    "DEC SP", // 3B
    "INC A", // 3C
    "DEC A", // 3D
    "LD A,%02X", // 3E
    "CCF", // 3F

    "LD B,B" , // 40
    "LD B,C", // 41
    "LD B,D", // 42
    "LD B,E", // 43
    "LD B,H", // 44
    "LD B,L", // 45
    "LD B,(HL)", // 46
    "LD B,A", // 47
    "LD C,B", // 48
    "LD C,C", // 49
    "LD C,D", // 4A
    "LD C,E", // 4B
    "LD C,H", // 4C
    "LD C,L", // 4D
    "LD C,(HL)", // 4E
    "LD C,A", // 4F

    "LD D,B", // 50
    "LD D,C", // 51
    "LD D,D", // 52
    "LD D,E", // 53
    "LD D,H", // 54
    "LD D,L", // 55
    "LD D,(HL)", // 56
    "LD D,A", // 57
    "LD E,B", // 58
    "LD E,C", // 59
    "LD E,D", // 5A
    "LD E,E", // 5B
    "LD E,H", // 5C
    "LD E,L", // 5D
    "LD E,(HL)", // 5E
    "LD E,A", // 5F

    "LD H,B", // 60
    "LD H,C", // 61
    "LD H,D", // 62
    "LD H,E", // 63
    "LD H,H", // 64
    "LD H,L", // 65
    "LD H,(HL)", // 66
    "LD H,A", // 67
    "LD L,B", // 68
    "LD L,C", // 69
    "LD L,D", // 6A
    "LD L,E", // 6B
    "LD L,H", // 6C
    "LD L,L", // 6D
    "LD L,(HL)", // 6E
    "LD L,A", // 6F

    "LD (HL),B", // 70
    "LD (HL),C", // 71
    "LD (HL),D", // 72
    "LD (HL),E", // 73
    "LD (HL),H", // 74
    "LD (HL),L", // 75
    "HALT", // 76
    "LD (HL),A", // 77
    "LD A,B", // 78
    "LD A,C", // 79
    "LD A,D", // 7A
    "LD A,E", // 7B
    "LD A,H", // 7C
    "LD A,L", // 7D
    "LD A,(HL)", // 7E
    "LD A,A", // 7F

    "ADD B", // 80
    "ADD C", // 81
    "ADD D", // 82
    "ADD E", // 83
    "ADD H", // 84
    "ADD L", // 85
    "ADD (HL)", // 86
    "ADD A", // 87
    "ADC B", // 88
    "ADC C", // 89
    "ADC D", // 8A
    "ADC E", // 8B
    "ADC H", // 8C
    "ADC L", // 8D
    "ADC (HL)", // 8E
    "ADC A", // 8F

    "SUB B", // 90
    "SUB C", // 91
    "SUB D", // 92
    "SUB E", // 93
    "SUB H", // 94
    "SUB L", // 95
    "SUB (HL)", // 96
    "SUB A", // 97
    "SBC B", // 98
    "SBC C", // 99
    "SBC D", // 9A
    "SBC E", // 9B
    "SBC H", // 9C
    "SBC L", // 9D
    "SBC (HL)", // 9E
    "SBC A", // 9F

    "AND B", // A0
    "AND C", // A1
    "AND D", // A2
    "AND E", // A3
    "AND H", // A4
    "AND L", // A5
    "AND (HL)", // A6
    "AND A", // A7
    "XOR B", // A8
    "XOR C", // A9
    "XOR D", // AA
    "XOR E", // AB
    "XOR H", // AC
    "XOR L", // AD
    "XOR (HL)", // AE
    "XOR A", // AF

    "OR B", // B0
    "OR C", // B1
    "OR D", // B2
    "OR E", // B3
    "OR H", // B4
    "OR L", // B5
    "OR (HL)", // B6
    "OR A", // B7
    "CP B", // B8
    "CP C", // B9
    "CP D", // BA
    "CP E", // BB
    "CP H", // BC
    "CP L", // BD
    "CP (HL)", // BE
    "CP A", // BF

    "RET NZ", // C0
    "POP BC", // C1
    "JP NZ,%02X%02X", // C2
    "JP %02X%02X", // C3
    "CALL NZ,%02X%02X", // C4
    "PUSH BC", // C5
    "ADD %02X", // C6
    "RST 0", // C7
    "RET Z", // C8
    "RET", // C9
    "JP Z,%02X%02X", // CA
    "bo", // CB
    "CALL Z,%02X%02X", // CC
    "CALL %02X%02X", // CD
    "ADC %02X", // CE
    "RST 8", // CF

    "RET NC", // D0
    "POP DE", // D1
    "JP NC,%02X%02X", // D2
    "OUT (%02X),A", // D3
    "CALL NC,%02X%02X", // D4
    "PUSH DE", // D5
    "SUB %02X", // D6
    "RST 10", // D7
    "RET C", // D8
    "EXX", // D9
    "JP C,%02X%02X", // DA
    "IN A,(%02X)", // DB
    "CALL C,%02X%02X", // DC
    "op IX:", // DD
    "SBC %02X", // DE
    "RST 18", // DF

    "RET PO", // E0
    "POP HL", // E1
    "JP PO,%02X%02X", // E2
    "EX (SP),HL", // E3
    "CALL PO,%02X%02X", // E4
    "PUSH HL", // E5
    "AND %02X", // E6
    "RST 20", // E7
    "RET PE", // E8
    "JP (HL)", // E9
    "JP PE,%02X%02X", // EA
    "EX DE,HL", // EB
    "CALL PE,%02X%02X", // EC
    "ext:", // ED
    "XOR %02X", // EE
    "RST 28", // EF

    "RET P", // F0
    "POP AF", // F1
    "JP P,%02X%02X", // F2
    "DI", // F3
    "CALL P,%02X%02X", // F4
    "PUSH AF", // F5
    "OR %02X", // F6
    "RST 30", // F7
    "RET M", // F8
    "LD SP,HL", // F9
    "JP M,%02X%02X", // FA
    "EI", // FB
    "CALL M,%02X%02X", // FC
    "op IY:", // FD
    "CP %02X", // FE
    "RST 38", // FF
};
//
const char __in_flash() *opcode_CB[]={
    "RLC B", // 00
    "RLC C", // 01
    "RLC D", // 02
    "RLC E", // 03
    "RLC H", // 04
    "RLC L", // 05
    "RLC (HL)", // 06
    "RLC A", // 07
    "RRC B", // 08
    "RRC C", // 09
    "RRC D", // 0A
    "RRC E", // 0B
    "RRC H", // 0C
    "RRC L", // 0D
    "RRC (HL)", // 0E
    "RRC A", // 0F

    "RL B", // 10
    "RL C", // 11
    "RL D", // 12
    "RL E", // 13
    "RL H", // 14
    "RL L", // 15
    "RL (HL)", // 16
    "RL A", // 17
    "RR B", // 18
    "RR C", // 19
    "RR D", // 1A
    "RR E", // 1B
    "RR H", // 1C
    "RR L", // 1D
    "RR (HL)", // 1E
    "RR A", // 1F

    "SLA B", // 20
    "SLA C", // 21
    "SLA D", // 22
    "SLA E", // 23
    "SLA H", // 24
    "SLA L", // 25
    "SLA (HL)", // 26
    "SLA A", // 27
    "SRA B", // 28
    "SRA C", // 29
    "SRA D", // 2A
    "SRA E", // 2B
    "SRA H", // 2C
    "SRA L", // 2D
    "SRA (HL)", // 2E
    "SRA A", // 2F

    "SLL B", // 30
    "SLL C", // 31
    "SLL D", // 32
    "SLL E", // 33
    "SLL H", // 34
    "SLL L", // 35
    "SLL (HL)", // 36
    "SLL A", // 37
    "SRL B", // 38
    "SRL C", // 39
    "SRL D", // 3A
    "SRL E", // 3B
    "SRL H", // 3C
    "SRL L", // 3D
    "SRL (HL)", // 3E
    "SRL A", // 3F

    "BIT 0,B", // 40
    "BIT 0,C", // 41
    "BIT 0,D", // 42
    "BIT 0,E", // 43
    "BIT 0,H", // 44
    "BIT 0,L", // 45
    "BIT 0,(HL)", // 46
    "BIT 0,A", // 47
    "BIT 1,B", // 48
    "BIT 1,C", // 49
    "BIT 1,D", // 4A
    "BIT 1,E", // 4B
    "BIT 1,H", // 4C
    "BIT 1,L", // 4D
    "BIT 1,(HL)", // 4E
    "BIT 1,A", // 4F

    "BIT 2,B", // 50
    "BIT 2,C", // 51
    "BIT 2,D", // 52
    "BIT 2,E", // 53
    "BIT 2,H", // 54
    "BIT 2,L", // 55
    "BIT 2,(HL)", // 56
    "BIT 2,A", // 57
    "BIT 3,B", // 58
    "BIT 3,C", // 59
    "BIT 3,D", // 5A
    "BIT 3,E", // 5B
    "BIT 3,H", // 5C
    "BIT 3,L", // 5D
    "BIT 3,(HL)", // 5E
    "BIT 3,A", // 5F

    "BIT 4,B", // 60
    "BIT 4,C", // 61
    "BIT 4,D", // 62
    "BIT 4,E", // 63
    "BIT 4,H", // 64
    "BIT 4,L", // 65
    "BIT 4,(HL)", // 66
    "BIT 4,A", // 67
    "BIT 5,B", // 68
    "BIT 5,C", // 69
    "BIT 5,D", // 6A
    "BIT 5,E", // 6B
    "BIT 5,H", // 6C
    "BIT 5,L", // 6D
    "BIT 5,(HL)", // 6E
    "BIT 5,A", // 6F

    "BIT 6,B", // 70
    "BIT 6,C", // 71
    "BIT 6,D", // 72
    "BIT 6,E", // 73
    "BIT 6,H", // 74
    "BIT 6,L", // 75
    "BIT 6,(HL)", // 76
    "BIT 6,A", // 77
    "BIT 7,B", // 78
    "BIT 7,C", // 79
    "BIT 7,D", // 7A
    "BIT 7,E", // 7B
    "BIT 7,H", // 7C
    "BIT 7,L", // 7D
    "BIT 7,(HL)", // 7E
    "BIT 7,A", // 7F

    "RES 0,B", // 80
    "RES 0,C", // 81
    "RES 0,D", // 82
    "RES 0,E", // 83
    "RES 0,H", // 84
    "RES 0,L", // 85
    "RES 0,(HL)", // 86
    "RES 0,A", // 87
    "RES 1,B", // 88
    "RES 1,C", // 89
    "RES 1,D", // 8A
    "RES 1,E", // 8B
    "RES 1,H", // 8C
    "RES 1,L", // 8D
    "RES 1,(HL)", // 8E
    "RES 1,A", // 8F

    "RES 2,B", // 90
    "RES 2,C", // 91
    "RES 2,D", // 92
    "RES 2,E", // 93
    "RES 2,H", // 94
    "RES 2,L", // 95
    "RES 2,(HL)", // 96
    "RES 2,A", // 97
    "RES 3,B", // 98
    "RES 3,C", // 99
    "RES 3,D", // 9A
    "RES 3,E", // 9B
    "RES 3,H", // 9C
    "RES 3,L", // 9D
    "RES 3,(HL)", // 9E
    "RES 3,A", // 9F

    "RES 4,B", // A0
    "RES 4,C", // A1
    "RES 4,D", // A2
    "RES 4,E", // A3
    "RES 4,H", // A4
    "RES 4,L", // A5
    "RES 4,(HL)", // A6
    "RES 4,A", // A7
    "RES 5,B", // A8
    "RES 5,C", // A9
    "RES 5,D", // AA
    "RES 5,E", // AB
    "RES 5,H", // AC
    "RES 5,L", // AD
    "RES 5,(HL)", // AE
    "RES 5,A", // AF

    "RES 6,B", // B0
    "RES 6,C", // B1
    "RES 6,D", // B2
    "RES 6,E", // B3
    "RES 6,H", // B4
    "RES 6,L", // B5
    "RES 6,(HL)", // B6
    "RES 6,A", // B7
    "RES 7,B", // B8
    "RES 7,C", // B9
    "RES 7,D", // BA
    "RES 7,E", // BB
    "RES 7,H", // BC
    "RES 7,L", // BD
    "RES 7,(HL)", // BE
    "RES 7,A", // BF

    "SET 0,B", // C0
    "SET 0,C", // C1
    "SET 0,D", // C2
    "SET 0,E", // C3
    "SET 0,H", // C4
    "SET 0,L", // C5
    "SET 0,(HL)", // C6
    "SET 0,A", // C7
    "SET 1,B", // C8
    "SET 1,C", // C9
    "SET 1,D", // CA
    "SET 1,E", // CB
    "SET 1,H", // CC
    "SET 1,L", // CD
    "SET 1,(HL)", // CE
    "SET 1,A", // CF

    "SET 2,B", // 90
    "SET 2,C", // D1
    "SET 2,D", // D2
    "SET 2,E", // D3
    "SET 2,H", // D4
    "SET 2,L", // D5
    "SET 2,(HL)", // D6
    "SET 2,A", // D7
    "SET 3,B", // D8
    "SET 3,C", // D9
    "SET 3,D", // DA
    "SET 3,E", // DB
    "SET 3,H", // DC
    "SET 3,L", // DD
    "SET 3,(HL)", // DE
    "SET 3,A", // DF

    "SET 4,B", // E0
    "SET 4,C", // E1
    "SET 4,D", // E2
    "SET 4,E", // E3
    "SET 4,H", // E4
    "SET 4,L", // E5
    "SET 4,(HL)", // E6
    "SET 4,A", // E7
    "SET 5,B", // E8
    "SET 5,C", // E9
    "SET 5,D", // EA
    "SET 5,E", // EB
    "SET 5,H", // EC
    "SET 5,L", // ED
    "SET 5,(HL)", // EE
    "SET 5,A", // EF

    "SET 6,B", // F0
    "SET 6,C", // F1
    "SET 6,D", // F2
    "SET 6,E", // F3
    "SET 6,H", // F4
    "SET 6,L", // F5
    "SET 6,(HL)", // F6
    "SET 6,A", // F7
    "SET 7,B", // F8
    "SET 7,C", // F9
    "SET 7,D", // FA
    "SET 7,E", // FB
    "SET 7,H", // FC
    "SET 7,L", // FD
    "SET 7,(HL)", // FE
    "SET 7,A" // FF
};
//
const char __in_flash() *opcode_CB_I[]={
    "RLC (%s%02X),B", // 00
    "RLC (%s%02X),C", // 01
    "RLC (%s%02X),D", // 02
    "RLC (%s%02X),E", // 03
    "RLC (%s%02X),H", // 04
    "RLC (%s%02X),L", // 05
    "RLC (%s%02X)", // 06
    "RLC (%s%02X),A", // 07
    "RRC (%s%02X),B", // 08
    "RRC (%s%02X),C", // 09
    "RRC (%s%02X),D", // 0A
    "RRC (%s%02X),E", // 0B
    "RRC (%s%02X),H", // 0C
    "RRC (%s%02X),L", // 0D
    "RRC (%s%02X)", // 0E
    "RRC (%s%02X),A", // 0F

    "RL (%s%02X),B", // 10
    "RL (%s%02X),C", // 11
    "RL (%s%02X),D", // 12
    "RL (%s%02X),E", // 13
    "RL (%s%02X),H", // 14
    "RL (%s%02X),L", // 15
    "RL (%s%02X)", // 16
    "RL (%s%02X),A", // 17
    "RR (%s%02X),B", // 18
    "RR (%s%02X),C", // 19
    "RR (%s%02X),D", // 1A
    "RR (%s%02X),E", // 1B
    "RR (%s%02X),H", // 1C
    "RR (%s%02X),L", // 1D
    "RR (%s%02X)", // 1E
    "RR (%s%02X),A", // 1F

    "SLA (%s%02X),B", // 20
    "SLA (%s%02X),C", // 21
    "SLA (%s%02X),D", // 22
    "SLA (%s%02X),E", // 23
    "SLA (%s%02X),H", // 24
    "SLA (%s%02X),L", // 25
    "SLA (%s%02X)", // 26
    "SLA (%s%02X),A", // 27
    "SRA (%s%02X),B", // 28
    "SRA (%s%02X),C", // 29
    "SRA (%s%02X),D", // 2A
    "SRA (%s%02X),E", // 2B
    "SRA (%s%02X),H", // 2C
    "SRA (%s%02X),L", // 2D
    "SRA (%s%02X)", // 2E
    "SRA (%s%02X),A", // 2F

    "SLL (%s%02X),B", // 30
    "SLL (%s%02X), C", // 31
    "SLL (%s%02X),D", // 32
    "SLL (%s%02X),E", // 33
    "SLL (%s%02X),H", // 34
    "SLL (%s%02X),L", // 35
    "SLL (%s%02X)", // 36
    "SLL (%s%02X),A", // 37
    "SRL (%s%02X),B", // 38
    "SRL (%s%02X),C", // 39
    "SRL (%s%02X),D", // 3A
    "SRL (%s%02X),E", // 3B
    "SRL (%s%02X),H", // 3C
    "SRL (%s%02X),L", // 3D
    "SRL (%s%02X)", // 3E
    "SRL (%s%02X),A", // 3F

    "BIT 0,(%s%02X)", // 40
    "BIT 0,(%s%02X)", // 41
    "BIT 0,(%s%02X)", // 42
    "BIT 0,(%s%02X)", // 43
    "BIT 0,(%s%02X)", // 44
    "BIT 0,(%s%02X)", // 45
    "BIT 0,(%s%02X)", // 46
    "BIT 0,(%s%02X)", // 47
    "BIT 1,(%s%02X)", // 48
    "BIT 1,(%s%02X)", // 49
    "BIT 1,(%s%02X)", // 4A
    "BIT 1,(%s%02X)", // 4B
    "BIT 1,(%s%02X)", // 4C
    "BIT 1,(%s%02X)", // 4D
    "BIT 1,(%s%02X)", // 4E
    "BIT 1,(%s%02X)", // 4F

    "BIT 2,(%s%02X)", // 50
    "BIT 2,(%s%02X)", // 51
    "BIT 2,(%s%02X)", // 52
    "BIT 2,(%s%02X)", // 53
    "BIT 2,(%s%02X)", // 54
    "BIT 2,(%s%02X)", // 55
    "BIT 2,(%s%02X)", // 56
    "BIT 2,(%s%02X)", // 57
    "BIT 3,(%s%02X)", // 58
    "BIT 3,(%s%02X)", // 59
    "BIT 3,(%s%02X)", // 5A
    "BIT 3,(%s%02X)", // 5B
    "BIT 3,(%s%02X)", // 5C
    "BIT 3,(%s%02X)", // 5D
    "BIT 3,(%s%02X)", // 5E
    "BIT 3,(%s%02X)", // 5F

    "BIT 4,(%s%02X)", // 60
    "BIT 4,(%s%02X)", // 61
    "BIT 4,(%s%02X)", // 62
    "BIT 4,(%s%02X)", // 63
    "BIT 4,(%s%02X)", // 64
    "BIT 4,(%s%02X)", // 65
    "BIT 4,(%s%02X)", // 66
    "BIT 4,(%s%02X)", // 67
    "BIT 5,(%s%02X)", // 68
    "BIT 5,(%s%02X)", // 69
    "BIT 5,(%s%02X)", // 6A
    "BIT 5,(%s%02X)", // 6B
    "BIT 5,(%s%02X)", // 6C
    "BIT 5,(%s%02X)", // 6D
    "BIT 5,(%s%02X)", // 6E
    "BIT 5,(%s%02X)", // 6F

    "BIT 6,(%s%02X)", // 70
    "BIT 6,(%s%02X)", // 71
    "BIT 6,(%s%02X)", // 72
    "BIT 6,(%s%02X)", // 73
    "BIT 6,(%s%02X)", // 74
    "BIT 6,(%s%02X)", // 75
    "BIT 6,(%s%02X)", // 76
    "BIT 6,(%s%02X)", // 77
    "BIT 7,(%s%02X)", // 78
    "BIT 7,(%s%02X)", // 79
    "BIT 7,(%s%02X)", // 7A
    "BIT 7,(%s%02X)", // 7B
    "BIT 7,(%s%02X)", // 7C
    "BIT 7,(%s%02X)", // 7D
    "BIT 7,(%s%02X)", // 7E
    "BIT 7,(%s%02X)", // 7F

    "RES 0,(%s%02X),B", // 80
    "RES 0,(%s%02X),C", // 81
    "RES 0,(%s%02X),D", // 82
    "RES 0,(%s%02X),E", // 83
    "RES 0,(%s%02X),H", // 84
    "RES 0,(%s%02X),L", // 85
    "RES 0,(%s%02X)", // 86 RES 0, (IY+d)
    "RES 0,(%s%02X),A", // 87
    "RES 1,(%s%02X),B", // 88
    "RES 1,(%s%02X),C", // 89
    "RES 1,(%s%02X),D", // 8A
    "RES 1,(%s%02X),E", // 8B
    "RES 1,(%s%02X),H", // 8C
    "RES 1,(%s%02X),L", // 8D
    "RES 1,(%s%02X)", // 8E
    "RES 1,(%s%02X),A", // 8F

    "RES 2,(%s%02X),B", // 90
    "RES 2,(%s%02X),C", // 91
    "RES 2,(%s%02X),D", // 92
    "RES 2,(%s%02X),E", // 93
    "RES 2,(%s%02X),H", // 94
    "RES 2,(%s%02X),L", // 95
    "RES 2,(%s%02X)", // 96
    "RES 2,(%s%02X),A", // 97
    "RES 3,(%s%02X),B", // 98
    "RES 3,(%s%02X),C", // 99
    "RES 3,(%s%02X),D", // 9A
    "RES 3,(%s%02X),E", // 9B
    "RES 3,(%s%02X),H", // 9C
    "RES 3,(%s%02X),L", // 9D
    "RES 3,(%s%02X)", // 9E
    "RES 3,(%s%02X),A", // 9F

    "RES 4,(%s%02X),B", // A0
    "RES 4,(%s%02X),C", // A1
    "RES 4,(%s%02X),D", // A2
    "RES 4,(%s%02X),E", // A3
    "RES 4,(%s%02X),H", // A4
    "RES 4,(%s%02X),L", // A5
    "RES 4,(%s%02X)", // A6
    "RES 4,(%s%02X),A", // A7
    "RES 5,(%s%02X),B", // A8
    "RES 5,(%s%02X),C", // A9
    "RES 5,(%s%02X),D", // AA
    "RES 5,(%s%02X),E", // AB
    "RES 5,(%s%02X),H", // AC
    "RES 5,(%s%02X),L", // AD
    "RES 5,(%s%02X)", // AE
    "RES 5,(%s%02X),A", // AF

    "RES 6,(%s%02X),B", // B0
    "RES 6,(%s%02X),C", // B1
    "RES 6,(%s%02X),D", // B2
    "RES 6,(%s%02X),E", // B3
    "RES 6,(%s%02X),H", // B4
    "RES 6,(%s%02X),L", // B5
    "RES 6,(%s%02X)", // B6
    "RES 6,(%s%02X),A", // B7
    "RES 7,(%s%02X),B", // B8
    "RES 7,(%s%02X),C", // B9
    "RES 7,(%s%02X),D", // BA
    "RES 7,(%s%02X),E", // BB
    "RES 7,(%s%02X),H", // BC
    "RES 7,(%s%02X),L", // BD
    "RES 7,(%s%02X)", // BE
    "RES 7,(%s%02X),A", // BF

    "SET 0,(%s%02X),B", // C0
    "SET 0,(%s%02X),C", // C1
    "SET 0,(%s%02X),D", // C2
    "SET 0,(%s%02X),E", // C3
    "SET 0,(%s%02X),H", // C4
    "SET 0,(%s%02X),L", // C5
    "SET 0,(%s%02X)", // C6
    "SET 0,(%s%02X),A", // C7
    "SET 1,(%s%02X),B", // C8
    "SET 1,(%s%02X),C", // C9
    "SET 1,(%s%02X),D", // CA
    "SET 1,(%s%02X),E", // CB
    "SET 1,(%s%02X),H", // CC
    "SET 1,(%s%02X),L", // CD
    "SET 1,(%s%02X)", // CE
    "SET 1,(%s%02X),A", // CF

    "SET 2(%s%02X),,B", // 90
    "SET 2,(%s%02X),C", // D1
    "SET 2,(%s%02X),D", // D2
    "SET 2,(%s%02X),E", // D3
    "SET 2,(%s%02X),H", // D4
    "SET 2,(%s%02X),L", // D5
    "SET 2,(%s%02X)", // D6
    "SET 2,(%s%02X),A", // D7
    "SET 3,(%s%02X),B", // D8
    "SET 3,(%s%02X),C", // D9
    "SET 3,(%s%02X),D", // DA
    "SET 3,(%s%02X),E", // DB
    "SET 3,(%s%02X),H", // DC
    "SET 3,(%s%02X),L", // DD
    "SET 3,(%s%02X)", // DE
    "SET 3,(%s%02X),A", // DF

    "SET 4,(%s%02X),B", // E0
    "SET 4,(%s%02X),C", // E1
    "SET 4,(%s%02X),D", // E2
    "SET 4,(%s%02X),E", // E3
    "SET 4,(%s%02X),H", // E4
    "SET 4,(%s%02X),L", // E5
    "SET 4,(%s%02X)", // E6
    "SET 4,(%s%02X),A", // E7
    "SET 5,(%s%02X),B", // E8
    "SET 5,(%s%02X),C", // E9
    "SET 5,(%s%02X),D", // EA
    "SET 5,(%s%02X),E", // EB
    "SET 5,(%s%02X),H", // EC
    "SET 5,(%s%02X),L", // ED
    "SET 5,(%s%02X)", // EE
    "SET 5,(%s%02X),A", // EF

    "SET 6,(%s%02X),B", // F0
    "SET 6,(%s%02X),C", // F1
    "SET 6,(%s%02X),D", // F2
    "SET 6,(%s%02X),E", // F3
    "SET 6,(%s%02X),H", // F4
    "SET 6,(%s%02X),L", // F5
    "SET 6,(%s%02X)", // F6
    "SET 6,(%s%02X),A", // F7
    "SET 7,(%s%02X),B", // F8
    "SET 7,(%s%02X),C", // F9
    "SET 7,(%s%02X),D", // FA
    "SET 7,(%s%02X),E", // FB
    "SET 7,(%s%02X),H", // FC
    "SET 7,(%s%02X),L", // FD
    "SET 7,(%s%02X)", // FE
    "SET 7,(%s%02X),A" // FF
};
//
const char* opcode_ED(uint8_t b) {
    switch(b) {
        case 0x40: return "IN B,(C)";
        case 0x41: return "OUT (C),B";
        case 0x42: return "SBC HL,BC";
        case 0x43: return "LD (%02X%02X),BC";
        case 0x44: return "NEG";
        case 0x45: return "RETN";
        case 0x46: return "IM 0";
        case 0x47: return "LD I,A";
        case 0x48: return "IN C,(C)";
        case 0x49: return "OUT (C),C";
        case 0x4A: return "ADC HL,BC";
        case 0x4B: return "LD BC,(%02X%02X)";
        case 0x4C: return "NEG";
        case 0x4D: return "RETI";
        case 0x4E: return "IM 0/1";
        case 0x4F: return "LD R,A";

        case 0x50: return "IN D,(C)";
        case 0x51: return "OUT (C),D";
        case 0x52: return "SBC HL,DE";
        case 0x53: return "LD (%02X%02X),DE";
        case 0x54: return "NEG";
        case 0x55: return "RETN";
        case 0x56: return "IM 1";
        case 0x57: return "LD A,I";
        case 0x58: return "IN E,(C)";
        case 0x59: return "OUT (C),E";
        case 0x5A: return "ADC HL,DE";
        case 0x5B: return "LD DE,(%02X%02X)";
        case 0x5C: return "NEG";
        case 0x5D: return "RETN";
        case 0x5E: return "IM 2";
        case 0x5F: return "LD A,R";

        case 0x60: return "IN H,(C)";
        case 0x61: return "OUT (C),H";
        case 0x62: return "SBC HL,HL";
        case 0x63: return "LD (%02X%02X),HL";
        case 0x64: return "NEG";
        case 0x65: return "RETN";
        case 0x66: return "IM 0";
        case 0x67: return "RRD";
        case 0x68: return "IN L,(C)";
        case 0x69: return "OUT (C),L";
        case 0x6A: return "ADC HL,HL";
        case 0x6B: return "LD HL,(%02X%02X)";
        case 0x6C: return "NEG";
        case 0x6D: return "RETN";
        case 0x6E: return "IM 0/1";
        case 0x6F: return "RLD";

        case 0x70: return "IN F,(C)";
        case 0x71: return "OUT (C),0";
        case 0x72: return "SBC HL,SP";
        case 0x73: return "LD (%02X%02X),SP";
        case 0x74: return "NEG";
        case 0x75: return "RETN";
        case 0x76: return "IM 1";

        case 0x78: return "IN A,(C)";
        case 0x79: return "OUT (C),A";
        case 0x7A: return "ADC HL,SP";
        case 0x7B: return "LD SP,(%02X%02X)";
        case 0x7C: return "NEG";
        case 0x7D: return "RETN";
        case 0x7E: return "IM 2";

        case 0xA0: return "LDI";
        case 0xA1: return "CPI";
        case 0xA2: return "INI";
        case 0xA3: return "OUTI";

        case 0xA8: return "LDD";
        case 0xA9: return "CPD";
        case 0xAA: return "IND";
        case 0xAB: return "OUTD";

        case 0xB0: return "LDIR";
        case 0xB1: return "CPIR";
        case 0xB2: return "INIR";
        case 0xB3: return "OUTIR";

        case 0xB8: return "LDDR";
        case 0xB9: return "CPDR";
        case 0xBA: return "INDR";
        case 0xBB: return "OUTDR";
    }
    return "UNKNOW";
}
//
const char* opcode_DD(uint8_t b) {
    switch(b) {
        case 0x09: return "ADD IX,BC";

        case 0x19: return "ADD IX,DE";

        case 0x21: return "LD IX,%02X%02X";
        case 0x22: return "LD (%02X%02X),IX";
        case 0x23: return "INC IX";
        case 0x24: return "INC IXh";
        case 0x25: return "DEC IXh";
        case 0x26: return "LD IXh,%02X";

        case 0x29: return "ADD IX,IX";
        case 0x2A: return "LD IX,(%02X%02X)";
        case 0x2B: return "DEC IX";
        case 0x2C: return "INC IXl";
        case 0x2D: return "DEC IXl";
        case 0x2E: return "LD IXl,%02X";

        case 0x34: return "INC (IX+%02X)";
        case 0x35: return "DEC (IX+%02X)";
        case 0x36: return "LD (IX+%02X),%02X";

        case 0x39: return "ADD IX,SP";

        case 0x44: return "LD B,IXh";
        case 0x45: return "LD B,IXl";
        case 0x46: return "LD B,(IX+%02X)";

        case 0x4C: return "LD C,IXh";
        case 0x4D: return "LD C,IXl";
        case 0x4E: return "LD C,(IX+%02X)";

        case 0x54: return "LD D,IXh";
        case 0x55: return "LD D,IXl";
        case 0x56: return "LD D,(IX+%02X)";

        case 0x5C: return "LD E,IXh";
        case 0x5D: return "LD E,IXl";
        case 0x5E: return "LD E,(IX+%02X)";

        case 0x60: return "LD IXh,B";
        case 0x61: return "LD IXh,C";
        case 0x62: return "LD IXh,D";
        case 0x63: return "LD IXh,E";
        case 0x64: return "LD IXh,IXh";
        case 0x65: return "LD IXh,IXl";
        case 0x66: return "LD H,(IX+%02X)";
        case 0x67: return "LD IXh,A";
        case 0x68: return "LD IXl,B";
        case 0x69: return "LD IXl,C";
        case 0x6A: return "LD IXl,D";
        case 0x6B: return "LD IXl,E";
        case 0x6C: return "LD IXl,IXh";
        case 0x6D: return "LD IXl,IXl";
        case 0x6E: return "LD L,(IX+%02X)";
        case 0x6F: return "LD IXl,A";

        case 0x70: return "LD (IX+%02X),B";
        case 0x71: return "LD (IX+%02X),C";
        case 0x72: return "LD (IX+%02X),D";
        case 0x73: return "LD (IX+%02X),E";
        case 0x74: return "LD (IX+%02X),H";
        case 0x75: return "LD (IX+%02X),L";

        case 0x77: return "LD (IX+%02X),A";

        case 0x7C: return "LD A,IXh";
        case 0x7D: return "LD A,IXl";
        case 0x7E: return "LD A,(IX+%02X)";

        case 0x84: return "ADD A,IXh";
        case 0x85: return "ADD A,IXl";
        case 0x86: return "ADD A,(IX+%02X)";

        case 0x8C: return "ADC A,IXh";
        case 0x8D: return "ADC A,IXl";
        case 0x8E: return "ADC A,(IX+%02X)";

        case 0x94: return "SUB A,IXh";
        case 0x95: return "SUB A,IXl";
        case 0x96: return "SUB A,(IX+%02X)";

        case 0x9C: return "SBC A,IXh";
        case 0x9D: return "SBC A,IXl";
        case 0x9E: return "SBC A,(IX+%02X)";

        case 0xA4: return "AND A,IXh";
        case 0xA5: return "AND A,IXl";
        case 0xA6: return "AND A,(IX+%02X)";

        case 0xAC: return "XOR A,IXh";
        case 0xAD: return "XOR A,IXl";
        case 0xAE: return "XOR A,(IX+%02X)";

        case 0xB4: return "OR A,IXh";
        case 0xB5: return "OR A,IXl";
        case 0xB6: return "OR A,(IX+%02X)";

        case 0xBC: return "CP A,IXh";
        case 0xBD: return "CP A,IXl";
        case 0xBE: return "CP A,(IX+%02X)";

        case 0xCB: return "IX bits:";

        case 0xE1: return "POP IX";

        case 0xE3: return "EX (SP),IX";

        case 0xE5: return "PUSH IX";

        case 0xE9: return "JP (IX)";

        case 0xF9: return "LD SP,IX";
    }
    return "UNKNOW";
}
//
const char* opcode_FD(uint8_t b) {
    switch(b) {
        case 0x09: return "ADD IY,BC";

        case 0x19: return "ADD IY,DE";

        case 0x21: return "LD IY,%02X%02X";
        case 0x22: return "LD (%02X%02X),IY";
        case 0x23: return "INC IY";
        case 0x24: return "INC IYh";
        case 0x25: return "DEC IYh";
        case 0x26: return "LD IYh,%02X";

        case 0x29: return "ADD IY,IY";
        case 0x2A: return "LD IY,(%02X%02X)";
        case 0x2B: return "DEC IY";
        case 0x2C: return "INC IYl";
        case 0x2D: return "DEC IYl";
        case 0x2E: return "LD IYl,%02X";

        case 0x34: return "INC (IY+%02X)";
        case 0x35: return "DEC (IY+%02X)";
        case 0x36: return "LD (IY+%02X),%02X";

        case 0x39: return "ADD IY,SP";

        case 0x44: return "LD B,IYh";
        case 0x45: return "LD B,IYl";
        case 0x46: return "LD B,(IY+%02X)";

        case 0x4C: return "LD C,IYh";
        case 0x4D: return "LD C,IYl";
        case 0x4E: return "LD C,(IY+%02X)";

        case 0x54: return "LD D,IYh";
        case 0x55: return "LD D,IYl";
        case 0x56: return "LD D,(IY+%02X)";

        case 0x5C: return "LD E,IYh";
        case 0x5D: return "LD E,IYl";
        case 0x5E: return "LD E,(IY+%02X)";

        case 0x60: return "LD IYh,B";
        case 0x61: return "LD IYh,C";
        case 0x62: return "LD IYh,D";
        case 0x63: return "LD IYh,E";
        case 0x64: return "LD IYh,IYh";
        case 0x65: return "LD IYh,IYl";
        case 0x66: return "LD H,(IY+%02X)";
        case 0x67: return "LD IYh,A";
        case 0x68: return "LD IYl,B";
        case 0x69: return "LD IYl,C";
        case 0x6A: return "LD IYl,D";
        case 0x6B: return "LD IYl,E";
        case 0x6C: return "LD IYl,IYh";
        case 0x6D: return "LD IYl,IYl";
        case 0x6E: return "LD L,(IY+%02X)";
        case 0x6F: return "LD IYl,A";

        case 0x70: return "LD (IY+%02X),B";
        case 0x71: return "LD (IY+%02X),C";
        case 0x72: return "LD (IY+%02X),D";
        case 0x73: return "LD (IY+%02X),E";
        case 0x74: return "LD (IY+%02X),H";
        case 0x75: return "LD (IY+%02X),L";

        case 0x77: return "LD (IY+%02X),A";

        case 0x7C: return "LD A,IYh";
        case 0x7D: return "LD A,IYl";
        case 0x7E: return "LD A,(IY+%02X)";

        case 0x84: return "ADD IYh";
        case 0x85: return "ADD IYl";
        case 0x86: return "ADD (IY+%02X)";

        case 0x8C: return "ADC IYh";
        case 0x8D: return "ADC IYl";
        case 0x8E: return "ADC (IY+%02X)";

        case 0x94: return "SUB IYh";
        case 0x95: return "SUB IYl";
        case 0x96: return "SUB (IY+%02X)";

        case 0x9C: return "SBC IYh";
        case 0x9D: return "SBC IYl";
        case 0x9E: return "SBC (IY+%02X)";

        case 0xA4: return "AND IYh";
        case 0xA5: return "AND IYl";
        case 0xA6: return "AND (IY+%02X)";

        case 0xAC: return "XOR IYh";
        case 0xAD: return "XOR IYl";
        case 0xAE: return "XOR (IY+%02X)";

        case 0xB4: return "OR IYh";
        case 0xB5: return "OR IYl";
        case 0xB6: return "OR (IY+%02X)";

        case 0xBC: return "CP IYh";
        case 0xBD: return "CP IYl";
        case 0xBE: return "CP (IY+%02X)";

        case 0xCB: return "IY bits:";

        case 0xE1: return "POP IY";

        case 0xE3: return "EX (SP),IY";

        case 0xE5: return "PUSH IY";

        case 0xE9: return "JP (IY)";

        case 0xF9: return "LD SP,IY";
    }
    return "UNKNOW";
}
//
uint8_t opcode_z80()
{
	uint8_t  len_opcode = 1;
	uint8_t b0 = read_z80(z1,address_pc);
	uint8_t b1 = read_z80(z1,address_pc+1);
	uint8_t b2 = read_z80(z1,address_pc+2);
	uint8_t b3 = read_z80(z1,address_pc+3);
	uint8_t b4 = read_z80(z1,address_pc+4);
   // uint8_t b4 = RdZ80(address_pc+4);
	len_opcode =OpcodeLen(address_pc);
    uint8_t x_byte =0;
	uint16_t adr_x =0;
     	switch (b0)
	{
	case 0x10:
    case 0x18:
    case 0x20:
	case 0x28:
    case 0x30:
	case 0x38:
    x_byte = read_z80(z1,address_pc+1);
	if (x_byte>127) adr_x=address_pc-(254-x_byte);
	else adr_x=address_pc+x_byte+2;
	snprintf(tmp_opcode, sizeof tmp_opcode, opcode_tab[b0],adr_x) ;
	return len_opcode;

	case 0xCB:
    snprintf(tmp_opcode, sizeof tmp_opcode, opcode_CB[b1]) ;
	return len_opcode;

	case 0xED:
	//opcode_ED
    snprintf(tmp_opcode, sizeof tmp_opcode, opcode_ED(b1),b3,b2) ;
	return len_opcode;

	case 0xFD:
	//opcode_FD
    if (b1==0xCB) 
	{
      snprintf(tmp_opcode,sizeof tmp_opcode,opcode_CB_I[b3],"IY+",b2);
	return len_opcode;	
	}
    
	if (len_opcode==3)
    snprintf(tmp_opcode, sizeof tmp_opcode, opcode_FD(b1),b2) ;
	else
	snprintf(tmp_opcode, sizeof tmp_opcode, opcode_FD(b1),b3,b2) ;   
	return len_opcode;
	//break;

	case 0xDD:
	//opcode_FD
    if (b1==0xCB) 
	{
      snprintf(tmp_opcode,sizeof tmp_opcode,opcode_CB_I[b3],"IX+",b2);
	return len_opcode;	
	}	
	if (len_opcode==3)
    snprintf(tmp_opcode, sizeof tmp_opcode, opcode_DD(b1),b2) ;
	else
	snprintf(tmp_opcode, sizeof tmp_opcode, opcode_FD(b1),b3,b2) ;   
	return len_opcode;
	}



	switch (len_opcode)
	{
	case 1:
		//opcode_tab[b];
		snprintf(tmp_opcode, sizeof tmp_opcode, opcode_tab[b0]);
		break;
	case 2:
		//opcode_tab[b];
		snprintf(tmp_opcode, sizeof tmp_opcode, opcode_tab[b0],b1 );
		break;
	case 3:
		//opcode_tab[b];
		
	//	snprintf(tmp_opcode, sizeof tmp_opcode, opcode_tab[b]);
		snprintf(tmp_opcode, sizeof tmp_opcode, opcode_tab[b0],b2,b1 );
      //  snprintf(tmp_data, sizeof tmp_data, "%04X",b1);
      //  strcat(tmp_opcode,tmp_data);

		break;			
	default:
	    snprintf(tmp_opcode, sizeof tmp_opcode, "          ");
		break;
	}
    //snprintf(tmp_opcode, sizeof tmp_opcode, "%02X ", b);

    return len_opcode;
}
//---------------------------------------------------------
// calculate the length of an opcode
int OpcodeLen( uint16_t p ) {
    int len = 1;

    switch ( read_z80(z1,p) ) { // Opcode
    case 0x06:                // LD B,n
    case 0x0E:                // LD C,n
    case 0x10:                // DJNZ e
    case 0x16:                // LD D,n
    case 0x18:                // JR e
    case 0x1E:                // LD E,n
    case 0x20:                // JR NZ,e
    case 0x26:                // LD H,n
    case 0x28:                // JR Z,e
    case 0x2E:                // LD L,n
    case 0x30:                // JR NC,e
    case 0x36:                // LD (HL),n
    case 0x38:                // JR C,e
    case 0x3E:                // LD A,n
    case 0xC6:                // ADD A,n
    case 0xCE:                // ADC A,n
    case 0xD3:                // OUT (n),A
    case 0xD6:                // SUB n
    case 0xDB:                // IN A,(n)
    case 0xDE:                // SBC A,n
    case 0xE6:                // AND n
    case 0xEE:                // XOR n
    case 0xF6:                // OR n
    case 0xFE:                // CP n

    case 0xCB: // shift-,rotate-,bit-opcodes
        len = 2;
        break;
    case 0x01: // LD BC,nn'
    case 0x11: // LD DE,nn'
    case 0x21: // LD HL,nn'
    case 0x22: // LD (nn'),HL
    case 0x2A: // LD HL,(nn')
    case 0x31: // LD SP,(nn')
    case 0x32: // LD (nn'),A
    case 0x3A: // LD A,(nn')
    case 0xC2: // JP NZ,nn'
    case 0xC3: // JP nn'
    case 0xC4: // CALL NZ,nn'
    case 0xCA: // JP Z,nn'
    case 0xCC: // CALL Z,nn'
    case 0xCD: // CALL nn'
    case 0xD2: // JP NC,nn'
    case 0xD4: // CALL NC,nn'
    case 0xDA: // JP C,nn'
    case 0xDC: // CALL C,nn'
    case 0xE2: // JP PO,nn'
    case 0xE4: // CALL PO,nn'
    case 0xEA: // JP PE,nn'
    case 0xEC: // CALL PE,nn'
    case 0xF2: // JP P,nn'
    case 0xF4: // CALL P,nn'
    case 0xFA: // JP M,nn'
    case 0xFC: // CALL M,nn'
        len = 3;
        break;
    case 0xDD:
        len = 2;
        switch ( read_z80(z1,p+1)) { // 2nd part of the opcode
        case 0x34:                    // INC (IX+d)
        case 0x35:                    // DEC (IX+d)
        case 0x46:                    // LD B,(IX+d)
        case 0x4E:                    // LD C,(IX+d)
        case 0x56:                    // LD D,(IX+d)
        case 0x5E:                    // LD E,(IX+d)
        case 0x66:                    // LD H,(IX+d)
        case 0x6E:                    // LD L,(IX+d)
        case 0x70:                    // LD (IX+d),B
        case 0x71:                    // LD (IX+d),C
        case 0x72:                    // LD (IX+d),D
        case 0x73:                    // LD (IX+d),E
        case 0x74:                    // LD (IX+d),H
        case 0x75:                    // LD (IX+d),L
        case 0x77:                    // LD (IX+d),A
        case 0x7E:                    // LD A,(IX+d)
        case 0x86:                    // ADD A,(IX+d)
        case 0x8E:                    // ADC A,(IX+d)
        case 0x96:                    // SUB A,(IX+d)
        case 0x9E:                    // SBC A,(IX+d)
        case 0xA6:                    // AND (IX+d)
        case 0xAE:                    // XOR (IX+d)
        case 0xB6:                    // OR (IX+d)
        case 0xBE:                    // CP (IX+d)
            len = 3;
            break;
        case 0x21: // LD IX,nn'
        case 0x22: // LD (nn'),IX
        case 0x2A: // LD IX,(nn')
        case 0x36: // LD (IX+d),n
        case 0xCB: // Rotation (IX+d)
            len = 4;
            break;
        }
        break;
    case 0xED:
        len = 2;
        switch ( read_z80(z1,p+1) ) { // 2nd part of the opcode
        case 0x43:                    // LD (nn'),BC
        case 0x4B:                    // LD BC,(nn')
        case 0x53:                    // LD (nn'),DE
        case 0x5B:                    // LD DE,(nn')
        case 0x73:                    // LD (nn'),SP
        case 0x7B:                    // LD SP,(nn')
            len = 4;
            break;
        }
        break;
    case 0xFD:
        len = 2;
        switch ( read_z80(z1,p+1) ) { // 2nd part of the opcode
        case 0x34:                    // INC (IY+d)
        case 0x35:                    // DEC (IY+d)
        case 0x46:                    // LD B,(IY+d)
        case 0x4E:                    // LD C,(IY+d)
        case 0x56:                    // LD D,(IY+d)
        case 0x5E:                    // LD E,(IY+d)
        case 0x66:                    // LD H,(IY+d)
        case 0x6E:                    // LD L,(IY+d)
        case 0x70:                    // LD (IY+d),B
        case 0x71:                    // LD (IY+d),C
        case 0x72:                    // LD (IY+d),D
        case 0x73:                    // LD (IY+d),E
        case 0x74:                    // LD (IY+d),H
        case 0x75:                    // LD (IY+d),L
        case 0x77:                    // LD (IY+d),A
        case 0x7E:                    // LD A,(IY+d)
        case 0x86:                    // ADD A,(IY+d)
        case 0x8E:                    // ADC A,(IY+d)
        case 0x96:                    // SUB A,(IY+d)
        case 0x9E:                    // SBC A,(IY+d)
        case 0xA6:                    // AND (IY+d)
        case 0xAE:                    // XOR (IY+d)
        case 0xB6:                    // OR (IY+d)
        case 0xBE:                    // CP (IY+d)
            len = 3;
            break;
        case 0x21: // LD IY,nn'
        case 0x22: // LD (nn'),IY
        case 0x2A: // LD IY,(nn')
        case 0x36: // LD (IY+d),n
        case 0xCB: // Rotation,Bitop (IY+d)
            len = 4;
            break;
        }
        break;
    }
    return len;
}
//=========================================================
// turbo/normal
void turbo_switch(void)
{
    if (conf.turbo > 1)
        conf.turbo = 0;

     switch (conf.turbo)
     {
     case 0:
        ticks_per_cycle = CPU_KHZ / 3500; // 108
       // ticks_per_frame = 71680;          // 71680- Пентагон //70908 - 128 +2A
      //  ticks_per_frame_0 = ticks_per_frame;
        break;
     case 1:
        ticks_per_cycle = 1;//CPU_KHZ /5250;//CPU_KHZ / (3500*3);// 5250; // 
       // ticks_per_cycle = CPU_KHZ / 7000; // 108
     //   ticks_per_frame = (71680);        // 71680- Пентагон //70908 - 128 +2A 1.5
     //   ticks_per_frame_0 = ticks_per_frame;
        break;   
     // case 2:
     //   ticks_per_cycle = 1;//CPU_KHZ /28000; // 54
    //    ticks_per_frame = 71680;    //=107520//120000;//71680*2 ;// 71680- Пентагон //70908 - 128 +2A
  
        break;          

     default:
        break;
     }

}
//===============================================================================

//===========================================================================



#include "string.h"
#include "stdbool.h"

#include "zx_machine.h"
#include "../wd1793.h"

#include "../screen_util.h"
#include "util.h"
#include "../aySoft.h"
#include "../util_tap.h"
#include "disassembler.h"


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

#include "rom/trdos604q_nova.h" // rom navigator
#include "rom/rom48Q_nova.h"// 48 kb   quorum
#include "rom/rom128Q_nova.h"// 128 kb quorum



//#include "rom/service.h"// сервис монитор пентагон 

#include "rom/romScorpion295.h"// Scorpion ZS256
//#include "rom/romScorpion.h"// Scorpion ZS256 Old

#include "quorum.h" 

//###
#include <stddef.h>
#ifndef Z_NULL
#define Z_NULL NULL
#endif
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

unsigned long prev_ticks, cur_ticks;


//bool z80_gen_nmi_from_main = false;
bool im_z80_stop = false;
bool im_ready_loading = false;
uint64_t tape_cycle_count = 0; // глобальный счётчик тактов Z80 для ленты
//bool vbuf_en=true;

////////////////////////////////
#ifdef RP2350_256K
uint8_t *zx_ram_bank[16]; // Хранит адреса 16 и банков памяти
#else
uint8_t *zx_ram_bank[8]; // Хранит адреса 8ми банков памяти
#endif
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
//######################################################################
bool is_SD_active=false;
uint8_t z_controler_cs;
//--------------------------------------------------
static inline uint8_t READ_SD_BYTE()
{
    uint8_t dataSPI=spi_get_hw(SDCARD_SPI_BUS)->dr;
    spi_get_hw(SDCARD_SPI_BUS)->dr=0xff;  
    return   dataSPI;
}

static inline void  WRITE_SD_BYTE(uint8_t data) 
{
    volatile uint8_t dataSPI=spi_get_hw(SDCARD_SPI_BUS)->dr;
    spi_get_hw(SDCARD_SPI_BUS)->dr=data; 

}
//============================================================
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

case QUORUM1024:
    rom_select_Quorum1024();
	return;
	break;


    
case SCORP256:

if ((zx_1ffd_lastOut & 0x02) == 0x02)
{
	rom=3;  zx_cpu_ram[0] = zx_rom_bank[3]; 
}
	else 
	{
		rom=(zx_7ffd_lastOut & 0x10)>>4;
	    zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
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
// чтение из памяти 128 стандарт для дизассемблера
zuint8 fast(read_memory)(Z80 *cpu, zuint16 addr)
{
	return zx_cpu_ram[(zuint8)(addr >> 14)][addr & 0x3fff];
}
// чтение из памяти 128 стандарт
inline static zuint8 fast(read_z80)(Z80 *cpu, zuint16 addr)
{
	return zx_cpu_ram[(zuint8)(addr >> 14)][addr & 0x3fff];
}

//---------------------------------------------------------------------------------------------------------------
// запись в память 128 стандарт
//inline static 
inline static void fast(write_z80)(Z80 *cpu, uint16_t addr, uint8_t val)
{
	uint8_t x = (addr >> 14);
    if(x == 0) return; // ПЗУ
	zx_cpu_ram[x][addr & 0x3fff] = val;												 
}
//######################################################################################
#ifdef RP2350_256K  // для памяти RAM 256kB Scorpion ZS256 на RP2350 не используя PSRAM
// чтение из памяти 256k
inline static zuint8 fast(read_z80_256_s)(Z80 *cpu, zuint16 addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    zuint8 x = (addr >> 14);
     if(x == 0) // Обработка первого сегмента (0x0000-0x3fff)
        {
         return (zx_1ffd_lastOut & 0x01)           // Быстрая проверка флага
                ? zx_ram_bank[0][masked_addr]      // RAM
                : zx_cpu_ram[0][masked_addr];      // ROM
	    }
// Общий случай для x=1,2 и x=3 с обычной RAM	       
        return zx_cpu_ram[x][masked_addr];
  }
//---------------------------------------------------------------------------------------
// запись в память 256k
inline static void fast(write_z80_256_s)(Z80 *cpu, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    if(x == 0) // Обработка первого сегмента (0x0000-0x3fff)
        {
             if(zx_1ffd_lastOut & 0x01) zx_ram_bank[0][masked_addr] = val;
            return;  // Выход для ROM/RAM банка 0
        }
    // Общий случай для x=1,2 и x=3 с обычной RAM	
     zx_cpu_ram[x][masked_addr] = val;	
}
//########################################################################## 
// для памяти RAM 256kB Navigator 256 на RP2350 не используя PSRAM
// чтение из памяти 256k
inline static zuint8 fast(read_z80_256_n)(Z80 *cpu, zuint16 addr)
{
	return zx_cpu_ram[(zuint8)(addr >> 14)][addr & 0x3fff];
}
//---------------------------------------------------------------------------------------
// запись в память 256k
inline static void fast(write_z80_256_n)(Z80 *cpu, uint16_t addr, uint8_t val)
{
	uint8_t x = (addr >> 14);
    if(x == 0) return; // ПЗУ
	zx_cpu_ram[x][addr & 0x3fff] = val;	
}
#endif  // 
//######################################################################################
//PSRAM_BOARD // для расширенной памяти на rp2040 и rp2350 с PSRAM на плате MURM1
#ifdef MURM1
// чтение из памяти
inline static uint8_t fast(read_z80_ext)(Z80 *cpu, uint16_t addr)
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
inline static void fast(write_z80_ext)(Z80 *cpu, uint16_t addr, uint8_t val)
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
inline static uint8_t fast(read_z80_ext8)(Z80 *cpu, uint16_t addr)
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
inline static void fast(write_z80_ext8)(Z80 *cpu, uint16_t addr, uint8_t val)
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
inline static uint8_t fast(read_z80_cash)(Z80 *cpu, uint16_t addr)
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
inline static void fast(write_z80_cash)(Z80 *cpu, uint16_t addr, uint8_t val)
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
inline static uint8_t fast(_read_z80_ext)(Z80 *cpu, uint16_t addr)
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
inline static void fast(_write_z80_ext)(Z80 *cpu, uint16_t addr, uint8_t val)
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
inline static uint8_t fast(_read_z80_ext8)(Z80 *cpu, uint16_t addr)
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
inline static void fast(_write_z80_ext8)(Z80 *cpu, uint16_t addr, uint8_t val)
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
inline static uint8_t fast(_read_z80_cash)(Z80 *cpu, uint16_t addr)
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
inline static void fast(_write_z80_cash)(Z80 *cpu, uint16_t addr, uint8_t val)
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

static uint64_t loadLastCycle = 0;
static uint16_t loadFastCount = 0; // счётчик быстрых вызовов подряд

bool fast (hw_zx_get_bit_LOAD)()
{
    bool x;
    if (tap_loader_active && TapeStatus==TAPE_LOADING){
        x = TAP_Read();
        uint64_t gap = tape_cycle_count - loadLastCycle;
        loadLastCycle = tape_cycle_count;
        if (gap < 3000)
            loadFastCount++;
        else
            loadFastCount = 0;
        // Звук только если мы в устойчивом tight loop (>30 быстрых IN подряд)
        valLoad = (loadFastCount > 30) ? conf.vol_load * x : 0;
    } else {
        x = gpio_get(PIN_ZX_LOAD);
        valLoad=0;
        loadLastCycle = 0;
        loadFastCount = 0;
    }
    return x;
};
//###############################################
// IN
//###############################################

inline static uint8_t fast(in_z80)(Z80 *cpu, uint16_t port16) {
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
    #else
    if (portL == 0x57) return READ_SD_BYTE();
    if (portL == 0x77) 
    {
        if (is_SD_active)   return 0xfc;
                            return 0xff;
    }
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
inline static uint8_t fast(in_z80_cash)(Z80 *cpu,uint16_t port16) {
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
    #else
    if (portL == 0x57) return READ_SD_BYTE();
    if (portL == 0x77) 
    {
        if (is_SD_active)   return 0xfc;
                            return 0xff;
    }
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
inline static uint8_t fast(in_z80_p8)(Z80 *cpu, uint16_t port16) {
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
    #else
    if (portL == 0x57) return READ_SD_BYTE();
    if (portL == 0x77) 
    {
        if (is_SD_active)   return 0xfc;
                            return 0xff;
    }
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
#ifdef RP2350_256K
	case NOVA256/* nova 256 */:
        zx_RAM_bank_active  = (((val&0b10000000)>>4)|(val&0b00000111)); // d0 d1 d2  и  d7 7ffd
        if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //        76543210  5 bit
        zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
       rom_select(); // переключение ПЗУ по портам и по сигналу DOS
	return; // выход нафиг	
#else
	case NOVA256/* nova 256 */:
	      zx_RAM_bank_active  = (((val&0b10000000))|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
          if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //        76543210  5 bit
        zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
       rom_select(); // переключение ПЗУ по портам и по сигналу DOS
	return; // выход нафиг
#endif    	 

	return; // выход нафиг	
	case QUORUM1024:
        pager7ffd_Quorum1024(val);
    	return; // выход нафиг	



//--------------------------------------------------------------------------------
#ifdef RP2350_256K
     case SCORP256 /* Scorpion 256 */:
	  
          if (val & 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
          
          zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd


            zx_RAM_bank_active = zx_RAM_bank_7ffd | zx_RAM_bank_1ffd   ;

	      zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	      if (val & 0x08) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

           rom_select(); // переключение ПЗУ по портам и по сигналу DOS

     return; // выход нафиг
#else
     case SCORP256 /* Scorpion 256 */:
	  
          if (val & 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
          
          zx_RAM_bank_7ffd = (val&0b00000111) ; // d0 d1 d2  7ffd


          zx_RAM_bank_active = zx_RAM_bank_7ffd |  zx_RAM_bank_1ffd ;


	      zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd];
	
	      if (val & 0x08) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

           rom_select(); // переключение ПЗУ по портам и по сигналу DOS

     return; // выход нафиг
#endif     
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
inline static void fast(spec48)(Z80 *cpu,uint16_t port16, uint8_t val)
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
inline static void fast(spec128)(Z80 *cpu, uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
inline static void fast(extram128)(Z80 *cpu,uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
inline static void fast(extram_1ffd)(Z80 *cpu,uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
 
            #ifdef RP2350_256K 
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active];
            #else
            zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_7ffd];
            #endif
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
inline static void fast(extram_gmx)(Z80 *cpu,uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
inline static void fast(extram_p8)(Z80 *cpu, uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
inline static void fast(nova_256)(Z80 *cpu, uint16_t port16, uint8_t val)
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
    #else    
        if (portL == 0x57) // передача данных в SD карту
        {
            if (is_SD_active)  WRITE_SD_BYTE(val);
             return;
            }

        if (portL == 0x77) 
        {
         is_SD_active=((val & 0x01)==1);
         gpio_put(SDCARD_PIN_CS,val & 0x02);
         z_controler_cs = val; 
         return;
        }
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
Z80 cpu_zx = {0};  // Один экземпляр Z80 для Spectrum

inline static void fast(halt_z80)(Z80 *cpu, zuint8 signal)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();


}
inline static zuint8 fast(inta_callback)(Z80 *cpu, zuint16 address)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();
 //int_enable = false;
 //z80_int(&cpu_zx, false);// INT OFF

        // Сброс линии INT после обработки
   //     if (int_enable && !(cpu_zx.request & Z80_REQUEST_INT)) {
    //        z80_int(&cpu_zx, Z_FALSE);
           // z1->int_pending = 0;
     //      int_enable = false;
    //    }




return 0xff;
}

inline static zuint8 fast(nop_callback)(Z80 *cpu, zuint16 address)
{
 // gpio_put(LED_BOARD, 1);
 // led_blink();
 //z80_int(&cpu_zx, false);// INT OFF
return 0x00;
}
//#define Z80_CHIP Z80_MODEL_ZILOG_NMOS
//#define Z80_CHIP Z80_MODEL_ZILOG_CMOS
//#define Z80_CHIP Z80_MODEL_NEC_NMOS
//#define Z80_CHIP Z80_MODEL_ST_CMOS
//########################

//
void select_cpu_z80(Z80 *cpu) {
    switch (conf.cpu_version) {
        case 0: cpu->options = Z80_MODEL_ZILOG_NMOS; return;
        case 1: cpu->options = Z80_MODEL_ZILOG_CMOS; return;   
        case 2: cpu->options = Z80_MODEL_NEC_NMOS; return;
        case 3: cpu->options = Z80_MODEL_ST_CMOS; return; 
        case 4: cpu->options = 0; return; 
        default: cpu->options = Z80_MODEL_ZILOG_NMOS; return;
    }
}
//###########################################
// инициализация Z80 
void zx_cpu_init(Z80 *cpu) {
  //  memset(cpu, 0, sizeof(Z80));
   
    select_cpu_z80(&cpu_zx);
    z80_power(cpu, Z_TRUE);
    z80_instant_reset(cpu);
}
// Pentagon 48
void machine_Spectrum_48(Z80 *cpu)
        {
        cpu->context      = cpu;
        cpu->fetch_opcode = (Z80Read )read_z80;
        cpu->fetch        = (Z80Read )read_z80;
        cpu->nop          = (Z80Read )read_z80;
        cpu->read         = (Z80Read )read_z80;
        cpu->write        = (Z80Write)write_z80;
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)spec48;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;

		pent_config = SPEC48;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// инициализация Z80 defain Pentagon 128
void machine_Pentagon_128(Z80 *cpu)
        {
        cpu->context      = cpu;
        cpu->fetch_opcode = (Z80Read )read_z80;
        cpu->fetch        = (Z80Read )read_z80;
        cpu->nop          = (Z80Read )read_z80;
        cpu->read         = (Z80Read )read_z80;
        cpu->write        = (Z80Write)write_z80;
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)spec128;//machine_cpu_out;
        cpu->halt         = Z_NULL;//= (Z80Halt)halt_z80;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;

		pent_config = PENT128;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        conf.shift_img=12585;
        }
// инициализация  Pentagon 512
void machine_Pentagon_512(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext;
        cpu->fetch        = (Z80Read )read_z80_ext;
        cpu->nop          = (Z80Read )read_z80_ext;
        cpu->read         = (Z80Read )read_z80_ext;
        cpu->write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext;
        cpu->fetch        = (Z80Read )_read_z80_ext;
        cpu->nop          = (Z80Read )_read_z80_ext;
        cpu->read         = (Z80Read )_read_z80_ext;
        cpu->write        = (Z80Write)_write_z80_ext;
        }
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)extram128;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;

		pent_config = PENT512;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// инициализация  Pentagon 512 CASH     
void nmi_Pentagon_512_cash(Z80 *cpu)
{
    zx_7ffd_lastOut = zx_7ffd_lastOut | 0x10;
	// zx_7ffd_lastOut = zx_7ffd_lastOut & 0xef;
    cash_f = 1;
}
//
void machine_Pentagon_512_cash(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_cash;
        cpu->fetch        = (Z80Read )read_z80_cash;
        cpu->nop          = (Z80Read )read_z80_cash;
        cpu->read         = (Z80Read )read_z80_cash;
        cpu->write        = (Z80Write)write_z80_cash;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_cash;
        cpu->fetch        = (Z80Read )_read_z80_cash;
        cpu->nop          = (Z80Read )_read_z80_cash;
        cpu->read         = (Z80Read )_read_z80_cash;
        cpu->write        = (Z80Write)_write_z80_cash;
        }
        cpu->in           = (Z80Read )in_z80_cash;//machine_cpu_in;
        cpu->out          = (Z80Write)extram128;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = (Z80Read )nmi_Pentagon_512_cash;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
        
		pent_config = PENT512;
		ticks_per_frame=71680 ;// 71680- Пентагон 
        }
// инициализация  Pentagon 1024
void machine_Pentagon_1024(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext;
        cpu->fetch        = (Z80Read )read_z80_ext;
        cpu->nop          = (Z80Read )read_z80_ext;
        cpu->read         = (Z80Read )read_z80_ext;
        cpu->write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext;
        cpu->fetch        = (Z80Read )_read_z80_ext;
        cpu->nop          = (Z80Read )_read_z80_ext;
        cpu->read         = (Z80Read )_read_z80_ext;
        cpu->write        = (Z80Write)_write_z80_ext;
        }
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)extram128;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
        
		pent_config = PENT1024;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// Scorpion ZS256    
void nmi_Scorpion_256(Z80 *cpu)
{
    rom=3;
    zx_cpu_ram[0] = zx_rom_bank[3];


}     

#ifdef RP2350_256K   //для памяти RAM 256kB на RP2350 не используя PSRAM
void machine_Scorpion_256(Z80 *cpu)
        {
        cpu->context      = cpu;
        
        cpu->fetch_opcode = (Z80Read )read_z80_256_s;
        cpu->fetch        = (Z80Read )read_z80_256_s;
        cpu->nop          = (Z80Read )read_z80_256_s;
        cpu->read         = (Z80Read )read_z80_256_s;
        cpu->write        = (Z80Write)write_z80_256_s;

        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)extram_1ffd;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = (Z80Read )nmi_Scorpion_256;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
       
        pent_config = SCORP256;
		ticks_per_frame=71680  ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        conf.shift_img=8939;
        }
#else
void machine_Scorpion_256(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext;
        cpu->fetch        = (Z80Read )read_z80_ext;
        cpu->nop          = (Z80Read )read_z80_ext;
        cpu->read         = (Z80Read )read_z80_ext;
        cpu->write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext;
        cpu->fetch        = (Z80Read )_read_z80_ext;
        cpu->nop          = (Z80Read )_read_z80_ext;
        cpu->read         = (Z80Read )_read_z80_ext;
        cpu->write        = (Z80Write)_write_z80_ext;
        }
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)extram_1ffd;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = (Z80Read )nmi_Scorpion_256;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
       
        pent_config = SCORP256;
		ticks_per_frame=71680  ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        conf.shift_img=8939;
        }
#endif


// Scorpion GMX 2048        
void machine_Scorpion_GMX(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext;
        cpu->fetch        = (Z80Read )read_z80_ext;
        cpu->nop          = (Z80Read )read_z80_ext;
        cpu->read         = (Z80Read )read_z80_ext;
        cpu->write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext;
        cpu->fetch        = (Z80Read )_read_z80_ext;
        cpu->nop          = (Z80Read )_read_z80_ext;
        cpu->read         = (Z80Read )_read_z80_ext;
        cpu->write        = (Z80Write)_write_z80_ext;
        }
        cpu->in           = (Z80Read )in_z80;//machine_cpu_in;
        cpu->out          = (Z80Write)extram_gmx;//machine_cpu_out;
        cpu->halt         = Z_NULL;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
        
		pent_config = PENT128;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
// NOVA 256 Кворум 256       
void nmi_NOVA_256(Z80 *cpu)
{
    rom=3;  
    zx_cpu_ram[0] = zx_rom_bank[3];
    zx_0000_lastOut = 0; 
}
//
#ifdef RP2350_256K  //для памяти RAM 256kB на RP2350 не используя PSRAM
void machine_NOVA_256(Z80 *cpu)
        {
        cpu->context      = cpu;

        cpu->fetch_opcode = (Z80Read )read_z80_256_n;
        cpu->fetch        = (Z80Read )read_z80_256_n;
        cpu->nop          = (Z80Read )read_z80_256_n;
        cpu->read         = (Z80Read )read_z80_256_n;
        cpu->write        = (Z80Write)write_z80_256_n;
        
        cpu->in           = (Z80Read )in_z80;
        cpu->out          = (Z80Write)nova_256;
        cpu->halt         = Z_NULL;
        cpu->nmia         = (Z80Read )nmi_NOVA_256;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
       
		pent_config = NOVA256;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
 #else
 void machine_NOVA_256(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext;
        cpu->fetch        = (Z80Read )read_z80_ext;
        cpu->nop          = (Z80Read )read_z80_ext;
        cpu->read         = (Z80Read )read_z80_ext;
        cpu->write        = (Z80Write)write_z80_ext;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext;
        cpu->fetch        = (Z80Read )_read_z80_ext;
        cpu->nop          = (Z80Read )_read_z80_ext;
        cpu->read         = (Z80Read )_read_z80_ext;
        cpu->write        = (Z80Write)_write_z80_ext;
        }
        cpu->in           = (Z80Read )in_z80;
        cpu->out          = (Z80Write)nova_256;
        cpu->halt         = Z_NULL;
        cpu->nmia         = (Z80Read )nmi_NOVA_256;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
       
		pent_config = NOVA256;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion
        }
#endif 
// MurmoZavr 8Mb      
void machine_MurmoZavr(Z80 *cpu)
        {
        cpu->context      = cpu;
        #ifdef MURM1
        if (psram_type)
        {
        cpu->fetch_opcode = (Z80Read )read_z80_ext8;
        cpu->fetch        = (Z80Read )read_z80_ext8;
        cpu->nop          = (Z80Read )nop_callback;
        cpu->read         = (Z80Read )read_z80_ext8;
        cpu->write        = (Z80Write)write_z80_ext8;
        }
        else
        #endif
        {
        cpu->fetch_opcode = (Z80Read )_read_z80_ext8;
        cpu->fetch        = (Z80Read )_read_z80_ext8;
        cpu->nop          = (Z80Read )nop_callback;
        cpu->read         = (Z80Read )_read_z80_ext8;
        cpu->write        = (Z80Write)_write_z80_ext8;
        }
        cpu->in           = (Z80Read )in_z80_p8;
        cpu->out          = (Z80Write)extram_p8;// aff7 + Pentagon 128
        cpu->halt         = Z_NULL;
        cpu->nmia         = Z_NULL;
        cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
        cpu->int_fetch    = Z_NULL;
        cpu->ld_i_a       = Z_NULL;
        cpu->ld_r_a       = Z_NULL;
        cpu->reti         = Z_NULL;
        cpu->retn         = Z_NULL;
        cpu->hook         = Z_NULL;
        cpu->illegal      = Z_NULL;
        
		pent_config =  PENT8M;
		ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion

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


case QUORUM1024:
        init_rom_ram_Q1024();
   	zx_RAM_bank_active =0x00;
	zx_RAM_bank_7ffd =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;
    zx_RAM_bank_ext8 =0x00;

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
        zx_rom_bank[0]=&ROM_128K[0*16384];//128k заглушка не используется
	    zx_rom_bank[1]=&ROM_48K_ORIGINAL[0*16384];//48k 
        if (conf.trdos_version==0) zx_rom_bank[2]=&ROM_TRDOS_504T[0*16384];//TRDOS 5.04T
        else zx_rom_bank[2]=&ROM_TRDOS_505D[0*16384];//TRDOS 5.05D
        zx_rom_bank[3]=&ROM_Qsm[0*16384];//SERVICE PENTAGON  /// TODO     заглушка не используется
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
		    rom=1;
		    zx_cpu_ram[0]=zx_rom_bank[1]; // 0x0000 - 0x3FFF 48 BASIC 0  с какой банки стартовать	
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
		 rom=1;
		 zx_cpu_ram[0]=zx_rom_bank[1]; //  48 BASIC
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

	
        zx_1ffd_lastOut=0x00; 

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
    #ifdef RP2350_256K
  	for(int i=0;i<16;i++)
	{
		zx_ram_bank[i]=&RAM[i*0x4000];
	} 
       memset(&RAM,0x00, 16*0x4000);	// стирание памяти 256kB
	#else
    for(int i=0;i<8;i++)
	{
		zx_ram_bank[i]=&RAM[i*0x4000];
	} 
       memset(&RAM,0x00, 8*0x4000);	// стирание памяти 128kB 
    #endif

	
     init_mashine_and_extram(conf.mashine);// <= это уже тут   machine_Pentagon_128(&cpu_zx);  // инициализация процессора
   
      z80_power(&cpu_zx, Z_TRUE);// Включаем питание машины
      z80_instant_reset(&cpu_zx);  // Включаем питание машины
      zx_machine_reset(0);// 0-первый запуск  1- запуск trd по SPACE  3-просто reset в BASIC128

  
};


void fast(zx_machine_input_set)(ZX_Input_t* input_data){memcpy(&zx_input,input_data,sizeof(ZX_Input_t));};

void zx_machine_reset(uint8_t rom_x)
{
	AY_reset();

    init_rom_ram(rom_x);

   //  machine_reset(&cpu_zx);// Используем  для сброса регистров
     

	zx_RAM_bank_active =0x00;
	zx_RAM_bank_7ffd =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;
    zx_RAM_bank_ext8 =0x00;
    zx_aff7_lastOut=0;
    zx_1ffd_lastOut=0x00;
    zx_0000_lastOut = 0x00;// QUORUM
    
    zx_cpu_ram[3]==zx_ram_bank[zx_RAM_bank_active];

 //   strcpy(conf.activefilename, conf.Disks[0]);// disk A   
    WD1793_Init();

   //memset(&RAM,0x00, 131072);	// стирание памяти 128kB
    cash_f = 0;// отключение кеш для Пентагон 512 CASH

    seekbuf =0;// обнуление счетчика tape при сбросе
    tap_loader_active = false;
    enable_tape = false;
    TapeStatus = TAPE_STOPPED;
    init_vol_ay(); 

  // Используем  для сброса регистров

        z80_instant_reset(&cpu_zx);

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
uint8_t* active_screen_buf=NULL;
//------------------------------------------
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
    ticks_per_cycle=cpu_pico_khz/3500;//
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
if (Z80_PC(cpu_zx) == 0x0556)  tape_load(); // вход в меню tape // CALL 1366 (0x0556)
////if (Z80_PC(cpu_zx) == 0x0562)  tape_load_0562(); // вход в меню tape // CALL 0x0562
if (Z80_PC(cpu_zx) == 0x056a)  tape_load_056a(); // вход в меню tape // CALL 0x056a
}
// NORMAL режим: запуск ленты когда ROM-загрузчик начинает читать
if (tap_loader_active && TapeStatus==TAPE_STOPPED)
{
if (Z80_PC(cpu_zx) == 0x0556 || Z80_PC(cpu_zx) == 0x056a) TAP_Play();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///!!!!!!!!!!!!  с этим не работает автозагрузка дисков при старте и по SPACE в Пентагонах и так далее !!!!!!!!!!!!!!!!!
		// tr-dos
/* 
		if ((zx_1ffd_lastOut & 0x02)== 0x00) // 0000 00x0  0x02 если не теневик Scorpion ZS 256
        {
	
		if (!trdos) // если еще не в trdos то вход
		{
			if ((Z80_PCH(cpu_zx) == 0x3D) && (rom == 1 ))// trdos работает с BASIC48 D4 = 1     rom =1 не выставляется при старте ??? TODO
			//if ((Z80_PCH(z1->cpu) == 0x3D) && (rom != 3 ))// trdos работает с BASIC48 D4 = 1     
			                                                                     
			{
			trdos = true;

           rom=2;
			zx_cpu_ram[0]=zx_rom_bank[2];// tr-dos
			
            }
			
		}
	  
     }
	  
		if (trdos) if ((Z80_PCH(cpu_zx) & 0xc0))// выход из trdos если в RAM
		{
		 trdos = false;
         rom_select(); // переключение ПЗУ по портам  
		} */
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
		// tr-dos

		if ((zx_1ffd_lastOut & 0x02)== 0x00) // 0000 00x0  0x02 если не теневик у Scorpion есть доступ к портам TR-DOS в ROM S.Monitor 
      {
	
		if (!trdos) // если еще не в trdos то вход
		{
			if (((Z80_PC(cpu_zx) & 0x3D00) == 0x3D00) && ((zx_7ffd_lastOut & 0x10) == 0x10) )// trdos работает с BASIC48 D4 = 1     
			                                                                     
			{
			trdos = true;

           rom=2;
			zx_cpu_ram[0]=zx_rom_bank[2];// tr-dos
			
            }
			
		}
	  
      }
	  
		if ((Z80_PC(cpu_zx) > 0x3FFF) && (trdos))// выход из trdos если в RAM
		{
		 trdos = false;
         rom_select(); // переключение ПЗУ по портам  
		}
		//=======================================================================================
			// Если нажали клавишу NMI // QUORUM // SCORPION
		//	if (main_nmi_key)
		//	{
                
         //   	 main_nmi_key = false;

//z80_run(&cpu_zx, 1);
         //   }
		//    if (conf.mashine == NOVA256)
	    	//	{
			//		rom=3;
			// НОВЫЙ ЭМУЛЯТОР	
           // 	zx_cpu_ram[0] = zx_rom_bank[3]; zx_0000_lastOut = 0; z80_nmi(&cpu_zx);
        //     } // QUORUM

        //   if (conf.mashine == SCORP256)
	    //		{
		//			rom=3;
				// НОВЫЙ ЭМУЛЯТОР	
         //       	 zx_cpu_ram[0] = zx_rom_bank[3];  z80_nmi(&cpu_zx);
           //      } // 


         //   if (conf.mashine == PENT_512CASH) // Пентагон 512 с кеш
	    //		{
					// zx_7ffd_lastOut = zx_7ffd_lastOut | 0x10;
					// zx_7ffd_lastOut = zx_7ffd_lastOut & 0xef;
				//	  cash_f = 1;
				// НОВЫЙ ЭМУЛЯТОР		
               //   z80_nmi(&cpu_zx);
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

// WD1793_Execute(); // так не работает демо UNREAL - не переходит на следующую часть, крутится на 1 части в цикле 
// нет особого смысла выполнять WD1793_Execute() если не активен trdos и нет обращения к портам TR-DOS 
//if (trdos) WD1793_Execute(); // так работает но не работает Кворум-1024 ))
if (conf.mashine==QUORUM1024) WD1793_Execute();
else if (trdos) WD1793_Execute();


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
   //  z80_int(&cpu_zx, true);// Генерация прерывания Z80
     //if (conf.turbo != 1) 
    // z80_int(&cpu_zx, true);// Генерация прерывания INT Z80; 
    #ifdef LEDBLINK
    led_blink();
    #endif
    } // вызов INT в обычном режиме

/*     if (conf.turbo == 1) 
    {
    if (int_enable) z80_int(&cpu_zx, true);// Генерация прерывания INT Z80 в TURBO режиме;
    } 
  //  

*/

      t0_time_ticks=(t0_time_ticks+d_dst_time_ticks)&0xffffff;  


  //   gpio_put(LED_BOARD, 1);
   //    if (int_enable) z80_int(&cpu_zx, true);// Генерация прерывания INT Z80
     if (/* (inx_tick_screen<32)&& */(int_enable))
    {
        z80_int(&cpu_zx, Z_TRUE);
    //    int_enable = false;
     //  dt_cpu = z80_run(&cpu_zx, 1);
      // z80_int(&cpu_zx, false);// INT OFF
      }
     //  else
     
        dt_cpu = z80_run(&cpu_zx, 1);
        tape_cycle_count += dt_cpu;

        // Сброс линии INT после обработки
        if (int_enable && !(cpu_zx.request & Z80_REQUEST_INT)) {
            z80_int(&cpu_zx, Z_FALSE);
            int_enable = false;
        }
 


    	d_dst_time_ticks=dt_cpu* ticks_per_cycle   ;// Расчетное количесто тактов реального процессора на выполненную команду Z80
	
		inx_tick_screen+=dt_cpu;//Увеличиваем на количество тактов Z80 на текущую выполненную команду.

 	 	// начало 
		 inx_tick_screen_ff=inx_tick_screen;

		 if (inx_tick_screen>=  ticks_per_frame)      // Если прошла 1/50 сек, 71680 тактов процессора Z80
			{
               
	        	//if (conf.turbo != 1) 
               // {
                int_enable=true; // включение INT NORMAL 50 Гц или FAST 100 Гц
           // z80_int(&cpu_zx, Z_TRUE);
              //  }
             
                //  z80_int(&cpu_zx, true);// Генерация прерывания INT Z80; 
              //  z80_run(&cpu_zx, 1);//28
              //  z80_int(&cpu_zx, false);// INT OFF

                 
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
		if (((y >= 240-20) && (y <= 240)))
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
    
 // zx_cpu_init(&cpu_zx);  // одна строка инициализации

    select_cpu_z80(&cpu_zx);

    setZxExtKeysDefault();

	switch (config_mashine)
	{
	case PENT128 :
        machine_Pentagon_128(&cpu_zx);
		break; //

	case SPEC48:
		machine_Spectrum_48(&cpu_zx);
		break; //

	case PENT512:
         machine_Pentagon_512(&cpu_zx);
		break; //

	case PENT1024:
    machine_Pentagon_1024(&cpu_zx);
		break; //
	case SCORP256:
      //  main_nmi_key = true;
        machine_Scorpion_256(&cpu_zx);   
		break; //
 
    // #ifndef NO_GMX     
	case GMX2048:
        machine_Scorpion_GMX(&cpu_zx);
		break; //
    //  #endif    

	case NOVA256:
      //    main_nmi_key = true;
          machine_NOVA_256(&cpu_zx);
		break; //

    case QUORUM1024:
        machine_Quorum1024(&cpu_zx);
        break;


	case PENT8M:
          machine_MurmoZavr(&cpu_zx);
		break; //

	case PENT_512CASH :// Пентагон 512 с кеш
        main_nmi_key = true;
        machine_Pentagon_512_cash(&cpu_zx);
		break; //

	default:
        machine_Pentagon_128(&cpu_zx);
		break;
	}

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
        ticks_per_cycle = cpu_pico_khz / 3500; // 108
       // ticks_per_frame = 71680;          // 71680- Пентагон //70908 - 128 +2A
      //  ticks_per_frame_0 = ticks_per_frame;
        break;
     case 1:
        ticks_per_cycle = 1;//cpu_pico_khz /5250;//cpu_pico_khz / (3500*3);// 5250; // 
       // ticks_per_cycle = cpu_pico_khz / 7000; // 108
     //   ticks_per_frame = (71680);        // 71680- Пентагон //70908 - 128 +2A 1.5
     //   ticks_per_frame_0 = ticks_per_frame;
        break;   
     // case 2:
     //   ticks_per_cycle = 1;//cpu_pico_khz /28000; // 54
    //    ticks_per_frame = 71680;    //=107520//120000;//71680*2 ;// 71680- Пентагон //70908 - 128 +2A
  
        break;          

     default:
        break;
     }

}
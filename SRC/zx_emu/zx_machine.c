#include "string.h"
#include "stdbool.h"

#include "zx_machine.h"
//#include "aySoundSoft.h"

#include "util.h"
// rom
#include "rom/rom48k.h"// 48 kb
#include "rom/rom128k.h"// 128 kb
#include "rom/trdos504tm.h"// rom trdos 504
//#include "rom/trdos503.h"// rom trdos 503
#include "rom/navigator_sm508.h"// сервис монитор навигатора

#include "rom/trdos604q.h" // rom navigator
#include "rom/rom48Q.h"// 48 kb   quorum
#include "rom/rom128Q.h"// 128 kb quorum


//#include "rom/rom_fatall.h"// сервис монитор пентагон 

#include "rom/romScorpion295.h"// Scorpion ZS256

//

#include "z80.h"
#include "../g_config.h"
#include "hardware/structs/systick.h"
#include "util_tap.h"


#include "I2C_rp2040.h"// это добавить
#include "usb_key.h"// это добавить

#include "drivers/psram/psram_spi.h"

#include "config.h"



//#define Z80_DEBUG


unsigned long prev_ticks, cur_ticks;


//bool z80_gen_nmi_from_main = false;
bool im_z80_stop = false;
bool im_ready_loading = false;
//bool vbuf_en=true;

////////////////////////////////
// tr-dos

bool trdos=0;
uint8_t VG_PortFF = 0x00;
////////////////////////////////

//uint8_t zx_machine_last_out_7ffd;
uint8_t zx_machine_last_out_fffd;



uint8_t zx_7ffd_lastOut=0;
uint8_t zx_1ffd_lastOut=0;
uint8_t zx_0000_lastOut=0x20;

uint8_t dos=1;

//uint8_t zx_RAM_bank_ext =0x00000000;
uint8_t zx_RAM_bank_1ffd =0x00000000;
uint8_t zx_RAM_bank_dffd =0x00000000;
u_int8_t pent_config=0;

////цвета спектрума в формате 6 бит
// uint8_t zx_color[]={
//         0b00000000,
//         0b00000010,
//         0b00100000,
//         0b00100010,
//         0b00001000,
//         0b00001010,
//         0b00101000,
//         0b00101010,
//         0b00000000,
//         0b00000011,
//         0b00110000,
//         0b00110011,
//         0b00001100,
//         0b00001111,
//         0b00111100,
//         0b00111111
//     };
//переменные состояния спектрума
volatile z80 cpu;
uint8_t RAM[16384*8]; //Реальная память куском 32Кб
ZX_Input_t zx_input;
bool zx_state_48k_MODE_BLOCK=false;
uint8_t zx_RAM_bank_active=0;/// 3????

	//uint8_t zx_Border_color=0x00;
	static uint32_t zx_colors_2_pix32[384];//предпосчитанные сочетания 2 цветов
	static uint8_t* zx_colors_2_pix=(uint8_t*)&zx_colors_2_pix32;//предпосчитанные сочетания 2 цветов


uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
uint8_t* zx_video_ram;//4 области памяти CPU

uint8_t* zx_ram_bank[8];//Хранит адреса 8ми банков памяти
uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))

typedef struct zx_vbuf_t
{
	uint8_t* data;
	bool is_displayed;
}zx_vbuf_t;

zx_vbuf_t zx_vbuf[ZX_NUM_GBUF];
zx_vbuf_t* zx_vbuf_active;


//выделение памяти может быть изменено в зависимости от платформы
//uint8_t RAM[16384*8]; //Реальная память куском 128Кб
//uint8_t RAM[16384*8]; //Реальная память куском 32Кб
//zx_cpu_ram_5 [16384];
//zx_cpu_ram_7 [16384];

//uint8_t VBUFS[ZX_SCREENW*ZX_SCREENH*ZX_NUM_GBUF*ZX_BPP/8];

//

uint8_t FAST_FUNC(zx_keyboardDecode)(uint8_t addrH)
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

// Scorpion 7ffd 1ffd
void FAST_FUNC(rom_select)()
{
//#define ROM128 0
//#define ROM48  1
//#define ROM_TRDOS 2
//#define ROM_SM  3


switch (conf.mashine)
{
	u_int8_t  dsg;
case NOVA256:
	//	rom_n =  ((zx_7ffd_lastOut & 0x10)>>4) | ((zx_0000_lastOut & 0x20)>>5) ;// 0001.0000 0000.1000 0000.0100 0000.0010 0000.0001
	//	zx_cpu_ram[0] =  zx_rom_bank[ table_nova256 [rom_n] ];

if ((zx_0000_lastOut&0b00100000) == 0) zx_cpu_ram[0] = zx_rom_bank[3]; 
	else  zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
	return;
	break;

case SCORP256:

       //  dsg = ~((zx_1ffd_lastOut & 0x08)>>3);// d3 0000 3000
       //   dos= dos & dsg;

        //   if (dos==0) trdos = true;
        //     else trdos = false;

if ((zx_1ffd_lastOut & 0x02) == 0x02) zx_cpu_ram[0] = zx_rom_bank[3]; 
	else  zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 

	return;
	break;


default:
         
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
// чтение из памяти
static uint8_t FAST_FUNC(read_z80)(void* userdata, uint16_t addr)
{
#if defined (PSRAM_ALL_PAGES )
	if (addr<0x4000) return zx_cpu_ram[0][addr];// если пзу
  	if (addr<0x8000) return zx_cpu_ram[1][addr-0x4000]; //NO psram ! 0x4000-0x7fff страница 5 zx_cpu_ram[1] = 5
	if (addr<0xc000) return zx_cpu_ram[2][addr-0x8000];//NO psram ! 0x8000-0xbfff
    if (zx_RAM_bank_active == 7) return zx_cpu_ram[3][addr-0xc000];
    if (zx_RAM_bank_active == 5) return zx_cpu_ram[3][addr-0xc000];
	if (zx_RAM_bank_active == 2) return zx_cpu_ram[3][addr-0xc000];
    return read8psram((zx_RAM_bank_active*0x4000)+(addr-0xc000)); // 0xc000-0xffff память больше 128
#endif // defined

#if defined(PSRAM_TOP_PAGES)

    uint8_t x = (addr >> 14);
	switch (x)
	{

	case 3:                                  // 0xc000-0xffff
	//	 if (zx_RAM_bank_active < 8)
		if (zx_RAM_bank_active & 0xf8) // 0b 1111 1000
		{
			return read8psram((uint32_t)(zx_RAM_bank_active << 14) + (addr & 0x3fff)); // 0xc000-0xffff память больше 128
		}
		break;
	}

	return zx_cpu_ram[x][addr & 0x3fff];

#endif // defined
}
//---------------------------------------------------------------------------------------------------------------
// запись в память
static void FAST_FUNC(write_z80)(void* userdata, uint16_t addr, uint8_t val)
{
#if defined (PSRAM_ALL_PAGES )
	if (addr<0x4000) return;//запрещаем писать в ПЗУ
	if (addr<0x8000) {zx_cpu_ram[1][addr-0x4000]=val;return;};// NO psram ! 0x4000-0x7fff
	if (addr<0xc000) {zx_cpu_ram[2][addr-0x8000]=val;return;};
	if (zx_RAM_bank_active == 2) {zx_cpu_ram[3][addr-0xc000] = val; return;}
    if (zx_RAM_bank_active == 5) {zx_cpu_ram[3][addr-0xc000] = val; return;}
	if (zx_RAM_bank_active == 7) {zx_cpu_ram[3][addr-0xc000] = val; return;}
    write8psram((zx_RAM_bank_active*0x4000)+(addr-0xc000),val);// 0xc000-0xffff память больше 128
#endif // defined

#if defined(PSRAM_TOP_PAGES)


	uint8_t x = (addr >> 14);
	switch (x)
	{
	case 0: return; // ПЗУ
  
	case 3:		// 0xc000-0xffff
		// if (zx_RAM_bank_active < 8)
		if (zx_RAM_bank_active & 0xf8) // 0b 1111 1000
		{
			write8psram((uint32_t)(zx_RAM_bank_active << 14) + (addr & 0x3fff), val); // 0xc000-0xffff память больше 128
		 // write8psram((zx_RAM_bank_active*0x4000)+(addr-0xc000),val);// 0xc000-0xffff память больше 128
			return;
		}
		break;
	}
	zx_cpu_ram[x][addr & 0x3fff] = val;

#endif // defined
}
//---------------------------------------------------------------------------------
//IN!
static uint8_t FAST_FUNC(in_z80)(z80* const z, uint8_t port) {

	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;

	if (trdos) // если это tr-dos
	{
		#if defined (TRDOS_0)
		uint8_t VG = 0xff;
		if ((portL & 0x1f) > 0)
		{
			uint8_t i = (portL >> 4) & 0x0E;
			if (i < 8)
			{
				ChipVG93(i, &VG);
			}
			else
			{
				if (i == 0x0E) // если  порт #FF
					ChipVG93(8, &VG);
				VG_PortFF = VG;
			}
			return VG;
		}
       #endif
      #if defined (TRDOS_1)
			        if (port == 0xFF){	
			return Requests;
        }

        

        if ((port & 0x7F) == port){ //((port == 0x7F) || (port == 0x5F) || (port == 0x3F) || (port == 0x1F))
		{
			
            return WD1793_Read((port>>5) & 0b11); // Read from 0x7F to 0x1F port
		}
        }
        return 0xFF; //???
       #endif

	} // tr-dos
	///////////////////////////////////////////////////////////////////
//if (portL==0xFD) return zx_7ffd_lastOut;
	if (port16&1)
	{
		uint16_t not_port16=~port16;

		//if ((port16&0x20)==0x0) {return zx_input.kempston;}//kempston{return 0xff;};//
/*
 Kempston Mouse i2c address 0x77
1 #FADF - поpт  кнопок
2 #FBDF - поpт X-кооpдинаты;
3 #FFDF - поpт У-кооpдинаты.
4 Kempston джойстик направления движения мыши
проверять до kempston
 */


		if ((port16 == 0xfadf) || (port16 == 0xfbdf) || (port16 == 0xffdf))

		{
			if (port16 == 0xfadf)
			{
				//   G_PRINTF("port #FADF: %x=%x\n",port16,ibuff[1]);
				return ibuff[1]; // #FADF
			}

			if (port16 == 0xfbdf)
			{
				//	G_PRINTF("port #FBDF: %x=%x\n",port16,ibuff[2]);
				return ibuff[2]; // #FBDF
			}
			if (port16 == 0xffdf)
			{
				//	G_PRINTF("port #FFDF: %x=%x\n",port16,ibuff[3]);
				return ibuff[3]; // #FFDF
			}
		}

		//  if ((port16&0x20)==0x0) {return ibuff[4];} !!! ЭТО МЕШАЕТ РАБОТЕ DENDY ДЖОЙСТИКУ
		// для правильной работы нужно скорее всего объединять данные из  ibuff[4] и  zx_input.kempston
		//if ((port16 & 0x20) == 0x0) {return zx_input.kempston;} // kempston{return 0xff;};//

		if ((port16 & 0x20) == 0x00)
		{
	//		if (joy_k == 0xff)

		//		return zx_input.kempston;
	//		else
 



         return (zx_input.kempston | joy_k);
		}
        //AY
		if ((port16 & 0xc002) == 0xc000) 	return AY_in_FFFD(); // fffd


	}
	else
	{
		//загрузка с магнитофона и опрос клавиатуры
			uint8_t out_data=zx_keyboardDecode(portH);
			return(out_data&0b10111111);
	}

	return 0xFF;
}
//-------------------------------------------------
// расширенная память и так далее
void FAST_FUNC (zx_machine_set_7ffd_out)(uint8_t val)// переключение банков памяти по 7ffd
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

        zx_7ffd_lastOut=val;

    switch (pent_config)
	{

	case PENT128 /* pentagon 128  и остальные кому нужно*/:
	  zx_RAM_bank_active = (val&0b00000111) ; // d0 d1 d2  7ffd
        if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
	
	   zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
   rom_select(); // переключение ПЗУ по портам и по сигналу DOS

	return; // выход нафиг
		break;


	case NOVA256/* nova 256 */:
	   zx_RAM_bank_active = (((val&0b10000000))|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
        if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //        76543210  5 bit
	  // zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	  // zx_RAM_bank_active = zx_RAM_bank_active;
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	   if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
     rom_select(); // переключение ПЗУ по портам и по сигналу DOS

	return; // выход нафиг	
		break;	

     case SCORP256 /* Scorpion 256 */:
	     zx_RAM_bank_active = (val&0b00000111) ; // d0 d1 d2  7ffd
          if (val & 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
          //  zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;

          zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd ;


	      zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	      if (val & 0x08) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

           rom_select(); // переключение ПЗУ по портам и по сигналу DOS

     return; // выход нафиг
		break;

///////////////////////////////////////////////////////////////////////////////

// дурацкие пентагоны 512 и 1024



	case PENT512/* pentagon 512 */:
	   zx_RAM_bank_active = (((val&0b11000000))|(val&0b00000111)); // d0 d1 d2  и d6 d7 7ffd
        if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
        //        76543210  5 bit

		break;	
	case PENT1024/* pentagon 1024 */:
        zx_RAM_bank_active = (((val&0b11100000)>>2)|(val&0b00000111)); // d0 d1 d2  и d5 d6 d7 7ffd
        // 5bit = 1 48k mode block отсутствует в pentagon1024
		break;	

	default:
			zx_RAM_bank_active = (val&0b00000111) ; // d0 d1 d2  7ffd
        if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
		break;
	}


    zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	
	if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];

              rom_select(); // переключение ПЗУ по портам 


///////////////////////////////////////////////////////////////////////////////
};
//------------------------------------------------------------------------------
uint8_t zx_machine_get_7ffd_lastOut(){return zx_7ffd_lastOut;}
//==============================================================================
// ZX Spectrum 128
static void FAST_FUNC(spec128)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
	
			uint8_t btemp = (portL >> 4);
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
          #endif
		 
	} // tr-dos

	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{
	//	if (port16==0x7FFD)
	//	if (portL==0xFD)
		if (((not_port16 & 0x8002) == 0x8002))//7ffd
		// 0111 1111  1101
		// A1=0   A15=0	
		{
        	//переключение банка памяти	
            zx_7ffd_lastOut=val;

        zx_RAM_bank_active = (val&0b00000111) ; // d0 d1 d2  7ffd
        if (val&0b00100000) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
       // zx_RAM_bank_active = zx_RAM_bank_active |  zx_RAM_bank_1ffd |  zx_RAM_bank_dffd;
	    zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active];
	    if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];  

	     rom_select(); // переключение ПЗУ по портам и по сигналу DOS
		return;
		}; 

	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY  

	}
	else
	{
		hw_zx_set_snd_out(val  & 0b00010000);					// D4 beep
		hw_zx_set_save_out(val & 0b00001000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end ZX Spectrum 128

// extram128
static void FAST_FUNC(extram128)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
          return;	
						   
		}
          #endif
		 
	} // tr-dos


	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{

		//if (port16==0x7FFD)
	//	if (portL==0xFD)
		if (((not_port16 & 0x8002) == 0x8002))//7ffd
		// 0111 1111  1101
		// A1=0   A15=0	
		{
            zx_machine_set_7ffd_out(val);  return;	//переключение банка памяти	
		}; 

    



	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}											// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY  

	}
	else
	{
		hw_zx_set_snd_out(val  & 0b00010000);					// D4 beep
		hw_zx_set_save_out(val & 0b00001000);					// 01000

		
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end extram128

// SCORPION ZS256
static void FAST_FUNC(extram_1ffd)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;






if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
         return;	
						   
		}
          #endif
		 
	} // tr-dos


	

	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{


		
		if (port16==0x7FFD)
	//	if (((not_port16 & 0x8002) == 0x8002))//7ffd
		// 0111 1111  1101
		// A1=0   A15=0	
		{
            zx_machine_set_7ffd_out(val);  return;	//переключение банка памяти	
		};


         if (port16 == 0x1ffd)
		// 0001 1111  1111 1101
		// A1=0  A15 = 0 A14 = 0 A13=0
		// if (((not_port16 & 0xe002) == 0xe002)&&((port16&0x1000)==0x1000))//   #1ffd
		//   if (((not_port16 & 0xe002) == 0xe002))//   #1ffd

		{
            zx_1ffd_lastOut=val;
			zx_RAM_bank_1ffd = ((val & 0x10) >> 1); // d4  1ffd scorpion 0000 x000

			//zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
                       zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd ;


			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active];

			rom_n = zx_7ffd_lastOut;
            rom_n1 = zx_1ffd_lastOut;	


           rom_select(); // переключение ПЗУ по портам 
			return;
		}




	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}													// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY





	}
	else
	{
		hw_zx_set_snd_out(val & 0x10);					// D4 beep
		hw_zx_set_save_out(val & 0x08);					// D3 save
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
	
}
// end extram_1ffd   // SCORPION ZS256
//
static void FAST_FUNC(extram_dffd)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
          return;	
						   
		}
          #endif
		 
	} // tr-dos

	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{

		if (port16 == 0xdffd)
		// 1101 1111  xxxx 1101
		// A1=0  A15 = 1 A14 = 1 A13=0
		//  if (((not_port16 & 0x2002) == 0x2002)&&((port16&0xc000)==0xc000))//   #dffd
		// if (((not_port16 & 0x2002) == 0x2002))//   #dffd

		{
			zx_RAM_bank_dffd = ((val & 0x07) << 4); // d0 d1 d2   dffd proffi 0xxx 0000
			zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active];

			return;
		}

			//if (port16 == 0x7FFD)
				 if (((not_port16 & 0x8002) == 0x8002)) // 7ffd
				// 0111 1111  1101
				// A1=0   A15=0
				{
					zx_machine_set_7ffd_out(val);
					return; // переключение банка памяти
				};

	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}													// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY



		}
		else
		{
			hw_zx_set_snd_out(val & 0x10);					// 10000
			hw_zx_set_save_out(val & 0x08);					// 01000
			zx_Border_color = ((val & 0x07) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
		}
	
}
// end extram_dffd
//
static void FAST_FUNC(extram_gmx)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
          return;	
						   
		}
          #endif
		 
	} // tr-dos


	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{
		if (port16 == 0x1ffd)
		// 0001 1111  1111 1101
		// A1=0  A15 = 0 A14 = 0 A13=0
		// if (((not_port16 & 0xe002) == 0xe002)&&((port16&0x1000)==0x1000))//   #1ffd
		//   if (((not_port16 & 0xe002) == 0xe002))//   #1ffd

		{
			zx_RAM_bank_1ffd = ((val & 0b000010000) >> 1); // d4  1ffd scorpion 0000 1000
			zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active];
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
			zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active]; // биты  0111 0000
			return;
		}

	//	if (port16 == 0x7FFD)
		 if (((not_port16 & 0x8002) == 0x8002)) // 7ffd 0000 0111
		//  0111 1111  1101
		//  A1=0   A15=0
		{
			zx_machine_set_7ffd_out(val);
			return; // переключение банка памяти
		};
	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}													// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY

	}

	else
	{
		hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_zx_set_save_out(val & 0b01000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end extram_gmx
//
static void FAST_FUNC(extram_4096)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
          return;	
						   
		}
          #endif
		 
	} // tr-dos

	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{

		if (port16 == 0xdffd)
		// 1101 1111  xxxx 1101
		// A1=0  A15 = 1 A14 = 1 A13=0
		//  if (((not_port16 & 0x2002) == 0x2002)&&((port16&0xc000)==0xc000))//   #dffd
		// if (((not_port16 & 0x2002) == 0x2002))//   #dffd

		{
			zx_RAM_bank_dffd = ((val & 0b00000111) << 3); // d0 d1 d2   dffd proffi 0011 1000
			zx_RAM_bank_active = zx_RAM_bank_active | zx_RAM_bank_1ffd | zx_RAM_bank_dffd;
			zx_cpu_ram[3] = zx_ram_bank[zx_RAM_bank_active];

			return;
		}
		if (port16 == 0x7FFD) // 0011 1000  dffd
		// if (((not_port16 & 0x8002) == 0x8002)) // 7ffd  1100 0111
		//  0111 1111  1101
		//  A1=0   A15=0
		{
			zx_machine_set_7ffd_out(val);
			return; // переключение банка памяти
		};

	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}													// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY
	}
	else
	{
		hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_zx_set_save_out(val & 0b01000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
}
// end extram_4096

//
static void FAST_FUNC(nova_256)(z80 *const z, uint8_t port, uint8_t val)
{
	uint8_t portH = z->_hi_addr_port;
	uint8_t portL = port;
	uint16_t port16 = (portH << 8) | portL;
	uint16_t not_port16 = ~port16;

// QUORUM
 if (portL == 0x00) {
//	if ((val&0b00100000) == 0) zx_cpu_ram[0] = zx_rom_bank[3]; 
//	else  
	zx_0000_lastOut = val;	// QUORUM
	rom_select(); // переключение ПЗУ по портам и по сигналу DOS
//	zx_cpu_ram[0]=zx_rom_bank[(zx_7ffd_lastOut & 0x10)>>4]; 
	return;
} 
// QUORUM

	if (trdos) // если это tr-dos
	{
           #if defined(TRDOS_0)
		if ((portL & 0x1f) > 0)
		{
			uint8_t VG = val;
			uint8_t btemp = (portL >> 4);

			if (btemp < 8)
				ChipVG93(btemp, &VG);
			else
			{
				if (btemp == 0x0F)
					ChipVG93(9, &VG);
				VG_PortFF = VG;
			}
			return;
		}
          #endif

          #if defined(TRDOS_1)

        if ((portL & 0x1f) > 0)
		{
			uint8_t btemp = (portL >> 4);
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
          return;	
						   
		}
          #endif
		 
	} // tr-dos


	if (port16 & 1) // Расширение памяти и экран Spectrum-128 //PSRAM_AVAILABLE = false;
	{

		if (port16 == 0x7FFD) // 0011 1000  dffd
		// if (((not_port16 & 0x8002) == 0x8002)) // 7ffd  1100 0111
		//  0111 1111  1101
		//  A1=0   A15=0
		{
			zx_machine_set_7ffd_out(val);
         //   if ((zx_0000_lastOut & 0x20) == 0) zx_cpu_ram[0] = zx_rom_bank[3]; // переключение на ПЗУ QUORUM 
         //  else   
		 //  zx_cpu_ram[0]=zx_rom_bank[(val & 0x10)>>4];

			return; // переключение банка памяти
		};

	// чип AY
	if (((not_port16 & 0x0002) == 0x0002) && ((port16 & 0xe000) == 0xe000)) // 0xFFFD
		{AY_out_FFFD(val); return;}													// OUT(#FFFD),val
	if (((not_port16 & 0x4002) == 0x4002) && ((port16 & 0x8000) == 0x8000)) // 0xBFFD
		{AY_out_BFFD(val); return;}
	// чип AY
	}
	else
	{
		hw_zx_set_snd_out(val & 0b10000);					// 10000
		hw_zx_set_save_out(val & 0b01000);					// 01000
		zx_Border_color = ((val & 0x7) << 4) | (val & 0x7); // дублируем для 4 битного видеобуфера
	}
	
}
// end nova_256

//====================================================================
//
void (*out_z80)(); // определение указателя на функцию

void init_extram(uint8_t conf) // инициализация кофигурации портов переключения памяти
{
	switch (conf)
	{
	case 0:
		cpu.port_out = spec128; // 128K
		pent_config = PENT128;
		break; //
	case 1:
		cpu.port_out = extram128; // Pentagon 512
		pent_config = PENT512;
		break; //

	case 2:
		cpu.port_out = extram128; // Pentagon 1024
		pent_config = PENT1024;
		break; //
	case 3:
		cpu.port_out  = extram_1ffd; // Scorpion ZS256
		pent_config = SCORP256;
		break; //
	case 4:
		cpu.port_out  = extram_dffd; // Profi 1024
		pent_config = PENT128;
		break; //
	case 5:
		cpu.port_out = extram_gmx; // GMX2048 Profi 1024+Scorpion ZS256
		pent_config = PENT128;
		break; //
	case 6:
		cpu.port_out  = extram_4096; // GMX2048 Profi 1024 + Pentagon 512
		pent_config = PENT512;
		break; //
	case 7:
		cpu.port_out  = nova_256; //  Nova 256
		pent_config = NOVA256;
		break; //
	default:
		//out_z80 = extram128;
		cpu.port_out = extram128;
		pent_config = PENT128;
		break;
	}

}
//-----------------------------------------------------------------------------
//==============================================================================
void init_rom_ram(uint8_t rom_x)
{
    
    zx_0000_lastOut = 0x00;// QUORUM
	zx_RAM_bank_1ffd = 0x00;
	zx_RAM_bank_dffd = 0x00;
	zx_7ffd_lastOut=0x10;

	if (conf.mashine == NOVA256)	
	{
		//zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	   // zx_rom_bank[1]=&ROM_48K[0*16384];//48k 
	    zx_rom_bank[0]=&ROM_128Q[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48Q[0*16384];//48k 
		zx_rom_bank[2]=&ROM_Qtr[0*16384];//TRDOS 6.04
	    zx_rom_bank[3]=&ROM_Qsm[0*16384];//NAVIGATOR
	    zx_cpu_ram[0]=zx_rom_bank[3]; // 0x0000 - 0x3FFF с какой банки стартовать
	}	
    else 
	{   
		zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48K[0*16384];//48k 
	//	zx_rom_bank[2]=&ROM_F[1*16384];//TRDOS 6.11Q
		zx_rom_bank[2]=&ROM_TR[0*16384];//TRDOS 5.04T

	 //  zx_rom_bank[3]=&ROM_P[0*16384];//SERVICE PENTAGON
zx_rom_bank[3]=&ROM_Qsm[0*16384];//SERVICE PENTAGON

//ROM_SCORPION

 if (conf.mashine == SCORP256)	
	{
	 
	  
	  //  zx_rom_bank[0]=&ROM_SCORPION[0x0000];//128k 
	  //  zx_rom_bank[1]=&ROM_SCORPION[0x4000];//48k 
	//	zx_rom_bank[2]=&ROM_SCORPION[0xc000];//TRDOS 5.03
	 //   zx_rom_bank[3]=&ROM_SCORPION[0x8000];//SM
	    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF с какой банки стартовать

		zx_rom_bank[0]=&ROM_128K[0*16384];//128k 
	    zx_rom_bank[1]=&ROM_48K[0*16384];//48k 
	//	zx_rom_bank[2]=&ROM_F[1*16384];//TRDOS 6.11Q
		zx_rom_bank[2]=&ROM_TR[0*16384];//TRDOS 5.04T






	}	




 // printf("conf.autorun =%x\n",conf.autorun );
		if (rom_x ==0) // первый запуск при включении или hard reset
        {
        switch (conf.autorun)
		{
		case 0     /* OFF */:
			zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
			break;
		case 1     /*TR-DOS */:
		
			if (conf.Disks[0][0] ==0 ) zx_cpu_ram[0]=zx_rom_bank[0]; // диска нет 128 BASIC 
			else 
			zx_cpu_ram[0]=zx_rom_bank[2]; // диск есть TR-DOS  
		    zx_7ffd_lastOut=0x10;
			break;
		case 2     /* QS SLOT 0*/:
			zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
		break;

		default:
		    zx_cpu_ram[0]=zx_rom_bank[0]; // 0x0000 - 0x3FFF 128 BASIC 0  с какой банки стартовать
			zx_7ffd_lastOut=0x00;
			break;
		}
		}
        if (rom_x ==1) // загрузка с вставленной дискетой по SPACE
		{
		 zx_cpu_ram[0]=zx_rom_bank[2]; // 0x0000 - 0x3FFF TR-DOS    запуск trd по   SPACE
		 zx_7ffd_lastOut=0x10;
		}

		if (rom_x ==3) // просто reset в  128 BASIC
		{
		 zx_cpu_ram[0]=zx_rom_bank[0]; //  128 BASIC
		 zx_7ffd_lastOut=0x00;
		}

	}

	zx_cpu_ram[1] = zx_ram_bank[5]; // 0x4000 - 0x7FFF
	zx_cpu_ram[2] = zx_ram_bank[2]; // 0x8000 - 0xBFFF
	zx_cpu_ram[3] = zx_ram_bank[0]; // 0xC000 - 0x7FFF
	zx_video_ram = zx_ram_bank[5];
	
	zx_RAM_bank_active =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;



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

	// инициализация процессора

	z80_init(&cpu);
	cpu.read_byte = read_z80;	// Присваиваем процедуру read_z80 структуре z80 (Процедура cpu.readbyte)
	cpu.write_byte = write_z80; // Аналогично
	cpu.port_in = in_z80;		// Аналогично
	//cpu.port_out = out_z80;		// Аналогично
    cpu.port_out = extram128;	// Аналогично
    init_extram(conf.mashine);
	G_PRINTF_INFO("zx machine initialized\n");

	//init_rom_ram(0);
    zx_machine_reset(0);// 0-первый запуск  1- запуск trd по SPACE  3-просто reset в BASIC128
};


void FAST_FUNC(zx_machine_input_set)(ZX_Input_t* input_data){memcpy(&zx_input,input_data,sizeof(ZX_Input_t));};

void zx_machine_reset(uint8_t rom_x)
{
t2=0;
	z80 *z = &cpu;

  init_rom_ram(rom_x);

	z->pc = 0;
	z->sp = 0xFFFF;
	z->ix = 0;
	z->iy = 0;
	z->mem_ptr = 0;

	// af and sp are set to 0xFFFF after reset,
	// and the other values are undefined (z80-documented)
	z->a = 0xFF;
	z->b = 0;
	z->c = 0;
	z->d = 0;
	z->e = 0;
	z->h = 0;
	z->l = 0;
   // z->f = 0;

	z->a_ = 0;
	z->b_ = 0;
	z->c_ = 0;
	z->d_ = 0;
	z->e_ = 0;
	z->h_ = 0;
	z->l_ = 0;
	z->f_ = 0;

	z->i = 0;
	z->r = 0;

	z->sf = 1;
	z->zf = 1;
	z->yf = 1;
	z->hf = 1;
	z->xf = 1;
	z->pf = 1;
	z->nf = 1;
	z->cf = 1;

	z->iff_delay = 0;
	z->interrupt_mode = 0;
	z->iff1 = 0;
	z->iff2 = 0;
	z->halted = 0;
	z->int_pending = 0;
	z->nmi_pending = 0;
	z->int_data = 0;
	AY_reset();
memset(&RAM,0x00, 131072);	

	zx_RAM_bank_active =0x00;
    zx_RAM_bank_1ffd =0x00;
    zx_RAM_bank_dffd =0x00;

//zx_7ffd_lastOut=0x10;
zx_1ffd_lastOut=0x00;
zx_0000_lastOut = 0x00;// QUORUM

#ifdef TRDOS_1
WD1793_Init();
#endif

	if (PSRAM_AVAILABLE)
	{
		psram_cleanup();//
	}



};
//-------------------------------------------------------------------------
uint8_t* FAST_FUNC(zx_machine_screen_get)(uint8_t* current_screen)
{
	
		return zx_vbuf[0].data; //если буфер 1, то вариантов нет
		

};



void FAST_FUNC(zx_machine_flashATTR)(void)
{
	static bool stateFlash=true;
	stateFlash^=1;
	#if ZX_BPP==4
		if (stateFlash) memcpy(zx_colors_2_pix+512,zx_colors_2_pix,512); else memcpy(zx_colors_2_pix+512,zx_colors_2_pix+1024,512);
		#else
		if (stateFlash) memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix,512*2); else memcpy(zx_colors_2_pix+512*2,zx_colors_2_pix+1024*2,512*2);
	#endif
}

//инициализация массива предпосчитанных цветов
void init_zx_2_pix_buffer()
{
	for(uint16_t i=0;i<384;i++)
	{
		uint8_t color=(uint8_t)i&0x7f;
		uint8_t color0=(color>>3)&0xf;
		uint8_t color1=(color&7)|(color0&0x08);
		
		//убрать ярко чёрный
		//  if (color0==0x80) color0=0;
		//  if (color1==0x80) color1=0;
		
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
// реальный INT 50 HZ через таймер пико
void zx_machine_int(void)
{
	if (conf.turbo == 1)	
	z80_gen_int(&cpu, 0xFF);
}
//----------------------------------------
uint8_t* active_screen_buf=NULL;



void FAST_FUNC(zx_machine_main_loop_start)(){
	
	//переменные для отрисовки экрана
	const int sh_y=56;
	const int sh_x=104;
	uint64_t inx_tick_screen=0;
	uint64_t tick_cpu=0; // Количество тактов до выполнения команды Z80
	uint32_t x=0;
	uint32_t y=0;
	
	init_zx_2_pix_buffer();
	#if (ZX_BPP==4)
		uint8_t* p_scr_str_buf=NULL;
		#else
		uint16_t* p_scr_str_buf=NULL;
	#endif
	uint8_t* p_zx_video_ram=NULL;
	uint8_t* p_zx_video_ramATTR=NULL;
	
	G_PRINTF_INFO("zx mashine starting\n");
	
	// uint64_t dst_time_ns=ext_get_ns();
	uint32_t d_dst_time_ticks=0; // Количесто тактов реального процессора на текущую выполненную команду Z80
	uint32_t t0_time_ticks=0;    // Количество реальных тактов процессора после запуска машины Z80
	
	//int ticks_per_cycle=DEL_Z80;//80// 72;     //от 254МГц: 72 - 3.5МГц, 63 - 4Мгц; 54 - 4Мгц;
    ticks_per_cycle=Z80_3500;//
	ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A

	systick_hw->csr = 0x5;
	systick_hw->rvr = 0xFFFFFF;
	

	

	#ifdef Z80_DEBUG
		bool ttest = false; // Флаг для вызова дебажной информации
	#endif
	
	//init_screen(g_gbuf,320,240);
	active_screen_buf=g_gbuf;
	p_scr_str_buf=active_screen_buf; 
	
	//смещение начала изображения от прерывания
	const int shift_img=(16+40)*224+48;////8888;////Пентагон=(16+40)*224+48;
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
			// inx_tick_screen=0;

			cpu.int_pending = false;
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// tape load
if (enable_tape)
{
if (cpu.pc == 0x0556)  tape_load(); // вход в меню tape // CALL 1366 (0x0556)
//if (cpu.pc == 0x0562)  tape_load_0562(); // вход в меню tape // CALL 0x0562
if (cpu.pc == 0x056a)  tape_load_056a(); // вход в меню tape // CALL 0x056a
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
		// tr-dos


		//if ((zx_1ffd_lastOut & 0x02)== 0x00) // 0000 00x0  0x02 если не теневик 
      {
		if (!trdos) // если еще не в trdos то вход
		{
			if (((cpu.pc & 0x3D00) == 0x3D00) && ((zx_7ffd_lastOut & 0x10) == 0x10) )// trdos работает с BASIC48 D4 = 1                                                                           // 1ffd D1 
			{
			trdos = true;
 

			zx_cpu_ram[0]=zx_rom_bank[2];// tr-dos
      
         //   rom_select(); // переключение ПЗУ по портам  
			}
		}
	  }
		
		if ((cpu.pc > 0x3FFF) && (trdos))// выход из trdos
		{
			trdos = false;

        // rom_select(); // переключение ПЗУ по портам  
          zx_cpu_ram[0]=zx_rom_bank[1];
            
		}

      
	//	wait_msg=1000; // показыват номер банки ROM
           
    //    msg_bar =9;
///////////////////////////////////////////////////////////////////////////////
          //   rom_select(); // переключение ПЗУ по портам и по сигналу DOS
		//=======================================================================================
			// Если нажали клавишу NMI // QUORUM
			if (main_nmi_key)
			{
            	 main_nmi_key = false;
		    if (conf.mashine == NOVA256)
	    		{zx_cpu_ram[0] = zx_rom_bank[3]; zx_0000_lastOut = 0; z80_gen_nmi(&cpu); } // QUORUM

        //   if (conf.mashine == SCORP256)
	    //		{ zx_cpu_ram[0] = zx_rom_bank[3]; profROM=true; z80_gen_nmi(&cpu); } // 



         /*    if (conf.mashine == PENT512)
	    		{
					zx_cpu_ram[0] = zx_rom_bank[3]; 
				 z80_gen_nmi(&cpu); } //
 */

			}
		//=======================================================================================	



///////////////////////////////////////////////////////////////////////////////////////////////////////
		

		// Цикл ождания пока количество потраченных тактов реального процессора
		// меньше количества расчетных тактов реального процессора на команду Z80
		while (((get_ticks()-t0_time_ticks)&0xffffff)<d_dst_time_ticks);
	//	while (((get_ticks()-t0_time_ticks))<d_dst_time_ticks);
if ((inx_tick_screen<32)&&(int_en)) {z80_gen_int(&cpu,0xFF);int_en=false;} // вызов INT в обычном режиме
//if ((int_en==true)) {z80_gen_int(&cpu,0xFF);int_en=false;}

		t0_time_ticks=(t0_time_ticks+d_dst_time_ticks)&0xffffff;   


		tick_cpu=cpu.cyc;                 // Запоминаем количество тактов Z80 до выполнения команды Z80
		z80_step(&cpu);                   // Выполняем очередню команду Z80
	
 {
       #if defined(TRDOS_1) 
       if (trdos) WD1793_Execute();
       #endif  
 } 

	

		dt_cpu=cpu.cyc-tick_cpu; // Вычисляем количество тактов Z80 на выполненную команду.
    	d_dst_time_ticks=dt_cpu*ticks_per_cycle; // Расчетное количесто тактов реального процессора на выполненную команду Z80
	
		inx_tick_screen+=dt_cpu;//Увеличиваем на количество тактов Z80 на текущую выполненную команду.

	 	// начало 

		 if (inx_tick_screen>=ticks_per_frame)      // Если прошла 1/50 сек, 71680 тактов процессора Z80
			{

	        	if (conf.turbo == 1) int_en=false; // выключение INT / вызывается через таймер в TURBO РЕЖИМЕ
	    	    else int_en=true; // включение INT

		 	inx_tick_screen-=ticks_per_frame; //Такты Z80 1/50 секунды
		 	x=0;y=0;
			draw_img_inx=0; //??????????
			p_scr_str_buf=active_screen_buf; 
			
			if (inx_tick_screen==0)  //  0 ?????
			    continue;

		};

		if (!vbuf_en) continue;
		//новая прорисовка
		register int img_inx=(inx_tick_screen-shift_img);

  	//	if (img_inx<0 || (img_inx>=(T_per_line*240))){ //область изображения, если вне, то не рисуем
	 if  (img_inx>=(53760)){ //область изображения, если вне, то не рисуем
			continue;
		}  
		

		//смещения бордера
		const int dy=24;
		const int dx=32;
		
		for(;draw_img_inx<img_inx;){
			
		//	if (x==T_per_line*2) {
		 	if (x==448) {
				x=0;
				y++;
				int ys=y-dy;//номер строки изображения
				p_zx_video_ram=zx_video_ram+(((ys&0b11000000)|((ys>>3)&7)|((ys&7)<<3))<<5);
				//указатель на начало строки байтов цветовых аттрибутов
				p_zx_video_ramATTR=zx_video_ram+(6144+((ys<<2)&0xFFE0));
				
			}; 



			if (x>=(SCREEN_W)||y>=(SCREEN_H))
			{
				x+=8;
				draw_img_inx+=4;
				continue;
			} 
		
//----------------------------------------------------------------
/*   			// рисование полоски на бордере
             if(((y>=240-8) && (y<=240)) )
			 {
			int i_c;
			 	if (x<dx) i_c=MIN((dx-x)/2,img_inx-draw_img_inx);
				else i_c=MIN((SCREEN_W-x)/2,img_inx-draw_img_inx);  
				register uint8_t bc=0xaa;     
				for(int i=i_c;i--;) *p_scr_str_buf++=bc;
				draw_img_inx+=i_c;
				x+=i_c<<1;
               continue;
				}    */

		// HE рисование полоски на бордере
#ifdef TEST
if (wait_msg !=0)
{
			if (((y >= 240 - 8) && (y <= 240)))
			{
				int i_c;
				if (x < dx)
					i_c = MIN((dx - x) / 2, img_inx - draw_img_inx);
				else
					i_c = MIN((SCREEN_W - x) / 2, img_inx - draw_img_inx);
				//		register uint8_t bc=0xaa;
				//		for(int i=i_c;i--;) *p_scr_str_buf++=bc;
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
			




			uint8_t c_attr=*p_zx_video_ramATTR++;
			uint8_t zx_pix8=*p_zx_video_ram++;
			
			if (old_c_attr!=c_attr)//если аттрибуты цвета не поменялись - используем последовательность с прошлого шага
			{
				// uint8_t* zx_colors_2_pix_current=zx_colors_2_pix+(4*(c_attr));
				// colorBuf=zx_colors_2_pix_current);
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


#include "zx_machine.h"
#include "config.h"
#include "../aySoft.h"
#include "wd1793.h"

#include "quorum.h"

extern bool psram_type;
extern uint8_t pent_config;
extern uint32_t ticks_per_frame;

extern bool zx_state_48k_MODE_BLOCK;

extern uint8_t zx_7ffd_lastOut;
extern uint8_t zx_1ffd_lastOut;
extern uint8_t zx_0000_lastOut;

extern uint32_t zx_RAM_bank_active;
extern uint8_t zx_RAM_bank_7ffd;


extern uint8_t rom;
extern uint8_t *zx_cpu_ram[];
extern uint8_t *zx_ram_bank[]; 
extern uint8_t* zx_rom_bank[];

extern uint8_t* zx_video_ram;


extern volatile uint8_t * PSRAM_DATA ;

extern uint8_t zx_keyboardDecode(uint8_t addrH);
extern uint8_t port_atr(void);
extern ZX_Input_t zx_input;
void trdos_out(uint8_t port, uint8_t val);
extern uint8_t wd1793_PortFF;

uint8_t qu_ExtKb[8];

void fast(pager7ffd_Quorum1024)(uint8_t val) {
	//zx_RAM_bank_active  = (val&0b00000111); //128K only
    // linear bank numbering, bits 5 7 6 3 2 1
	zx_RAM_bank_active  = (val & 0b00100111) | ((val >> 3) & 0b00011000); //1024k

    //if (val& 0x20) zx_state_48k_MODE_BLOCK=true; // 5bit = 1 48k mode block
    zx_state_48k_MODE_BLOCK = false;
    //        76543210  5 bit
    zx_RAM_bank_7ffd = (val&0b00000111) ; // 
       
    #if RP2350_256K 
	    zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active & 0x0f];
    #else
	   zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active & 0x07];
    #endif
	
	if (val&8) zx_video_ram=zx_ram_bank[7];   else zx_video_ram=zx_ram_bank[5];	
    rom_select(); // переключение ПЗУ по портам и по сигналу DOS
}

void fast(rom_select_Quorum1024)() {
    if ((zx_0000_lastOut & 0b00000001)) {
        //enable ram page
        zx_cpu_ram[0] = zx_rom_bank[3]; // STUB
    } else 
    if ((zx_0000_lastOut & 0b00100000) == 0) {
	    rom=3;
        zx_cpu_ram[0] = zx_rom_bank[3]; 
    }
	else 
	{
		rom=(zx_7ffd_lastOut & 0x10)>>4;  // 1 if bit4 is set else 0
		zx_cpu_ram[0]=zx_rom_bank[rom]; 
	} 

}

uint8_t QuorumExtKeyboardDecode(uint8_t portH) {
    uint8_t result = 0;
    uint8_t *bp = qu_ExtKb;
    portH = ~portH;

    if (portH & (1<<0)) result |= bp[0]; 
    if (portH & (1<<1)) result |= bp[1]; 
    if (portH & (1<<2)) result |= bp[2]; 
    if (portH & (1<<3)) result |= bp[3]; 
    if (portH & (1<<4)) result |= bp[4]; 
    if (portH & (1<<5)) result |= bp[5]; 
    if (portH & (1<<6)) result |= bp[6]; 
    if (portH & (1<<7)) result |= bp[7];

    return ~result; 
}

#ifdef MURM1
#include "psram_spi.h"
#endif

//#include "rom/QU7V45T5.h" // original Quorum 256-Quorum1024 rom
#include "rom/QU7V42T5.h" // original Quorum 256-Quorum1024 rom


void init_rom_ram_Q1024() {
	zx_rom_bank[0]=ROM_B128_QU1024;//128k 
	zx_rom_bank[1]=ROM_B48_QU1024; //48k 
	zx_rom_bank[2]=ROM_TRD_QU1024; //TRDOS 6.04
	zx_rom_bank[3]=ROM_SM_QU1024;  //SYSTEM MENU
	rom=3;
	zx_cpu_ram[0]=zx_rom_bank[3]; // 0x0000 - 0x3FFF с какой банки стартовать

   	zx_RAM_bank_active =0x00;

	zx_state_48k_MODE_BLOCK = false;
}


void nmi_Quorum1024(Z80 *cpu)
{
    rom=3;  
    zx_cpu_ram[0] = zx_rom_bank[3];
    zx_0000_lastOut = 0; 
}

#ifdef MURM1
// чтение из памяти Quorum (PSRAM MURM1)
inline static uint8_t fast(read_z80_q)(Z80 *cpu, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
    {
        uint8_t ram0 = zx_0000_lastOut & 0b1001;
        switch (ram0) {
        case 0b0001: return zx_ram_bank[0][masked_addr]; break;
        case 0b1001: return read8psram((uint32_t)(8 << 14) | masked_addr); break;
        default: return zx_cpu_ram[0][masked_addr]; break;
        }
    }
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) 
    {
    	//	 write_z80_qif (zx_RAM_bank_active > 7)// 0b 1111 1000
		if (zx_RAM_bank_active & 0xf8)  return read8psram((uint32_t)(zx_RAM_bank_active << 14) | masked_addr); 
    }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
//##########################################################################################
// запись в память Quorum
inline static void fast(write_z80_q)(Z80 *cpu, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
    {
        uint8_t ram0 = zx_0000_lastOut & 0b1001;
        switch (ram0) {
        case 0b0001: zx_ram_bank[0][masked_addr] = val; return; break;
        case 0b1001: write8psram((uint32_t)(8 << 14) | (masked_addr), val); return; break;
        default: return; //TODO write shadowed page
        }
    }
  
    if(x == 3) {
        if (zx_RAM_bank_active & 0xf8) {
            write8psram((uint32_t)(zx_RAM_bank_active << 14) | (masked_addr), val);
            return;
        }
    }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	zx_cpu_ram[x][masked_addr] = val;
}
#endif

// чтение из памяти Quorum
inline static uint8_t fast(_read_z80_q)(Z80 *cpu, uint16_t addr)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
    uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
    {
        uint8_t ram0 = zx_0000_lastOut & 0b1001;
        switch (ram0) {
        case 0b0001: return zx_ram_bank[0][masked_addr]; break;
        case 0b1001: return PSRAM_DATA[(8 << 14) | masked_addr]; break;
        default: return zx_cpu_ram[0][masked_addr]; break;
        }
    }
    // Обработка верхнего сегмента (0xc000-0xffff)   
    if(x == 3) {
		if (zx_RAM_bank_active & 0xf8) return PSRAM_DATA[(zx_RAM_bank_active << 14) | masked_addr];
    }  
    // Общий случай для x=1,2 и x=3 с обычной RAM
	return zx_cpu_ram[x][masked_addr];
}
// запись в память Quorum
inline static void fast(_write_z80_q)(Z80 *cpu, uint16_t addr, uint8_t val)
{
    const uint16_t masked_addr = addr & 0x3fff;  // Предвычисление маскированного адреса
	uint8_t x = (addr >> 14);
    // Обработка первого сегмента (0x0000-0x3fff)
    if(x == 0) 
    {
        uint8_t ram0 = zx_0000_lastOut & 0b1001;
        switch (ram0) {
        case 0b0001: zx_ram_bank[0][masked_addr] = val; return; break;
        case 0b1001: PSRAM_DATA[(8 << 14) | masked_addr] = val; return; break;
        default: return; break;
        }
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

//###############################################
// IN QUORUM
//###############################################

inline static uint8_t fast(in_z80quorum)(Z80 *cpu, uint16_t port16) {
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
            
        return 0xFF;  
	} // end tr-dos

    if ((portL & 0xF0) == 0x80) {
        // TR-DOS or CPM?
        if ((zx_0000_lastOut & 0x80) == 0) {
            switch (portL) {
                case 0x80:
                    return (WD1793_Read(0) & 0x7f) | ((wd1793_PortFF & (1<<3))  ? 0x00 : 0x80);
                case 0x81:
                case 0x82:
                case 0x83: return WD1793_Read(portL & 0b11);
                default: return 0xff; //0x84, 0x85;
            }
            if (portL & 0x4) 
                return 0xff; //0x84, 0x85
            else 
                return WD1793_Read(portL & 0b11);
        } 
    }

	if (portL & 1<<0)
	{
		// МЫШЬ
        if (port16 == 0xfadf) return mouse[1]; //#FADF - поpт  кнопок
        if (port16 == 0xfbdf) return mouse[2]; //#FBDF - поpт X-кооpдинаты;
        if (port16 == 0xffdf) return mouse[3]; //#FFDFportL & 0b11 - поpт У-кооpдинаты.

        //Kempston джойстик    
        if (portL==0x1f) return (zx_input.kempston | joy_k);
            
        #ifdef  TURBOSOUND         
        if ((port16 & 0xc002) == 0xc000) 	return in_GSP(TS_READ_IN_FFFD); 
        #else  
		if ((port16 & 0xc002) == 0xc000) 	return AY_in_FFFD(); 
        #endif    
	}
	else    //PORTL = 0bxxxxxx0
    {   
        // 0xXX7e - extended keyboard
        if (portL == 0x7e) {
            uint8_t value = QuorumExtKeyboardDecode(portH) & 0x3f; //TODO keys
            if (zx_7ffd_lastOut & (1<<3)) value |= 0x80;    // screen bit readout
            if (zx_0000_lastOut & (1<<3)) value |= 0x40;    // RAM_8 bit readout
            return value; 
        }
		//загрузка с магнитофона и опрос клавиатуры
		if (hw_zx_get_bit_LOAD())  return zx_keyboardDecode(portH);
		else return(zx_keyboardDecode(portH) & 0b10111111);	
    }

  if (portL== 0xFF) return port_atr();
	return 0xFF;
}

//===========================================================================
// Quorum 512
//===========================================================================
inline static void fast(out_z80quorum)(Z80 *cpu, uint16_t port16, uint8_t val)
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

    if ((portL & 0xF0) == 0x80) {
        // TR-DOS or CPM?
        if ((zx_0000_lastOut & 0x80) == 0) {
            if (portL < 0x84) { WD1793_Write(portL & 0b11, val); return; }
            if (portL == 0x85) {
                uint8_t pff = 0b0100;
                switch (val & QCPM85_DRVMASK) {
                    case QCPM85_DRVA: pff |= 0b00; break; //drive A
                    case QCPM85_DRVB: pff |= 0b01; break; //drive B
                    default: pff |= 0x00; break; // fallback to A
                }
                if (val & QCPM85_MOTOR) pff |= 1<<3;  //Head load
                if ((val & QCPM85_SIDE) == 0) pff |= 1<<4; //Side (inverted)
                wd1793_PortFF = pff;
                return;
            }
        } 
    }

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
// end Quorum_1024

#define KA8  (0 )
#define KA9  (1 )
#define KA10 (2)
#define KA11 (3)
#define KA12 (4)
#define KA13 (5)
#define KA14 (6)
#define KA15 (7)
#define SET_EXT_KEY(AX, DX) (qu_ExtKb[AX] |= (1<<DX))
#define SET_ZX_KEY(AX, DX) (zx_kb[AX] |= (1<<DX))
#define CLR_ZX_KEY(AX, DX) (zx_kb[AX] &= ~(1<<DX))

void convertQuorumKbAccords(kb_u_state* kb_st,uint8_t* zx_kb) 
{
    //fast fill array with zero
    // *((uint32_t*)(qu_ExtKb)) = 0;
    // *((uint32_t*)(qu_ExtKb+4)) = 0;

    // memset also optimizes small array fill, code will be equivalent to shown upper
    memset(qu_ExtKb, 0, sizeof(qu_ExtKb) / sizeof(char));

    uint32_t u0 = kb_st->u[0];
    if (u0) {
        if (u0 & KB_U0_SEMICOLON) {SET_EXT_KEY(KA13, 0); };
        if (u0 & KB_U0_QUOTE)     {SET_EXT_KEY(KA9,  4); };
        if (u0 & KB_U0_COMMA)     {SET_EXT_KEY(KA14, 0); };
        if (u0 & KB_U0_PERIOD)    {SET_EXT_KEY(KA8,  5); };
        if (u0 & KB_U0_LEFT_BR)   {SET_EXT_KEY(KA13, 5); };
        if (u0 & KB_U0_RIGHT_BR)  {SET_EXT_KEY(KA13, 4); };
    }

    uint32_t u1 = kb_st->u[1];
    if (u1) {
        if (u1 & KB_U1_SLASH)     {SET_EXT_KEY(KA14, 4); };
        if (u1 & KB_U1_MINUS)     {SET_EXT_KEY(KA12, 0); };

        if (u1 & KB_U1_EQUALS)    {SET_EXT_KEY(KA12, 2); };
        if (u1 & KB_U1_BACKSLASH) {SET_EXT_KEY(KA13, 2); };
        if (u1 & KB_U1_CAPS_LOCK) {SET_EXT_KEY(KA9,  0); };
        if (u1 & KB_U1_TAB)       {SET_EXT_KEY(KA10, 0); };
        //if (u1 & KB_U1_BACK_SPACE) {SET_EXT_KEY(KA11, 2); };
        if (u1 & KB_U1_BACK_SPACE) {SET_EXT_KEY(KA12, 3); };
        if (u1 & KB_U1_ESC)       {SET_EXT_KEY(KA11, 0); };
        if (u1 & KB_U1_TILDE)     {SET_EXT_KEY(KA9,  2); };
        //if (u1 & KB_U1_MENU) {};

        if (u1 & (KB_U1_L_ALT | KB_U1_R_ALT | KB_U1_L_WIN | KB_U1_R_WIN)) {
            // F-KEYS and RUS/LAT
            if (u1 & KB_U1_1) {SET_EXT_KEY(KA15,  1); CLR_ZX_KEY(KA11, 0);}    // Quorum F1
            if (u1 & KB_U1_2) {SET_EXT_KEY(KA9,   1); CLR_ZX_KEY(KA11, 1);}    // Quorum F2
            if (u1 & KB_U1_3) {SET_EXT_KEY(KA13,  1); CLR_ZX_KEY(KA11, 2);}    // Quorum F3
            if (u1 & KB_U1_4) {SET_EXT_KEY(KA10,  1); CLR_ZX_KEY(KA11, 3);}    // Quorum F4
            if (u1 & KB_U1_5) {SET_EXT_KEY(KA11,  1); CLR_ZX_KEY(KA11, 4);}    // Quorum F5
            if (u1 & KB_U1_6) {SET_EXT_KEY(KA12,  5); CLR_ZX_KEY(KA12, 4);}    // Quorum F6/G

            if (u1 & KB_U1_9) {SET_EXT_KEY(KA8,   0); CLR_ZX_KEY(KA12, 1);}    // Quorum RUS
            if (u1 & KB_U1_0) {SET_EXT_KEY(KA8,   1); CLR_ZX_KEY(KA12, 0);}    // Quorum LAT
        }
    }

    uint32_t u2 = kb_st->u[2];
    if (u2) {
        //if (u2 & KB_U2_DELETE)      {SET_EXT_KEY(KA12, 3); };
        if (u2 & KB_U2_DELETE)      {SET_EXT_KEY(KA11, 2); };
        if (u2 & KB_U2_NUM_0)       {SET_EXT_KEY(KA15, 3); };
        if (u2 & KB_U2_NUM_1)       {SET_EXT_KEY(KA8,  3); };
        if (u2 & KB_U2_NUM_2)       {SET_EXT_KEY(KA8,  4); };
        if (u2 & KB_U2_NUM_3)       {SET_EXT_KEY(KA14, 5); };
        if (u2 & KB_U2_NUM_4)       {SET_EXT_KEY(KA9,  3); };
        if (u2 & KB_U2_NUM_5)       {SET_EXT_KEY(KA10, 4); };
        if (u2 & KB_U2_NUM_6)       {SET_EXT_KEY(KA9,  5); };
        if (u2 & KB_U2_NUM_7)       {SET_EXT_KEY(KA10, 3); };
        if (u2 & KB_U2_NUM_8)       {SET_EXT_KEY(KA11, 4); };
        if (u2 & KB_U2_NUM_9)       {SET_EXT_KEY(KA10, 5); };
        if (u2 & KB_U2_NUM_ENTER)   {zx_kb[6]|=1<<0;};
        if (u2 & KB_U2_NUM_SLASH)   {SET_EXT_KEY(KA11, 3); };
        if (u2 & KB_U2_NUM_MINUS)   {SET_EXT_KEY(KA11, 5); };
        if (u2 & KB_U2_NUM_PLUS)    {SET_EXT_KEY(KA15, 5); };
        if (u2 & KB_U2_NUM_PERIOD)  {SET_EXT_KEY(KA15, 4); };
        if (u2 & KB_U2_NUM_MULT)    {SET_EXT_KEY(KA12, 4); };   // F8 на схеме?
    }

}

void machine_Quorum1024(Z80 *cpu) {
    cpu->context      = cpu;
    #ifdef MURM1
    if (psram_type) {
        cpu->fetch_opcode = (Z80Read )read_z80_q;
        cpu->fetch        = (Z80Read )read_z80_q;
        cpu->nop          = (Z80Read )read_z80_q;
        cpu->read         = (Z80Read )read_z80_q;
        cpu->write        = (Z80Write)write_z80_q;
    }
    else
    #endif
    {
        cpu->fetch_opcode = (Z80Read )_read_z80_q;
        cpu->fetch        = (Z80Read )_read_z80_q;
        cpu->nop          = (Z80Read )_read_z80_q;
        cpu->read         = (Z80Read )_read_z80_q;
        cpu->write        = (Z80Write)_write_z80_q;
    }
    cpu->in           = (Z80Read )in_z80quorum;
    cpu->out          = (Z80Write)out_z80quorum;
    cpu->halt         = Z_NULL;
    cpu->nmia         = (Z80Read )nmi_Quorum1024;
    cpu->inta         = Z_NULL;//= (Z80Read )inta_callback;
    cpu->int_fetch    = Z_NULL;
    cpu->ld_i_a       = Z_NULL;
    cpu->ld_r_a       = Z_NULL;
    cpu->reti         = Z_NULL;
    cpu->retn         = Z_NULL;
    cpu->hook         = Z_NULL;
    cpu->illegal      = Z_NULL;
    
    pent_config = QUORUM1024;
    ticks_per_frame=71680 ;// 71680- Пентагон //70908 - 128 +2A // 70784 Scorpion

    setZxExtKeysHook(convertQuorumKbAccords);
}

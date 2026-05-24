#include "disassembler.h"
#include "zx_machine.h"
#include "../screen_util.h"
#include <stdio.h>
#include <string.h>

extern Z80 cpu_zx;
// Определение глобальных переменных
char tmp_opcode[16];
uint16_t dis_adres;
uint16_t address_pc;

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

void     print_interrupt_status(void) {
        switch (get_interrupt_state(&cpu_zx)) {
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
//=========================================================
void disassembler(void) 
{

   // write_z80(z1,0x81fe,0x18); //0x38
 //write_z80(z1,0x8207,0x00); //0x76
 //address_pc =Z80_PC(cpu_zx);
	draw_rect(0, 10, 318, 210, CL_BLACK, true);				   // рамка 3 фон
	draw_rect(0, 10, 318, 210, CL_GRAY, false);				   // рамка 1
	///draw_rect(0 + 2, 10 + 2, 318 - 4, 200 - 4, CL_GRAY, false); // рамка 2

	//draw_rect(0 + 3, 10 + 3, 318 - 6, 8, CL_GRAY, true);			 // шапка меню
	//draw_text(0 + 10, 10 + 3, "ZX Keyboard  [ESC] Exit", CL_BLACK, CL_GRAY); // шапка меню
       uint16_t y=9;
      snprintf(temp_msg, sizeof temp_msg, "PC %04X", Z80_PC(cpu_zx));
      draw_text(0 + 10, 10 + y, temp_msg,CL_GRAY , CL_BLACK); //

       snprintf(temp_msg, sizeof temp_msg, "SP %04X", Z80_SP(cpu_zx));
      draw_text(0 + 10, 10 + y*2, temp_msg,CL_GRAY , CL_BLACK); //    

       snprintf(temp_msg, sizeof temp_msg, "IX %04X", Z80_IX(cpu_zx));
      draw_text(0 + 10, 10 + y*3, temp_msg,CL_GRAY , CL_BLACK); //    

        snprintf(temp_msg, sizeof temp_msg, "IY %04X", Z80_IY(cpu_zx));
      draw_text(0 + 10, 10 + y*4, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "HL %04X",  Z80_HL(cpu_zx) );
      draw_text(0 + 10, 10 + y*5, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "DE %04X",  Z80_DE(cpu_zx));
      draw_text(0 + 10, 10 + y*6, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "BC %04X",  Z80_BC(cpu_zx) );
      draw_text(0 + 10, 10 + y*7, temp_msg,CL_GRAY , CL_BLACK); // 

        snprintf(temp_msg, sizeof temp_msg, "AF %04X",  Z80_AF(cpu_zx));
      draw_text(0 + 10, 10 + y*8, temp_msg,CL_GRAY , CL_BLACK); // 

       snprintf(temp_msg, sizeof temp_msg, "HL'%04X",  Z80_HL_(cpu_zx));
      draw_text(0 + 10, 10 + y*9, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "DE'%04X",  Z80_DE_(cpu_zx));
      draw_text(0 + 10, 10 + y*10, temp_msg,CL_GRAY , CL_BLACK); //  

        snprintf(temp_msg, sizeof temp_msg, "BC'%04X",  Z80_BC_(cpu_zx));
      draw_text(0 + 10, 10 + y*11, temp_msg,CL_GRAY , CL_BLACK); // 

        snprintf(temp_msg, sizeof temp_msg, "AF'%04X",  (Z80_AF_(cpu_zx)));
      draw_text(0 + 10, 10 + y*12, temp_msg,CL_GRAY , CL_BLACK); // 

     
        snprintf(temp_msg, sizeof temp_msg, "IR %02X%02X",  cpu_zx.i,cpu_zx.r);
        draw_text(0 + 10, 10+ y*13, temp_msg,CL_GRAY , CL_BLACK); // 



        print_interrupt_status();
       
        snprintf(temp_msg, sizeof temp_msg, "IM%01X",  cpu_zx.im);
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
                    if (address_pc==Z80_PC(cpu_zx))//dis_adres =Z80_PC(cpu_zx);
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
				snprintf(temp_msg, sizeof temp_msg, "%02X            ", read_memory(&cpu_zx,address_pc) );//1 byte
				address_pc=address_pc+1;
					break;
				 case 2:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X        ", read_memory(&cpu_zx,address_pc),read_memory(&cpu_zx,address_pc+1) );//2 byte
				address_pc=address_pc+2;
					break;	
				 case 3:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X%02X    ", read_memory(&cpu_zx,address_pc),read_memory(&cpu_zx,address_pc+1),read_memory(&cpu_zx,address_pc+2) );//3 byte
				address_pc=address_pc+3;
					break;	
				 case 4:					
				snprintf(temp_msg, sizeof temp_msg, "%02X%02X%02X%02X", read_memory(&cpu_zx,address_pc),read_memory(&cpu_zx,address_pc+1),read_memory(&cpu_zx,address_pc+2),read_memory(&cpu_zx,address_pc+3) );//4byte
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
				 {
					uint8_t b[9];
                    uint8_t s[9];
                    uint8_t c;
                    for (int j = 0; j < 8; j++)
					{
                    c = read_memory(&cpu_zx,address_pc+j);
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
	uint8_t b0 = read_memory(&cpu_zx,address_pc);
	uint8_t b1 = read_memory(&cpu_zx,address_pc+1);
	uint8_t b2 = read_memory(&cpu_zx,address_pc+2);
	uint8_t b3 = read_memory(&cpu_zx,address_pc+3);
	uint8_t b4 = read_memory(&cpu_zx,address_pc+4);
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
    x_byte = read_memory(&cpu_zx,address_pc+1);
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

    switch ( read_memory(&cpu_zx,p) ) { // Opcode
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
        switch ( read_memory(&cpu_zx,p+1)) { // 2nd part of the opcode
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
        switch ( read_memory(&cpu_zx,p+1) ) { // 2nd part of the opcode
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
        switch ( read_memory(&cpu_zx,p+1) ) { // 2nd part of the opcode
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

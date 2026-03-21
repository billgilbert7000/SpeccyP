#include "util_tap.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include "zx_emu/Z80.h"
#include "pico/stdlib.h"
#include "ps2.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"
#include <math.h>
#include "stdbool.h"

#include "ff.h"
#include "config.h"  
//#define ZX_RAM_PAGE_SIZE 0x4000
#define BUFF_PAGE_SIZE 0x200
TapeBlock tap_blocks[TAPE_BLK_SIZE];
uint8_t tape_autoload_status;
uint16_t tap_block_position;
char tapeFileName[160];
uint8_t TapeStatus;
 uint8_t SaveStatus;
uint8_t RomLoading;
size_t file_pos;


uint8_t tapeCurrentBlock;
size_t tapeFileSize;
uint32_t tapeTotByteCount;



extern volatile Z80 cpu;
extern char temp_msg[80];
char tape_file_name[DIRS_DEPTH*LENF]; // 400 // 5*16 96

#include "zx_emu/zx_machine.h"
#include "screen_util.h"

FIL f;
//int tfd =-1; //tape file descriptor
size_t bytesRead;
size_t bytesToRead;
uint8_t tapBlocksCount=0;
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
char tapeBHbuffer[20]; //tape block header buffer
bool tap_loader_active;
/*
typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;
*/
/* 


uint8_t TapeStatus;
uint8_t SaveStatus;
uint8_t RomLoading;
FIL f;
int tfd =-1; //tape file descriptor
size_t bytesRead;
size_t bytesToRead;

static uint8_t tapePhase;
static uint64_t tapeStart;
static uint32_t tapePulseCount;
static uint16_t tapeBitPulseLen;   
static uint8_t tapeBitPulseCount;     


static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t* tape;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask; 

uint8_t tapeCurrentBlock;
size_t tapeFileSize;
uint32_t tapeTotByteCount;




uint8_t __not_in_flash_func(TAP_Read)(){
//uint8_t TAP_Read(){
    uint64_t tapeCurrent = Z80_C(z1->cpu)yc - tapeStart;
    ////printf("Tape PHASE:%X\n",tapePhase);
    switch (tapePhase) {
    case TAPE_PHASE_SYNC:
        if (tapeCurrent > TAPE_SYNC_LEN) {
            tapeStart=Z80_C(z1->cpu)yc;
            tapeEarBit ^= 1;
            tapePulseCount++;
            if (tapePulseCount>tapeHdrPulses) {
                tapePulseCount=0;
                tapePhase=TAPE_PHASE_SYNC1;
            }
        }
        break;
    case TAPE_PHASE_SYNC1:
        if (tapeCurrent > TAPE_SYNC1_LEN) {
            tapeStart=Z80_C(z1->cpu)yc;
            tapeEarBit ^= 1;
            tapePhase=TAPE_PHASE_SYNC2;
        }
        break;
    case TAPE_PHASE_SYNC2:
        if (tapeCurrent > TAPE_SYNC2_LEN) {
            tapeStart=Z80_C(z1->cpu)yc;
            tapeEarBit ^= 1;
            if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;            
            tapePhase=TAPE_PHASE_DATA;
        }
        break;
    case TAPE_PHASE_DATA:
        if (tapeCurrent > tapeBitPulseLen) {
            tapeStart=Z80_C(z1->cpu)yc;
            tapeEarBit ^= 1;
            tapeBitPulseCount++;
            if (tapeBitPulseCount==2) {
                tapeBitPulseCount=0;
                tapeBitMask = (tapeBitMask >>1 | tapeBitMask <<7);
                if (tapeBitMask==0x80) {
                    tapebufByteCount++;
					tapeBlockByteCount++;
					tapeTotByteCount++;
					//printf("BUF:%d BLOCK:%d TOTAL:%d\n",tapebufByteCount,tapeBlockByteCount,tapeTotByteCount);
					if(tapebufByteCount>=BUFF_PAGE_SIZE){
						//printf("Read next buffer\n");
						tfd = f_read(&f,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
                    	//printf("bytesRead=%d\n",bytesRead);
		                if (tfd!=FR_OK){
							//printf("Error read SD\n");
							f_close(&f);
							tap_loader_active = false;
							tapebufByteCount=0;
							//im_z80_stop = false;       
							TapeStatus=TAPE_STOPPED;
							return false;
						}
						//im_z80_stop = false;
						tapebufByteCount=0;
					}
					if(tapeBlockByteCount==(tapeBlockLen-2)){
						tapeTotByteCount+=2;
						//printf("Wait next block: %d\n",tapeTotByteCount);
						tapebufByteCount=0;
                        tapePhase=TAPE_PHASE_PAUSE;
                        tapeEarBit=false;
						if (tapeTotByteCount == tapeFileSize){
							//printf("Full Read TAPE_STOPPED 2\n");
							TapeStatus=TAPE_STOPPED;
							TAP_Rewind();
							tap_loader_active = false;
							return false;
						}
                        break;
					}					
                }
                if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;
            }
        }
        break;
    case TAPE_PHASE_PAUSE:
        if (tapeTotByteCount < tapeFileSize) {
            if (tapeCurrent > TAPE_BLK_PAUSELEN) {
				tapeCurrentBlock++;
				f_lseek(&f,tap_blocks[tapeCurrentBlock].FPos);
				tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
				bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
				tfd = f_read(&f,sd_buffer,bytesToRead,&bytesRead);
				if (tfd != FR_OK){
					//printf("Error read SD\n");
					f_close(&f);
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);
                tapeStart=Z80_C(z1->cpu)yc;
                tapePulseCount=0;
                tapePhase=TAPE_PHASE_SYNC;
                tapebufByteCount=2;
				tapeBlockByteCount=0;
				//printf("Flag:%X, DType:%X \n",tap_blocks[tapeCurrentBlock].Flag,tap_blocks[tapeCurrentBlock].DataType);
                if (tap_blocks[tapeCurrentBlock].Flag) tapeHdrPulses=TAPE_HDR_SHORT; else tapeHdrPulses=TAPE_HDR_LONG;
            }
        } else {
			//printf("Full Read TAPE_STOPPED\n");
			TapeStatus=TAPE_STOPPED;
			TAP_Rewind();
			tap_loader_active = false;
			break;
		}
        return false;
    } 
  
    return tapeEarBit;
}


void __not_in_flash_func(TAP_Play)(){
//void TAP_Play(){
    switch (TapeStatus) {
    case TAPE_STOPPED:
		//TAP_Load(conf.activefilename);
       	tapePhase=TAPE_PHASE_SYNC;
       	tapePulseCount=0;
       	tapeEarBit=false;
       	tapeBitMask=0x80;
       	tapeBitPulseCount=0;
       	tapeBitPulseLen=TAPE_BIT0_PULSELEN;
       	tapeHdrPulses=TAPE_HDR_LONG;
		f_lseek(&f,tap_blocks[tapeCurrentBlock].FPos);
		tapeTotByteCount = tap_blocks[tapeCurrentBlock].FPos;
		tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
		bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
		tfd = f_read(&f,sd_buffer,bytesToRead,&bytesRead);
		if (tfd != FR_OK){f_close(&f);break;}
		//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);		
       	tapebufByteCount=2;
		tapeBlockByteCount=0;
       	tapeStart=Z80_C(z1->cpu)yc;
       	TapeStatus=TAPE_LOADING;
       	break;
    case TAPE_LOADING:
       	TapeStatus=TAPE_PAUSED;
       	break;
    case TAPE_PAUSED:
        tapeStart=Z80_C(z1->cpu)yc;
        TapeStatus=TAPE_LOADING;
		break;
    }
}


void Init(){
	TapeStatus = TAPE_STOPPED;
	SaveStatus = SAVE_STOPPED;
	RomLoading = false;
}

bool TAP_Load(char *file_name){
	
	//printf("Tap Load begin\n");
    TapeStatus = TAPE_STOPPED;
	//printf("Tap FN:%s\n",file_name);
	tfd = f_open(&f,file_name,FA_READ);
    //printf("f_open=%d\n",tfd);
	if (tfd!=FR_OK){f_close(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    //printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = f_read(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){f_close(&f);return false;}
		//printf(" Readbuf:%d\n", bytesRead);
		//printf(" pos:%d\n", tapeTotByteCount);
		TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
		tap_blocks[tapBlocksCount].Size = block->Size;
		//printf(" block:%d, size:%d ",tapBlocksCount,block->Size);
		memset(tap_blocks[tapBlocksCount].NAME, 0, sizeof(tap_blocks[tapBlocksCount].NAME));
		if (block->Flag==0){
			memcpy(tap_blocks[tapBlocksCount].NAME,block->NAME,10);
			//printf(" header:%s", block->NAME);
		}
		tap_blocks[tapBlocksCount].Flag = block->Flag;
		tap_blocks[tapBlocksCount].DataType = block->DataType;
		//printf(" flag:%d, datatype:%d \n", block->Flag,block->DataType);
		tap_blocks[tapBlocksCount].FPos=tapeTotByteCount;
		tapeTotByteCount+=block->Size+2;
		f_lseek(&f,tapeTotByteCount);
		tapBlocksCount++;
		if(tapeTotByteCount>=tapeFileSize){
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			break;
		}
	}
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	f_lseek(&f,0);
	tape = (uint8_t *)&sd_buffer;//error




	
    return true;
}

void TAP_Rewind(){
	TapeStatus=TAPE_STOPPED;
	//printf("Tape Rewind\n");
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeCurrentBlock=0;	
	tapeTotByteCount=0;
};

void TAP_NextBlock(){
	//printf("Tape NextBlock\n");
	tapeCurrentBlock++;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		TapeStatus=TAPE_STOPPED;
		TAP_Play();
	}
	if (tapeCurrentBlock>tapBlocksCount){
		TAP_Rewind();
	}

};

void TAP_PrevBlock(){
	//printf("Tape PrevBlock\n");
	tapeCurrentBlock--;
	if ((tapeCurrentBlock>=0)&&(tapeCurrentBlock<tapBlocksCount)){
		TapeStatus=TAPE_STOPPED;
		TAP_Play();
	}
	if (tapeCurrentBlock>tapBlocksCount){
		TAP_Rewind();
	}
};

 */
/*

#define coef           (0.990) //0.993->
#define PILOT_TONE     (2168*coef) //2168
#define PILOT_SYNC_HI  (667*coef)  //667
#define PILOT_SYNC_LOW (735*coef)  //735
#define LOG_ONE        (1710*coef) //1710
#define LOG_ZERO       (855*coef)  //855

	#define PILOT_TONE     (2168) //2168
	#define PILOT_SYNC_HI  (667)  //667
	#define PILOT_SYNC_LOW (735)  //735
	#define LOG_ONE        (1710) //1710
	#define LOG_ZERO       (855)  //855
*/

bool LoadScreenFromTap(char *file_name){

	bool screen_found = false;
	uint8_t* bufferOut;

	memset(sd_buffer, 0, sizeof(sd_buffer));

	for(uint8_t i=0;i<TAPE_BLK_SIZE;i++){
		tap_blocks[i].DataType=0;
		tap_blocks[i].Flag=0;
		tap_blocks[i].FPos=0;
		tap_blocks[i].Size=0;
		tap_blocks[i].NAME[0]=0;
	}
	//FIL f;
    int tfd =-1; //tape file descriptor

	tfd = f_open(&f,file_name,FA_READ);
    //printf("f_open=%d\n",tfd);
	if (tfd!=FR_OK){f_close(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    //printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = f_read(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){f_close(&f);return false;}
		//printf(" Readbuf:%d\n", bytesRead);
		//printf(" pos:%d\n", tapeTotByteCount);
		TapeBlock* block = (TapeBlock*) &tapeBHbuffer;
		tap_blocks[tapBlocksCount].Size = block->Size;
		//printf(" block:%d, size:%d ",tapBlocksCount,block->Size);
		memset(tap_blocks[tapBlocksCount].NAME, 0, sizeof(tap_blocks[tapBlocksCount].NAME));
		if (block->Flag==0){
			memcpy(tap_blocks[tapBlocksCount].NAME,block->NAME,10);
			//printf(" header:%s", block->NAME);
		}
		tap_blocks[tapBlocksCount].Flag = block->Flag;
		tap_blocks[tapBlocksCount].DataType = block->DataType;
		//printf(" flag:%d, datatype:%d \n", block->Flag,block->DataType);
		tap_blocks[tapBlocksCount].FPos=tapeTotByteCount;
		tapeTotByteCount+=block->Size+2;
		f_lseek(&f,tapeTotByteCount);
		tapBlocksCount++;
		if(tapeTotByteCount>=tapeFileSize){
			break;
		}
		if(tapBlocksCount==TAPE_BLK_SIZE){
			break;
		}
	}
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	tapeCurrentBlock=0;
	f_lseek(&f,0);

	for (int8_t i=0;i<tapBlocksCount;i++){
		if (tap_blocks[i].Flag>0){
			if((tap_blocks[i].Size>=0x1AFE)&&(tap_blocks[i].Size<=0x1B02)){
				f_lseek(&f,tap_blocks[i].FPos);
				//bufferOut = (SD_BUFFER_SIZE>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
               bufferOut = (sizeof(sd_buffer)>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];

				memset(sd_buffer, 0, sizeof(sd_buffer));
				tfd = f_read(&f,bufferOut,tap_blocks[i].Size,&bytesRead);
	        	//printf("bytesRead=%d\n",bytesRead);
	        	if (tfd!=FR_OK){f_close(&f);return false;}
		        ShowScreenshot(bufferOut);
				screen_found = true;
				break;
			}
		}			
	}

	if (!screen_found){
		draw_text_len(18+FONT_W*14,42+20,"File TAPE:",COLOR_TEXT,COLOR_BACKGOUND,14);
		uint8_t j =1;
		for (uint8_t i = 0; i < 22; i++){
			if (tap_blocks[i].Size>0){
				if (tap_blocks[i].Flag==0){
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg,"%s",tap_blocks[i].NAME);
					draw_text_len(10+FONT_W*15,62+FONT_H*(j),temp_msg,CL_GRAY,COLOR_BACKGOUND,10);
					j++;
				} 
				
 				else 
				{
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg," %d",(tap_blocks[i].Size));
					draw_text_len(90+FONT_W*15,62+FONT_H*(j-1),temp_msg,CL_LT_CYAN,COLOR_BACKGOUND,10);
				}  
				
			//	draw_text_len(18+FONT_W*15,24+FONT_H*i,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,10);
			}
		}
	}


	f_close(&f);
	
	memset(temp_msg, 0, sizeof(temp_msg));
    return true;
}
//==================================================================================
void Set_load_tape(char *filename,char *current_lfn)
{   
	seekbuf =0;
	strcpy(tape_file_name,filename);
	strncpy(TapeName, current_lfn, 16);
	enable_tape = true;//// tap файл подключен разрешение на перехват в пзу
}
//==================================================================================
void load_to_zx(uint16_t adr, uint16_t len)
{

  //  int tfd =-1; //tape file descriptor
	
	while (1)
	{
		//tfd = 
		f_read(&f, sd_buffer, 512, &bytesRead);
		for (uint16_t i = 0; i < 512; i++)
		{
			z1->cpu.write  (z1, adr, sd_buffer[i]);
			adr++;
			len--;
			if (len == 0)
				return;
		}
	}
}

//--------------------------------------------------------------------------------------------------
		void tape_load_056a(void)
	{
		f_open(&f, tape_file_name, FA_READ);
		uint16_t x;
		uint16_t adr_s = Z80_IX(z1->cpu); // адрес загрузки в spectrum				   
		uint16_t len_s = Z80_DE(z1->cpu); // длина в DE

		x = sd_buffer[1];
		uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина блока +0 +1

		f_lseek(&f, seekbuf); // смещаемся в файле на seekbuf байт
 
		f_read(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта
		
		lenBlk = (sd_buffer[1] << 8) | sd_buffer[0]; // длина блока

		load_to_zx(adr_s, len_s);

		seekbuf = seekbuf + lenBlk + 2;
		
		Z80_F(z1->cpu)= 1;		 //	cpu.zf = 1;	 // ошибка
		
		Z80_PC(z1->cpu) = 0x0555; // ret c9 там
		Z80_IX(z1->cpu) = Z80_IX(z1->cpu) + len_s;

		return;
	}
//---------------------------------------------------------------------------------------------------
	void tape_load_0562(void)
	{
	//	FIL f;
       // int tfd =-1; //tape file descriptor
		//   seekbuf = 0;

		//printf("tape_load_0562: %d\n", seekbuf);
		//tfd = 
		f_open(&f, tape_file_name, FA_READ);
		//	tfd = f_open(&f, "0:/TEST_TAP.TAP ", FA_READ);

		uint16_t x;
		uint16_t adr_s = Z80_IX(z1->cpu); // адрес загрузки в spectrum
		uint16_t len_s = Z80_DE(z1->cpu); // длина в DE
		/* x = Z80_D(z1->cpu) ;						   
		uint16_t len_s = (x << 8) | Z80_E(z1->cpu); // длина */

		//x = sd_buffer[1];
		uint16_t lenBlk = (sd_buffer[1] << 8) | sd_buffer[0]; // длина блока

		f_lseek(&f, seekbuf); // смещаемся в файле на seekbuf байт

		//tfd =
		 f_read(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта
		x = sd_buffer[1];
		lenBlk = (sd_buffer[1] << 8) | sd_buffer[0]; // длина блока

		// seekbuf = seekbuf + lenBlk + 2;
		//	printf("DATA bank:%x adr:%x len:%x seekbuf2: %d\n", zx_RAM_bank_active, adr_s, len_s, seekbuf);

		load_to_zx(adr_s, len_s);

		seekbuf = seekbuf + lenBlk + 2;
		
		Z80_F(z1->cpu) = 1;		 // ошибка
		Z80_PC(z1->cpu) = 0x0555; // ret c9 там
		Z80_IX(z1->cpu) = Z80_IX(z1->cpu) + len_s;
		return;
	}

	void tape_load(void)
	{
		
	//	FIL f;
       // int tfd =-1; //tape file descriptor
		//printf("seekbuf1: %d\n", seekbuf);
		//tfd =
		 f_open(&f, tape_file_name, FA_READ);
		
		if (Z80_A(z1->cpu) == 0) // то заголовок
		{
			// чтение заголовка
			uint16_t x;
			uint16_t adr_s = Z80_IX(z1->cpu); // адрес загрузки в spectrum
			uint16_t len_s = Z80_DE(z1->cpu); // длина в DE

											   // изначально  seekbuf = 0;
			f_lseek(&f, seekbuf);		   // смещаемся в файле на seekbuf байт

			 f_read(&f, sd_buffer, 20, &bytesRead); // считывание 20 байт заголовка
			uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина заголовка 17+2 длина + 1 тип + 1 кс

			seekbuf = seekbuf + lenBlk + 2;

			//printf("HEADER seekbuf2: %d\n", seekbuf);
			// 0..1 Длина блока
			// 2    Флаговый байт (0 для заголовка, 255 для тела файла)
			// 3..xx  Эти байты представляют собой данные самого блока - либо заголовка, либо основного текста
			//  last  Байт контрольной суммы

			for (uint8_t i = 0; i < 17; i++)
			{
				z1->cpu.write   (z1, adr_s + i, sd_buffer[i + 3]);
				
			}

			Z80_F(z1->cpu) = 1;		 // ошибка
			Z80_PC(z1->cpu) = 0x0555; // ret c9 там
			Z80_IX(z1->cpu) = Z80_IX(z1->cpu) + len_s;

			return;
		}
		// uint8_t s = (Z80_A(z1->cpu) & 0x01);
		// if (s == 0x01) // то данные

		else
		{
			uint16_t x;
			uint16_t adr_s = Z80_IX(z1->cpu); // адрес загрузки в spectrum
			uint16_t len_s = Z80_DE(z1->cpu); // длина в DE

			//x = sd_buffer[1];
			uint16_t lenBlk = (sd_buffer[1] << 8) | sd_buffer[0]; // длина блока

			f_lseek(&f, seekbuf); // смещаемся в файле на seekbuf байт

			//tfd = 
			f_read(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта

			lenBlk = (sd_buffer[1] << 8) | sd_buffer[0]; // длина блока

			load_to_zx(adr_s, len_s);

			seekbuf = seekbuf + lenBlk + 2;

			Z80_F(z1->cpu) = 1;		 // ошибка
			Z80_PC(z1->cpu) = 0x0555; // ret c9 там
			Z80_IX(z1->cpu) = Z80_IX(z1->cpu) + len_s;
			return;
		}
	}
//#############################################################################
int tape_file_status =-1; //tape file descriptor
uint8_t blockChecksum=0;
size_t bytesRead;
size_t bytesToRead;



static uint8_t tapePhase;
static uint64_t tapeStart;
static uint32_t tapePulseCount;
static uint16_t tapeBitPulseLen;   
static uint8_t tapeBitPulseCount;	 
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t* tape;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask; 


// ============================================================================
// Функция чтения состояния ленты (эмуляция аудио сигнала с кассеты)
// Вызывается эмулятором при каждой инструкции IN для получения бита EAR
// ============================================================================
uint8_t __not_in_flash_func(TAP_Read)(){
#ifndef DEBUG_DISABLE_LOADERS
	// Проверяем, активна ли загрузка с ленты
	if(TapeStatus!=TAPE_LOADING) return false;
	
	// Получаем количество тактов, прошедших с начала текущего импульса
	uint64_t tapeCurrent = 4;//dt_cpu - tapeStart;
	
	// Обработка различных фаз аудио сигнала
	switch (tapePhase) {
	
	// ФАЗА 1: Длинные синхроимпульсы (Pilot Tone)
	// В реальной кассете - непрерывный тон длительностью ~2-3 секунды
	case TAPE_PHASE_SYNC:
		if (tapeCurrent > TAPE_SYNC_LEN) {  // Длительность одного импульса ~2168 тактов
			tapeStart=dt_cpu;
			tapeEarBit ^= 1;                // Инверсия сигнала - создаём прямоугольный импульс
			tapePulseCount++;
			if (tapePulseCount>tapeHdrPulses) { // Завершили серию синхроимпульсов
				tapePulseCount=0;
				tapePhase=TAPE_PHASE_SYNC1;     // Переход к первому синхроимпульсу данных
			}
		}
		break;
		
	// ФАЗА 2: Первый короткий синхроимпульс (Sync Pulse 1)
	// Маркер начала блока данных - 667 тактов
	case TAPE_PHASE_SYNC1:
		if (tapeCurrent > TAPE_SYNC1_LEN) {
			tapeStart=dt_cpu;
			tapeEarBit ^= 1;                // Завершаем первый синхроимпульс
			tapePhase=TAPE_PHASE_SYNC2;     // Переход ко второму синхроимпульсу
		}
		break;
		
	// ФАЗА 3: Второй короткий синхроимпульс (Sync Pulse 2)
	// Маркер начала блока данных - 735 тактов
	case TAPE_PHASE_SYNC2:
		if (tapeCurrent > TAPE_SYNC2_LEN) {
			tapeStart=dt_cpu;
			tapeEarBit ^= 1;                // Завершаем второй синхроимпульс
			// Определяем длительность импульса для первого бита данных
			if (tape[tapebufByteCount] & tapeBitMask) 
				tapeBitPulseLen=TAPE_BIT1_PULSELEN;   // 1710 тактов для "1"
			else 
				tapeBitPulseLen=TAPE_BIT0_PULSELEN;   // 855 тактов для "0"
			tapePhase=TAPE_PHASE_DATA;      // Переход к фазе передачи данных
		}
		break;
		
	// ФАЗА 4: Передача данных (Data Phase)
	// Каждый бит кодируется двумя импульсами разной длины
	case TAPE_PHASE_DATA:
		if (tapeCurrent >= tapeBitPulseLen) {
			tapeStart=dt_cpu;
			tapeEarBit ^= 1;                // Инверсия сигнала - завершаем текущий импульс
			tapeBitPulseCount++;
			
			// Каждый бит данных требует 2 импульса
			if (tapeBitPulseCount==2) {
				tapeBitPulseCount=0;
				// Переходим к следующему биту (циклический сдвиг маски)
				tapeBitMask = (tapeBitMask >>1 | tapeBitMask <<7);
				
				// Если обработали все 8 бит - переходим к следующему байту
				if (tapeBitMask==0x80) {
					tapebufByteCount++;      // Счётчик в текущем буфере
					tapeBlockByteCount++;    // Счётчик в текущем блоке
					tapeTotByteCount++;      // Общий счётчик в файле
					
					// Если буфер исчерпан - читаем следующую страницу с SD карты
					if(tapebufByteCount>=BUFF_PAGE_SIZE){
						im_z80_stop = true;  // Останавливаем эмуляцию Z80
					
					//	tape_file_status = sd_read_file(&sd_file,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
					
						im_z80_stop = false;
						if (tape_file_status!=FR_OK){

						//	sd_close_file(&sd_file);
							
							tap_loader_active = false;
							tapebufByteCount=0;
							TapeStatus=TAPE_STOPPED;
							return false;
						}
						tapebufByteCount=0;  // Сброс указателя буфера
					}
					
					// Проверка - достигли ли конца текущего блока
					if(tapeBlockByteCount==(tapeBlockLen-2)){
						tapeTotByteCount+=2;
						tapebufByteCount=0;
						tapePhase=TAPE_PHASE_PAUSE;  // Пауза между блоками
						tapeEarBit=false;            // Уровень тишины во время паузы
						break;
					}
					
					// Проверка - достигли ли конца файла
					if (tapeTotByteCount >= tapeFileSize){
						tapePhase=TAPE_PHASE_PAUSE;
						TapeStatus=TAPE_LOADED;      // Загрузка завершена
						return false;
					}
				}
				
				// Определяем длительность импульса для следующего бита
				if (tape[tapebufByteCount] & tapeBitMask) 
					tapeBitPulseLen=TAPE_BIT1_PULSELEN;
				else 
					tapeBitPulseLen=TAPE_BIT0_PULSELEN;
			}
		}
		break;
		
	// ФАЗА 5: Пауза между блоками (Pause)
	// В реальной кассете - тишина ~1-3 секунды между программами
	case TAPE_PHASE_PAUSE:
		if (tapeTotByteCount <= tapeFileSize) {
			if (tapeCurrent > TAPE_BLK_PAUSELEN) {
				// Переход к следующему блоку
				tapeCurrentBlock++;
				
			//	TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapeCurrentBlock];
			TapeBlock* tblock = (TapeBlock*)&sd_buffer[sizeof(TapeBlock)*tapeCurrentBlock];
			//	sd_seek_file(&sd_file,tblock->FPos);
				tapeBlockLen=tblock->Size + 2;

				bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
				
			//	tape_file_status = sd_read_file(&sd_file,sd_buffer,bytesToRead,&bytesRead);
				
				if (tape_file_status != FR_OK){
					
				//	sd_close_file(&sd_file);
					
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				tapeStart=dt_cpu;
				tapePulseCount=0;
				tapePhase=TAPE_PHASE_SYNC;          // Возврат к синхроимпульсам
				tapebufByteCount=2;
				tapeBlockByteCount=0;
				// Выбор длительности серии синхроимпульсов:
				// - Для заголовка (Flag=0) - длинная серия ~8063 импульсов
				// - Для данных (Flag≠0) - короткая серия ~3223 импульсов
				if (tblock->Flag) 
					tapeHdrPulses=TAPE_HDR_SHORT;
				else 
					tapeHdrPulses=TAPE_HDR_LONG;
			}
		}
		if (tapeTotByteCount >= tapeFileSize){
			TapeStatus=TAPE_LOADED;                 // Загрузка завершена
			tap_loader_active=TAPE_OFF;
			break;
		}
		return false;
	} 
  
	return tapeEarBit;  // Возвращаем текущее состояние сигнала EAR
#endif
}


// ============================================================================
// Управление воспроизведением ленты
// ============================================================================
void TAP_Play(){
#ifndef DEBUG_DISABLE_LOADERS
	switch (TapeStatus) {
	case TAPE_STOPPED:
		// Инициализация всех параметров аудио сигнала
	   	tapePhase=TAPE_PHASE_SYNC;        // Начинаем с синхроимпульсов
	   	tapePulseCount=0;                 // Счётчик импульсов
	   	tapeEarBit=false;                // Начальный уровень сигнала
	   	tapeBitMask=0x80;                // Начинаем со старшего бита
	   	tapeBitPulseCount=0;             // Счётчик полупериодов бита
	   	tapeBitPulseLen=TAPE_BIT0_PULSELEN;  // Начальная длительность
	   	tapeHdrPulses=TAPE_HDR_LONG;     // Длинная серия для заголовка

		//TapeBlock* tblock = (TapeBlock*)&temp_buffer_y[sizeof(TapeBlock)*tapeCurrentBlock];
		TapeBlock* tblock = (TapeBlock*)&sd_buffer[sizeof(TapeBlock)*tapeCurrentBlock];

	//	sd_seek_file(&sd_file,tblock->FPos);
		
		tapeTotByteCount = tblock->FPos;
		tapeBlockLen=tblock->Size + 2;
		bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
		
	//	tape_file_status = sd_read_file(&sd_file,sd_buffer,bytesToRead,&bytesRead);
	   	
		tapebufByteCount=2;
		tapeBlockByteCount=0;
	   	tapeStart=dt_cpu;               // Запоминаем время начала импульса
	   	TapeStatus=TAPE_LOADING;         // Активируем загрузку
	   	break;
	   
	case TAPE_LOADING:
	   	TapeStatus=TAPE_PAUSED;          // Приостановка загрузки
	   	break;
	   
	case TAPE_PAUSED:
		tapeStart=dt_cpu;               // Продолжение загрузки
		TapeStatus=TAPE_LOADING;
		break;
		
	case TAPE_LOADED:
		return;
		break;
	}
#endif
}


// ============================================================================
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
#include "aySoft.h"
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
int tfd =-1; //tape file descriptor
size_t bytesRead;
size_t bytesToRead;
uint8_t tapBlocksCount=0;
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
char tapeBHbuffer[20]; //tape block header buffer
bool tap_loader_active;
static uint8_t* tape;
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
*/
// ============================================================================
// Активные функции управления TAP (раскомментированы из старого блока)
// ============================================================================

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

	// Используем только вторую половину sd_buffer [0x2000..0x3FFF] —
	// первая половина [0..BUFF_PAGE_SIZE-1] содержит активную страницу ленты.
	bufferOut = &sd_buffer[0x2000];
	memset(bufferOut, 0, sizeof(sd_buffer) - 0x2000);

	// Локальные переменные — глобальные FIL f и tap_blocks[] не трогаем
	FIL f_prev;
	size_t lBR;
	size_t lFileSize;
	uint32_t lTotByteCount;
	uint8_t lBlocksCount;
	char lBHbuffer[20];
	static TapeBlock lBlocks[TAPE_BLK_SIZE];

	for(uint8_t i=0;i<TAPE_BLK_SIZE;i++){
		lBlocks[i].DataType=0;
		lBlocks[i].Flag=0;
		lBlocks[i].FPos=0;
		lBlocks[i].Size=0;
		lBlocks[i].NAME[0]=0;
	}

	int tfd = f_open(&f_prev, file_name, FA_READ);
	if (tfd!=FR_OK){f_close(&f_prev);return false;}
	lFileSize = sd_file_size(&f_prev);
	lBlocksCount=0;
	lTotByteCount=0;
	while (lTotByteCount<=lFileSize){
		tfd = f_read(&f_prev,lBHbuffer,14,&lBR);
		if (tfd != FR_OK){f_close(&f_prev);return false;}
		TapeBlock* block = (TapeBlock*) &lBHbuffer;
		lBlocks[lBlocksCount].Size = block->Size;
		memset(lBlocks[lBlocksCount].NAME, 0, sizeof(lBlocks[lBlocksCount].NAME));
		if (block->Flag==0){
			memcpy(lBlocks[lBlocksCount].NAME,block->NAME,10);
		}
		lBlocks[lBlocksCount].Flag = block->Flag;
		lBlocks[lBlocksCount].DataType = block->DataType;
		lBlocks[lBlocksCount].FPos=lTotByteCount;
		lTotByteCount+=block->Size+2;
		f_lseek(&f_prev,lTotByteCount);
		lBlocksCount++;
		if(lTotByteCount>=lFileSize){
			break;
		}
		if(lBlocksCount==TAPE_BLK_SIZE){
			break;
		}
	}

	for (int8_t i=0;i<lBlocksCount;i++){
		if (lBlocks[i].Flag>0){
			if((lBlocks[i].Size>=0x1AFE)&&(lBlocks[i].Size<=0x1B02)){
				f_lseek(&f_prev,lBlocks[i].FPos);
				memset(bufferOut, 0, sizeof(sd_buffer) - 0x2000);
				tfd = f_read(&f_prev,bufferOut,lBlocks[i].Size,&lBR);
	        	if (tfd!=FR_OK){f_close(&f_prev);return false;}
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
			if (lBlocks[i].Size>0){
				if (lBlocks[i].Flag==0){
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg,"%s",lBlocks[i].NAME);
					draw_text_len(10+FONT_W*15,62+FONT_H*(j),temp_msg,CL_GRAY,COLOR_BACKGOUND,10);
					j++;
				}
 				else
				{
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg," %d",(lBlocks[i].Size));
					draw_text_len(90+FONT_W*15,62+FONT_H*(j-1),temp_msg,CL_LT_CYAN,COLOR_BACKGOUND,10);
				}
			}
		}
	}

	f_close(&f_prev);
	memset(temp_msg, 0, sizeof(temp_msg));
    return true;
}
//==================================================================================
void Set_load_tape(char *filename,char *current_lfn)
{
	seekbuf =0;
	strcpy(tape_file_name,filename);
	strncpy(TapeName, current_lfn, 16);
	TAP_Load(tape_file_name); // парсим блоки TAP файла

	if (conf.tape_mode == 0) {
		// Быстрая загрузка через ROM-трапы
		enable_tape = true;
		tap_loader_active = false;
	} else {
		// Медленная загрузка с полосками и звуком
		enable_tape = false;
		tap_loader_active = true;
		// TAP_Play() не вызываем здесь - старт по первому обращению к порту
	}
}
//==================================================================================
// Переключение режима загрузки на горячую
void TAP_SwitchMode(void)
{
	TapeStatus = TAPE_STOPPED;
	if (tape_file_name[0] == 0) return; // файл не выбран

	if (conf.tape_mode == 0) {
		enable_tape = true;
		tap_loader_active = false;
	} else {
		enable_tape = false;
		tap_loader_active = true;
	}
	seekbuf = 0;
	TAP_Rewind();
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

static uint8_t tapePhase;
static uint64_t tapeStart;
static uint64_t tapeLastCycle;   // tape_cycle_count при последнем вызове TAP_Read
static uint32_t tapePulseCount;
static uint16_t tapeBitPulseLen;
static uint8_t tapeBitPulseCount;
static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask;
// Позиция и размер текущей страницы sd_buffer в TAP-файле — для восстановления после меню
static uint32_t tape_page_fpos = 0;
static size_t   tape_page_size = 0;


// ============================================================================
// Функция чтения состояния ленты (эмуляция аудио сигнала с кассеты)
// Вызывается эмулятором при каждой инструкции IN для получения бита EAR
// Тайминги привязаны к глобальному счётчику тактов Z80 (tape_cycle_count)
// ============================================================================
uint8_t __not_in_flash_func(TAP_Read)(){
#ifndef DEBUG_DISABLE_LOADERS
	if(TapeStatus!=TAPE_LOADING) return false;

	uint64_t tapeCurrent = tape_cycle_count - tapeStart;

	// Если gap слишком большой и не в паузе — не генерируем сигнал, просто синхронизируемся
	if (tapeCurrent > 50000 && tapePhase != TAPE_PHASE_PAUSE) {
		tapeStart = tape_cycle_count;
		return tapeEarBit;
	}

	switch (tapePhase) {

	case TAPE_PHASE_SYNC:
		if (tapeCurrent > TAPE_SYNC_LEN) {
			tapeStart=tape_cycle_count;
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
			tapeStart=tape_cycle_count;
			tapeEarBit ^= 1;
			tapePhase=TAPE_PHASE_SYNC2;
		}
		break;

	case TAPE_PHASE_SYNC2:
		if (tapeCurrent > TAPE_SYNC2_LEN) {
			tapeStart=tape_cycle_count;
			tapeEarBit ^= 1;
			if (tape[tapebufByteCount] & tapeBitMask)
				tapeBitPulseLen=TAPE_BIT1_PULSELEN;
			else
				tapeBitPulseLen=TAPE_BIT0_PULSELEN;
			tapePhase=TAPE_PHASE_DATA;
		}
		break;

	case TAPE_PHASE_DATA:
		if (tapeCurrent > tapeBitPulseLen) {
			tapeStart=tape_cycle_count;
			tapeEarBit ^= 1;
			tapeBitPulseCount++;
			if (tapeBitPulseCount==2) {
				tapeBitPulseCount=0;
				tapeBitMask = (tapeBitMask >>1 | tapeBitMask <<7);
				if (tapeBitMask==0x80) {
					tapebufByteCount++;
					tapeBlockByteCount++;
					tapeTotByteCount++;

					// Буфер исчерпан - читаем следующую страницу с SD
					if(tapebufByteCount>=BUFF_PAGE_SIZE){
						im_z80_stop = true;
						tape_page_fpos = f_tell(&f);
						tape_page_size = BUFF_PAGE_SIZE;
						int tfd = f_read(&f,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
						im_z80_stop = false;
						if (tfd!=FR_OK){
							f_close(&f);
							tap_loader_active = false;
							tapebufByteCount=0;
							TapeStatus=TAPE_STOPPED;
							return false;
						}
						tapebufByteCount=0;
					}

					// Конец текущего блока
					if(tapeBlockByteCount==(tapeBlockLen-2)){
						tapeTotByteCount+=2;
						tapebufByteCount=0;
						tapePhase=TAPE_PHASE_PAUSE;
						tapeEarBit=false;
						if (tapeTotByteCount >= tapeFileSize){
							TapeStatus=TAPE_STOPPED;
							TAP_Rewind();
							tap_loader_active = false;
							return false;
						}
						break;
					}
				}
				if (tape[tapebufByteCount] & tapeBitMask)
					tapeBitPulseLen=TAPE_BIT1_PULSELEN;
				else
					tapeBitPulseLen=TAPE_BIT0_PULSELEN;
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
				tape_page_fpos = tap_blocks[tapeCurrentBlock].FPos;
				tape_page_size = bytesToRead;
				int tfd = f_read(&f,sd_buffer,bytesToRead,&bytesRead);
				if (tfd != FR_OK){
					f_close(&f);
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				tapeStart=tape_cycle_count;
				tapePulseCount=0;
				tapePhase=TAPE_PHASE_SYNC;
				tapebufByteCount=2;
				tapeBlockByteCount=0;
				if (tap_blocks[tapeCurrentBlock].Flag)
					tapeHdrPulses=TAPE_HDR_SHORT;
				else
					tapeHdrPulses=TAPE_HDR_LONG;
			}
		} else {
			TapeStatus=TAPE_STOPPED;
			TAP_Rewind();
			tap_loader_active = false;
			break;
		}
		return false;
	}

	return tapeEarBit;
#endif
}


// ============================================================================
// Управление воспроизведением ленты
// ============================================================================
void __not_in_flash_func(TAP_Play)(){
#ifndef DEBUG_DISABLE_LOADERS
	switch (TapeStatus) {
	case TAPE_STOPPED:
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
		tape_page_fpos = tap_blocks[tapeCurrentBlock].FPos;
		tape_page_size = bytesToRead;
		int tfd = f_read(&f,sd_buffer,bytesToRead,&bytesRead);
		if (tfd != FR_OK){f_close(&f);break;}
	   	tapebufByteCount=2;
		tapeBlockByteCount=0;
		tapeLastCycle=0;
	   	tapeStart=tape_cycle_count;
	   	TapeStatus=TAPE_LOADING;
	   	break;
	case TAPE_LOADING:
	   	TapeStatus=TAPE_PAUSED;
	   	break;
	case TAPE_PAUSED:
		TapeStatus=TAPE_LOADING;
		break;
	}
#endif
}

// ============================================================================
// Восстановление страницы sd_buffer после операций меню/SD, которые её затёрли.
// Вызывать из SpeccyP.c после возврата из file_manager, save_slot, load_slot.
// ============================================================================
void TAP_RestorePage(void) {
	if (!tap_loader_active || TapeStatus != TAPE_LOADING) return;
	if (tape_page_size == 0) return;
	f_lseek(&f, tape_page_fpos);
	f_read(&f, sd_buffer, tape_page_size, &bytesRead);
	// После f_read указатель файла стоит на tape_page_fpos + tape_page_size —
	// это корректная позиция для чтения следующей страницы.
}

// ============================================================================

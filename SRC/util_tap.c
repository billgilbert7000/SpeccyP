#include "util_tap.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include <zx_emu/z80.h>
#include "pico/stdlib.h"
#include <ps2.h>
#include "zx_emu/zx_machine.h"
#include "screen_util.h"
#include <math.h>
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>
#include "config.h"
//#define ZX_RAM_PAGE_SIZE 0x4000
#define BUFF_PAGE_SIZE 0x200

extern volatile z80 cpu;
extern uint8_t RAM[ZX_RAM_PAGE_SIZE*8]; //Реальная память куском 128Кб
//extern char sd_buffer[SD_BUFFER_SIZE];
extern char temp_msg[60];

#include "zx_emu/zx_machine.h"
#include "screen_util.h"



/*
typedef struct TapeBlock{
	uint16_t Size;
	uint8_t Flag;
	uint8_t DataType;
	char NAME[11];
	uint32_t FPos;
} __attribute__((packed)) TapeBlock;
*/

uint8_t tapBlocksCount=0;

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
static uint32_t tapebufByteCount;
static uint32_t tapeBlockByteCount;
static uint16_t tapeHdrPulses;
static uint32_t tapeBlockLen;
static uint8_t* tape;
static uint8_t tapeEarBit;
static uint8_t tapeBitMask; 

uint8_t tapeCurrentBlock;
size_t tapeFileSize;
uint32_t tapeTotByteCount;

char tapeBHbuffer[20]; //tape block header buffer


uint8_t __not_in_flash_func(TAP_Read)(){
//uint8_t TAP_Read(){
    uint64_t tapeCurrent = cpu.cyc - tapeStart;
    ////printf("Tape PHASE:%X\n",tapePhase);
    switch (tapePhase) {
    case TAPE_PHASE_SYNC:
        if (tapeCurrent > TAPE_SYNC_LEN) {
            tapeStart=cpu.cyc;
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
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            tapePhase=TAPE_PHASE_SYNC2;
        }
        break;
    case TAPE_PHASE_SYNC2:
        if (tapeCurrent > TAPE_SYNC2_LEN) {
            tapeStart=cpu.cyc;
            tapeEarBit ^= 1;
            if (tape[tapebufByteCount] & tapeBitMask) tapeBitPulseLen=TAPE_BIT1_PULSELEN; else tapeBitPulseLen=TAPE_BIT0_PULSELEN;            
            tapePhase=TAPE_PHASE_DATA;
        }
        break;
    case TAPE_PHASE_DATA:
        if (tapeCurrent > tapeBitPulseLen) {
            tapeStart=cpu.cyc;
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
						tfd = sd_read_file(&f,sd_buffer,BUFF_PAGE_SIZE,&bytesRead);
                    	//printf("bytesRead=%d\n",bytesRead);
		                if (tfd!=FR_OK){
							//printf("Error read SD\n");
							sd_close_file(&f);
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
				sd_seek_file(&f,tap_blocks[tapeCurrentBlock].FPos);
				tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
				bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
				tfd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);
				if (tfd != FR_OK){
					//printf("Error read SD\n");
					sd_close_file(&f);
					tap_loader_active = false;
					tapebufByteCount=0;
					TapeStatus=TAPE_STOPPED;
					return false;
				}
				//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);
                tapeStart=cpu.cyc;
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
		sd_seek_file(&f,tap_blocks[tapeCurrentBlock].FPos);
		tapeTotByteCount = tap_blocks[tapeCurrentBlock].FPos;
		tapeBlockLen=tap_blocks[tapeCurrentBlock].Size + 2;
		bytesToRead = tapeBlockLen<BUFF_PAGE_SIZE ? tapeBlockLen : BUFF_PAGE_SIZE;
		tfd = sd_read_file(&f,sd_buffer,bytesToRead,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);break;}
		//printf("Block:%d Seek:%d Length:%d Read:%d\n",tapeCurrentBlock,tap_blocks[tapeCurrentBlock].FPos,tapeBlockLen,bytesRead);		
       	tapebufByteCount=2;
		tapeBlockByteCount=0;
       	tapeStart=cpu.cyc;
       	TapeStatus=TAPE_LOADING;
       	break;
    case TAPE_LOADING:
       	TapeStatus=TAPE_PAUSED;
       	break;
    case TAPE_PAUSED:
        tapeStart=cpu.cyc;
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
	tfd = sd_open_file(&f,file_name,FA_READ);
    //printf("sd_open_file=%d\n",tfd);
	if (tfd!=FR_OK){sd_close_file(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    //printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = sd_read_file(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);return false;}
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
		sd_seek_file(&f,tapeTotByteCount);
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
	sd_seek_file(&f,0);
	tape = (uint8_t *)&sd_buffer;//error

	/*for(uint8_t j=0;j<tapBlocksCount;j++){
		//printf(" block:%d, size:%d \n",j,tap_blocks[j].Size);
	}*/


	
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

	memset(sd_buffer, 0, sizeof(sd_buffer));

	for(uint8_t i=0;i<TAPE_BLK_SIZE;i++){
		tap_blocks[i].DataType=0;
		tap_blocks[i].Flag=0;
		tap_blocks[i].FPos=0;
		tap_blocks[i].Size=0;
		tap_blocks[i].NAME[0]=0;
	}
	

	tfd = sd_open_file(&f,file_name,FA_READ);
    //printf("sd_open_file=%d\n",tfd);
	if (tfd!=FR_OK){sd_close_file(&f);return false;}
   	tapeFileSize = sd_file_size(&f);
    //printf(".TAP Filesize %u bytes\n", tapeFileSize);
	tapBlocksCount=0;
	tapebufByteCount=0;
	tapeBlockByteCount=0;
	tapeTotByteCount=0;
	while (tapeTotByteCount<=tapeFileSize){
		tfd = sd_read_file(&f,tapeBHbuffer,14,&bytesRead);
		if (tfd != FR_OK){sd_close_file(&f);return false;}
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
		sd_seek_file(&f,tapeTotByteCount);
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
	sd_seek_file(&f,0);

	for (int8_t i=0;i<tapBlocksCount;i++){
		if (tap_blocks[i].Flag>0){
			if((tap_blocks[i].Size>=0x1AFE)&&(tap_blocks[i].Size<=0x1B02)){
				sd_seek_file(&f,tap_blocks[i].FPos);
				bufferOut = (SD_BUFFER_SIZE>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
				memset(sd_buffer, 0, sizeof(sd_buffer));
				tfd = sd_read_file(&f,bufferOut,tap_blocks[i].Size,&bytesRead);
	        	//printf("bytesRead=%d\n",bytesRead);
	        	if (tfd!=FR_OK){sd_close_file(&f);return false;}
		        ShowScreenshot(bufferOut);
				screen_found = true;
				break;
			}
		}			
	}

	if (!screen_found){
		draw_text_len(18+FONT_W*14,42,"File TAPE:",COLOR_TEXT,COLOR_BACKGOUND,14);
		uint8_t j =1;
		for (uint8_t i = 0; i < 22; i++){
			if (tap_blocks[i].Size>0){
				if (tap_blocks[i].Flag==0){
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg,"%s",tap_blocks[i].NAME);
					draw_text_len(10+FONT_W*15,42+FONT_H*(j),temp_msg,CL_GRAY,COLOR_BACKGOUND,10);
					j++;
				} 
				
 				else 
				{
					memset(temp_msg, 0, sizeof(temp_msg));
					sprintf(temp_msg," %d",(tap_blocks[i].Size));
					draw_text_len(90+FONT_W*15,42+FONT_H*(j-1),temp_msg,CL_LT_CYAN,COLOR_BACKGOUND,10);
				}  
				
			//	draw_text_len(18+FONT_W*15,24+FONT_H*i,temp_msg,COLOR_TEXT,COLOR_BACKGOUND,10);
			}
		}
	}
   // memset(temp_msg, 0, sizeof(temp_msg));
//	sprintf(temp_msg,"Type:.TAP");
//	draw_text_len(18+FONT_W*14,208, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
//	memset(temp_msg, 0, sizeof(temp_msg));
//	sprintf(temp_msg,"Size: %dKb",sd_file_size(&f)/1024);
	//draw_text_len(18+FONT_W*14,216, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);
//	memset(temp_msg, 0, sizeof(temp_msg));
	//strncpy(file_name,"A:/",3);
	//strncpy(temp_msg,file_name,22);
	//draw_text_len(18+FONT_W*14,224, temp_msg,COLOR_TEXT,COLOR_BACKGOUND,22);

	sd_close_file(&f);
	//printf("\nAll loaded\n");
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
	while (1)
	{
		tfd = sd_read_file(&f, sd_buffer, 512, &bytesRead);
		for (uint16_t i = 0; i < 512; i++)
		{
			cpu.write_byte(NULL, adr, sd_buffer[i]);
			adr++;
			len--;
			//	   printf ("len: %x\n",len );
			if (len == 0)
				return;
		}
	}
}
	//-------------------------
	// printf("sd_open_file=%d\n",tfd);
	//if (tfd!=FR_OK){sd_close_file(&f);return false;}
//--------------------------------------------------------------------------------------------------
		void tape_load_056a(void)
	{
		
		//   seekbuf = 0;

		//printf("tape_load_0562: %d\n", seekbuf);
		tfd = sd_open_file(&f, tape_file_name, FA_READ);
		//	tfd = sd_open_file(&f, "0:/TEST_TAP.TAP ", FA_READ);

		uint16_t x;
		uint16_t adr_s = cpu.ix; // адрес загрузки в spectrum
		x = cpu.d;						   
		uint16_t len_s = (x << 8) | cpu.e; // длина

		x = sd_buffer[1];
		uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина блока

		sd_seek_file(&f, seekbuf); // смещаемся в файле на seekbuf байт

		tfd = sd_read_file(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта
		x = sd_buffer[1];
		lenBlk = (x << 8) | sd_buffer[0]; // длина блока

		// seekbuf = seekbuf + lenBlk + 2;
		//	printf("DATA bank:%x adr:%x len:%x seekbuf2: %d\n", zx_RAM_bank_active, adr_s, len_s, seekbuf);

		load_to_zx(adr_s, len_s);

		seekbuf = seekbuf + lenBlk + 2;
		//   seekbuf = seekbuf + len_s + 2;

		cpu.cf = 1;		 // ошибка
		cpu.zf = 1;		 // ошибка
		cpu.pc = 0x0555; // ret c9 там
		cpu.ix = cpu.ix + len_s;
		//      sd_close_file(&f);
		return;
	}
//---------------------------------------------------------------------------------------------------
	void tape_load_0562(void)
	{
		
		//   seekbuf = 0;

		//printf("tape_load_0562: %d\n", seekbuf);
		tfd = sd_open_file(&f, tape_file_name, FA_READ);
		//	tfd = sd_open_file(&f, "0:/TEST_TAP.TAP ", FA_READ);

		uint16_t x;
		uint16_t adr_s = cpu.ix; // адрес загрузки в spectrum
		x = cpu.d;						   
		uint16_t len_s = (x << 8) | cpu.e; // длина

		x = sd_buffer[1];
		uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина блока

		sd_seek_file(&f, seekbuf); // смещаемся в файле на seekbuf байт

		tfd = sd_read_file(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта
		x = sd_buffer[1];
		lenBlk = (x << 8) | sd_buffer[0]; // длина блока

		// seekbuf = seekbuf + lenBlk + 2;
		//	printf("DATA bank:%x adr:%x len:%x seekbuf2: %d\n", zx_RAM_bank_active, adr_s, len_s, seekbuf);

		load_to_zx(adr_s, len_s);

		seekbuf = seekbuf + lenBlk + 2;
		//   seekbuf = seekbuf + len_s + 2;

		cpu.cf = 1;		 // ошибка
		cpu.pc = 0x0555; // ret c9 там
		cpu.ix = cpu.ix + len_s;
		//      sd_close_file(&f);
		return;
	}

	void tape_load(void)
	{
		//printf("seekbuf1: %d\n", seekbuf);
		tfd = sd_open_file(&f, tape_file_name, FA_READ);
		//	tfd = sd_open_file(&f, "0:/TEST_TAP.TAP ", FA_READ);

		if (cpu.a == 0) // то заголовок
		{
			// чтение заголовка
			uint16_t x;
			uint16_t adr_s = cpu.ix; // адрес загрузки в spectrum
			x = cpu.d;
			uint16_t len_s = (x << 8) | cpu.e; // длина
											   // изначально  seekbuf = 0;
			sd_seek_file(&f, seekbuf);		   // смещаемся в файле на seekbuf байт

			tfd = sd_read_file(&f, sd_buffer, 20, &bytesRead); // считывание 20 байт заголовка
			x = sd_buffer[1];
			uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина заголовка 17+2 длина + 1 тип + 1 кс

			seekbuf = seekbuf + lenBlk + 2;

			//printf("HEADER seekbuf2: %d\n", seekbuf);
			// 0..1 Длина блока
			// 2    Флаговый байт (0 для заголовка, 255 для тела файла)
			// 3..xx  Эти байты представляют собой данные самого блока - либо заголовка, либо основного текста
			//  last  Байт контрольной суммы

			for (uint8_t i = 0; i < 17; i++)
			{
				cpu.write_byte(NULL, adr_s + i, sd_buffer[i + 3]);
			}

			cpu.cf = 1;		 // ошибка
			cpu.pc = 0x0555; // ret c9 там
			cpu.ix = cpu.ix + len_s;
			//     sd_close_file(&f);
			return;
		}
		// uint8_t s = (cpu.a & 0x01);
		// if (s == 0x01) // то данные

		else
		{
			uint16_t x;
			uint16_t adr_s = cpu.ix; // адрес загрузки в spectrum
			x = cpu.d;
			uint16_t len_s = (x << 8) | cpu.e; // длина
			x = sd_buffer[1];
			uint16_t lenBlk = (x << 8) | sd_buffer[0]; // длина блока

			sd_seek_file(&f, seekbuf); // смещаемся в файле на seekbuf байт

			tfd = sd_read_file(&f, sd_buffer, 3, &bytesRead); // считываем 3 байта
			x = sd_buffer[1];
			lenBlk = (x << 8) | sd_buffer[0]; // длина блока

			// seekbuf = seekbuf + lenBlk + 2;
			//	//printf("DATA bank:%x adr:%x len:%x seekbuf2: %d\n", zx_RAM_bank_active, adr_s, len_s, seekbuf);

			load_to_zx(adr_s, len_s);

			seekbuf = seekbuf + lenBlk + 2;

			cpu.cf = 1;		 // ошибка
			cpu.pc = 0x0555; // ret c9 там
			cpu.ix = cpu.ix + len_s;
			//    sd_close_file(&f);
			return;
		}
	}

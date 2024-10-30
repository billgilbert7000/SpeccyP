#include "util_z80.h"
#include "util_sd.h"
#include <VFS.h>
#include <fcntl.h>
#include <string.h>
#include <zx_emu/z80.h>
//#include "zx_emu/aySoundSoft.h"
#include "zx_emu/zx_machine.h"
#include "screen_util.h"

//#define DUMP_PAGES_V1
//#define DUMP_PAGES_V2
//#define DUMP_UNPACK
//#define DUMP_EXPAND
//#define DUMP_RAW
//#define DUMP_BUFFER

extern uint8_t RAM[ZX_RAM_PAGE_SIZE*8]; //Реальная память куском 128Кб
extern z80 cpu;
extern uint8_t zx_RAM_bank_active;
extern uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
extern uint8_t* zx_ram_bank[8];
extern uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))
extern uint8_t* zx_video_ram;

extern bool zx_state_48k_MODE_BLOCK;
extern uint8_t zx_Border_color;

extern void zx_machine_set_7ffd_out(uint8_t val);
extern uint8_t zx_machine_get_7ffd_lastOut();

//extern char sd_buffer[SD_BUFFER_SIZE];
extern int last_error;

typedef struct FileHeader{
	uint8_t A; //0  A register
	uint8_t F; //1  F register
	uint8_t C; //2  BC register pair (LSB, i.e. C, first)
	uint8_t B; //3  BC register pair (LSB, i.e. C, first)
	uint8_t L; //4  HL register pair
	uint8_t H; //5  HL register pair
	uint16_t PC; //7<<6 PC (version 1) or 0 to signal a version 2 or 3
	uint16_t SP; //9<<8 Stack pointer
	uint8_t IR;  //10
	uint8_t RR;  //11
	uint8_t Flags1; //12
	uint8_t E; //13
	uint8_t D; //14
	uint8_t C_Dash; //15
	uint8_t B_Dash; //16
	uint8_t E_Dash; //17
	uint8_t D_Dash; //18
	uint8_t L_Dash; //19
	uint8_t H_Dash; //20
	uint8_t A_Dash; //21
	uint8_t F_Dash; //22
	uint16_t IY; //24<<23
	uint16_t IX; //26<<25
	uint8_t InterruptFlipFlop; //27
	uint8_t IFF2; //28
	uint8_t Flags2; //29
	uint16_t AdditionalBlockLength; //31<<30
	uint16_t PCVersion2; //32<<33
	uint8_t HardwareMode; //34
	uint8_t PagingState; //35
	uint8_t IF1RomPaged; //36
	uint8_t HWModeState; //37
	uint8_t LastOut_FFFD; //38
	uint8_t AY_Regs[16]; //39-55
} __attribute__((packed)) FileHeader;

typedef struct MemBlock{
	uint16_t Size; //1<<0
	uint8_t PageNum;//2
} __attribute__((packed)) MemBlock;


int fd;                     //индикатор ошибки чтения файла
char buf[10];               //временный буфер
//uint8_t  buf[10];               //временный буфер
char header_buf[87];        // буфер для чтения заголовка

void readAYState(FileHeader* header){
	for (uint8_t i=0;i<16;i++){
		AY_select_reg(i);
		AY_set_reg(header->AY_Regs[i]);
		//printf("GET AY[%02d]=%02X\n",i,header->AY_Regs[i]);
		// cpu._hi_addr_port = 0xFF;
		// cpu.port_out(&cpu,0xFD,i);
		// cpu._hi_addr_port = 0xBF;        
		// cpu.port_out(&cpu,0xFD,header[i+39]);
	}
	AY_select_reg(header->LastOut_FFFD);
}

void saveAYState(FileHeader* header){
	header->LastOut_FFFD = zx_machine_get_7ffd_lastOut();
	for (uint8_t i=0;i<16;i++){
		AY_select_reg(i);
		header->AY_Regs[i] = AY_get_reg();
		//printf("SET AY[%02d]=%02X\n",i,header->AY_Regs[i]);
	}	
}

void clearAYState(FileHeader* header){
	header->LastOut_FFFD = zx_machine_get_7ffd_lastOut();
	for (uint8_t i=0;i<16;i++){
		AY_select_reg(i);
		header->AY_Regs[i] = 0;
		//printf("SET AY[%02d]=%02X\n",i,header->AY_Regs[i]);
	}	
}

void readCPUstate(FileHeader* header,uint8_t im_ver,uint8_t im_hw){
	uint8_t last_out_7ffd;	
	// If byte 12 is 255, it has to be regarded as being 1
	if (header->Flags1 == 255){
		header->Flags1 = 1;
	}
	//printf("readCPUstate begin\n");
	
	cpu.a = header->A;
	
	cpu.sf = (header->F & 0b10000000);
	cpu.zf = (header->F & 0b01000000);
	cpu.yf = (header->F & 0b00100000);
	cpu.hf = (header->F & 0b00010000);
	cpu.xf = (header->F & 0b00001000);
	cpu.pf = (header->F & 0b00000100);
	cpu.nf = (header->F & 0b00000010);
	cpu.cf = (header->F & 0b00000001);
	
	cpu.b = header->B; cpu.c = header->C;
	cpu.h = header->H; cpu.l = header->L;
	
	cpu.sp = header->SP;
	cpu.i = header->IR;
	cpu.r = header->RR;
	cpu.r = cpu.r|(((header->Flags1&1)<<7)&0b10000000);
	
	zx_Border_color=(((header->Flags1>>1)&0x7)<<4)|((header->Flags1>>1)&0x7);//дублируем для 4 битного видеобуфера
	//printf("Border color: %02x -> %02x\n",header->Flags1,zx_Border_color);
	
	cpu.d   = header->D; cpu.e   = header->E;    
	
	cpu.b_  = header->B_Dash; cpu.c_  = header->C_Dash;	
	cpu.d_  = header->D_Dash; cpu.e_  = header->E_Dash;	
	cpu.h_  = header->H_Dash; cpu.l_  = header->L_Dash;
	
	cpu.a_  = header->A_Dash;
	cpu.f_  = header->F_Dash;
	
	
	cpu.ix  = header->IX;
	cpu.iy  = header->IY;
	
	cpu.iff1 = header->InterruptFlipFlop;
	cpu.iff2 = header->IFF2;
	
	cpu.pc = header->PC == 0 ? header->PCVersion2 : header->PC; //cpu.pc  = (header[7]<<8)|(header[6]);
	
	cpu.interrupt_mode  = (header->Flags2 & 0b00000011); //Биты 0-1: режим прерываний (0-2)
	//printf("cpu.interrupt_mode %d\n",cpu.interrupt_mode);
	cpu.int_pending = 0;
	
	if (im_ver == 1) 
		last_out_7ffd = 0b00110000;
	else 
		last_out_7ffd = header->PagingState;
	
	//printf("im_hw: %d \n",im_hw);
	//printf("last_out_7ffd: %02X\n", last_out_7ffd);
	
	if ((im_hw == 48) && (im_ver  > 1)) last_out_7ffd = 0x30;//last_out_7ffd | 0b00110000; // ??
	
	
	
	zx_RAM_bank_active = last_out_7ffd & 0x7;
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_active]; // 0xC000 - 0x7FFF 
	
	if (last_out_7ffd &  8) zx_video_ram=zx_ram_bank[7];  else zx_video_ram  = zx_ram_bank[5];
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//if (last_out_7ffd & 16) zx_cpu_ram[0]=zx_rom_bank[0]; else zx_cpu_ram[0] = zx_rom_bank[1]; //5bit = {1 - 48k[R0], 0 - 128k[R1]}
	if (last_out_7ffd & 16) zx_cpu_ram[0]=zx_rom_bank[1]; else zx_cpu_ram[0] = zx_rom_bank[0]; //5bit = {1 - 48k[R0], 0 - 128k[R1]}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//if ((last_out_7ffd & 32)&& (im_hw==48)) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block
	if (last_out_7ffd & 32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block // && (im_hw==48)
	
	//printf("last_out_7ffd modified: %02X\n", last_out_7ffd);
	zx_machine_set_7ffd_out(last_out_7ffd);
	//printf("readCPUstate end\n");
}

void saveCPUstate(FileHeader* header){

	//printf("saveCPUstate begin\n");

	memset(header_buf, 0, sizeof(header_buf));

	header->PC = 0;

	header->A = cpu.a;
	header->F = (cpu.sf << 7) | (cpu.zf << 6) | (cpu.yf << 5) | (cpu.hf << 4) | (cpu.xf << 3) | (cpu.pf << 2) | (cpu.nf << 1) | (cpu.cf);
	header->B = cpu.b; header->C = cpu.c;
	header->H = cpu.h; header->L = cpu.l;
	header->SP = cpu.sp;
	header->IR = cpu.i;
	header->RR = cpu.r;
	header->Flags1 = cpu.r>>7;
	header->Flags1 = header->Flags1 | ((zx_Border_color & 7)<<1);
	header->D = cpu.d;
	header->E = cpu.e;
	

	header->B_Dash = cpu.b_; header->C_Dash = cpu.c_;
	header->D_Dash = cpu.d_; header->E_Dash = cpu.e_;	
	header->H_Dash = cpu.h_; header->L_Dash = cpu.l_;
	
	header->A_Dash = cpu.a_;
	header->F_Dash = cpu.f_;
	
	
	header->IX = cpu.ix;
	header->IY = cpu.iy;
	
	header->InterruptFlipFlop = cpu.iff1;
	header->IFF2 = cpu.iff2;
	
	header->Flags2 = cpu.interrupt_mode; //Биты 0-1: режим прерываний (0-2)
	
	//header->AdditionalBlockLength = 54; //

	header->AdditionalBlockLength = 23; // Длина доп. блока для V2

	header->PCVersion2 = cpu.pc;
	
	
	header->PagingState = zx_RAM_bank_active; // zx_machine_last_out_7ffd;
	if (zx_state_48k_MODE_BLOCK)       header->PagingState = header->PagingState | 0b00100000;
	/////////////////////////////////////////////////////////////////////////////////////////////
	//if (zx_cpu_ram[0]==zx_rom_bank[0]) header->PagingState = header->PagingState | 0b00010000;
    if (zx_cpu_ram[0]==zx_rom_bank[1]) header->PagingState = header->PagingState | 0b00010000;
	/////////////////////////////////////////////////////////////////////////////////////////////
	if (zx_video_ram ==zx_ram_bank[7]) header->PagingState = header->PagingState | 0b00001000;
	
	header->IF1RomPaged = 0; //Contains 0xff if Interface I rom paged			If in Timex mode, contains last OUT to 0xff
	header->HWModeState = 0b00000100; //Bit 2: AY sound in use, even on 48K machines   Bit 7: Modify hardware	
	//printf("saveCPUstate end\n");
}

void GetPageInfo(uint8_t* buffer, uint8_t im_hw, uint8_t pagingState, int8_t* pageNumber, uint16_t* pageSize){
	*pageSize = buffer[0];
	*pageSize |= buffer[1] << 8;
	*pageNumber = buffer[2];
	
	////printf("GetPageInfo_1: pageNumber=%02X, pageSize=%02X, im_hw=%d, pagingState=%d\n",*pageNumber,*pageSize,im_hw,pagingState);
	
	if (im_hw==48){
		// 48K snapshot
		switch (*pageNumber){
			case 4:
			// 48K : 0x8000..0xBFFF
			*pageNumber = 2;
			break;
			case 5:
			// 48K : 0xC000..0xFFFF
			*pageNumber = 0;
			break;
			case 8:
			// 48K : 0x4000..0x7FFF
			*pageNumber = 5;
			break;
		}
		//printf("GetPageInfo: 48K pageNumber=%02X\n",*pageNumber);
		return;
		} /*else {
		// 128K snapshot
		*pageNumber -= 3;
	}*/
	//printf("GetPageInfo_2: pageNumber=%02X\n",*pageNumber);
	if (im_hw==128) {
		/*switch (*pageNumber){
			case 8:
			// 0x4000..0x7FFF
			*pageNumber = 5;
			break;
			case 4:
			// 0x8000..0xBFFF
			*pageNumber = 2;
			break;
			default:
			if (*pageNumber == (pagingState & 0x03) + 3){
			*pageNumber = 0; // 0xC000..0xFFFF
			}else{
			// skip it
			*pageNumber -= 3;
			}
			break;
		}*/
		*pageNumber -= 3;
		if (*pageNumber>7) *pageNumber=0xFF;
		//printf("GetPageInfo: 128K pageNumber=%02X\n",*pageNumber);
	}
}

uint16_t DecompressPage(uint8_t *page, uint16_t pageLength, bool isCompressed,	uint16_t maxSize, uint8_t* destMemory, uint16_t* destSize,uint8_t* base_page){
	uint8_t* memory = destMemory;
	uint16_t size = 0;	
	//for (int i = 0; i < (pageLength-4); i++){
	for (int i = 0; i < pageLength; i++){
		#ifdef DUMP_EXPAND
		if (i==0){
			printf("\nBegin\n");
			for (int j = 0; j < 16; j++){
				printf(" [%02X]",page[i+j]);
			}
			printf("\n");
		}
		#endif
		if (i < pageLength-4){
			if (page[i] == 0x00 && page[i + 1] == 0xED && page[i + 2] == 0xED && page[i + 3] == 0x00){
				#ifdef DUMP_UNPACK
					printf("\nUsed 1[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
				#endif
				*destSize = size-1;
				return i + 4;
			}
		}
		if (i < pageLength){
			if (isCompressed && page[i] == 0xED && page[i + 1] == 0xED){
				i += 2;
				int repeat = page[i++];
				uint8_t value = page[i];
				#ifdef DUMP_EXPAND
					printf("\nExpand: val[%02X]rep[%02X]",value,repeat);
				#endif
				for (int j = 0; j < repeat; j++){
					if (maxSize > 0 && size <= maxSize){
						*memory = value;
						memory++;
					}
					size++;
					if ((maxSize > 0 && size > maxSize)||((memory-base_page)>ZX_RAM_PAGE_SIZE)){
						#ifdef DUMP_UNPACK
							printf("\nOverflow by %d\n",repeat-j);
							printf("\nUsed 2[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
						#endif
						page[i-1] = repeat-j;
						*destSize = size-1;
						return i-3; 
					}
						
				}

				#ifdef DUMP_EXPAND
					printf("addr[%04X]\n",memory-base_page);
				#endif
				
				continue;
			} 
		}

		*memory = page[i];
		memory++;
		size++;
		#ifdef DUMP_RAW
			if((memory-base_page)%48==0){
				printf("\naddr[%04X]",memory-base_page);
			}
			printf("%02X|",page[i]);
		#endif
		if ((maxSize > 0 && size > maxSize)||((memory-base_page)>ZX_RAM_PAGE_SIZE)){
			#ifdef DUMP_UNPACK
				printf("\nUsed 3[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
			#endif
			*destSize = size-1;
			return i;//i+1;
			
		}

	}
	if((page[pageLength-3]==0xED)&&(page[pageLength-2]==0xED)){ //(page[pageLength-1]!=0xED)&& // &&(page[i+1]== 0xED)
		#ifdef DUMP_UNPACK
			printf("\nUsed 7:[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
			for (int j = pageLength-16; j < pageLength; j++){
				printf(" [%02X]",page[j]);
			}
			printf("\n");
		#endif
		*destSize = size-3;
		return (pageLength-3);
	}		
	if((page[pageLength-2]==0xED)&&(page[pageLength-1]==0xED)){ //(page[pageLength-1]!=0xED)&& // &&(page[i+1]== 0xED)
		#ifdef DUMP_UNPACK
			printf("\nUsed 6:[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
			for (int j = pageLength-16; j < pageLength; j++){
				printf(" [%02X]",page[j]);
			}
			printf("\n");
		#endif
		*destSize = size-2;
		return (pageLength-2);
	}
	if(page[pageLength-1]== 0xED){ //(page[pageLength-1]!=0xED)&& // &&(page[i+1]== 0xED)
		#ifdef DUMP_UNPACK
			printf("\nUsed 5:[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
			for (int j = pageLength-16; j < pageLength; j++){
				printf(" [%02X]",page[j]);
			}
			printf("\n");
		#endif
		*destSize = size-1;
		return (pageLength-1);
	}

	#ifdef DUMP_UNPACK
	printf("\nUsed 4:[%08X][%04X][%04X]\n",memory-base_page,pageLength,size);
	for (int j = pageLength-16; j < pageLength; j++){
		printf(" [%02X]",page[j]);
	} printf("\n");	
	//printf(" [%02X]",page[pageLength-3]);
	//printf(" [%02X]",page[pageLength-2]);
	//printf(" [%02X]",page[pageLength-1]);
	/*for (uint8_t col=16;col>0;col--){
		printf(" [%02X]",page[pageLength-col]);
   	}*/
	printf("\n");
	#endif
	*destSize = size;
	return pageLength;
}

uint8_t CountEqualBytes(uint8_t* address, uint8_t* maxAddress){
	int result;
	uint8_t byteValue = *address;

	for (result = 1; result < 255; result++)	{
		address++;
		if (byteValue != *address || address >= maxAddress){
			break;
		}
	}
	return (uint8_t)result;
}

uint16_t CompressPage(uint8_t* page, uint8_t* destMemory,uint16_t maxSize){
	uint16_t size = 0;
	uint16_t destPos = 0;
	uint8_t* maxAddress = page + maxSize;
	bool isPrevoiusSingleED = false;

	for (uint8_t* memory = page; memory < maxAddress; memory++){
		uint8_t byteValue = *memory;
		uint8_t equalBytes;

		if (isPrevoiusSingleED){
			// A byte directly following a single 0xED is not taken into a block
			equalBytes = 1;
		} else {
			equalBytes = CountEqualBytes(memory, maxAddress);
		}

		uint8_t minRepeats;
		if (byteValue == 0xED){
			minRepeats = 2;
		} else {
			minRepeats = 5;
		}

		if (equalBytes >= minRepeats){
			*destMemory = 0xED;
			destMemory++;
			*destMemory = 0xED;
			destMemory++;
			*destMemory = equalBytes;
			destMemory++;
			memory += equalBytes - 1;
			size += 3;
		}

		*destMemory = byteValue;
		destMemory++;
		size++;

		isPrevoiusSingleED = (byteValue == 0xED && memory < maxAddress && *(memory + 1) != 0xED);
	}
	return size;
}

/*  memory dump
    printf("[%04X] Page 5 Used bytes[%04X]\n",5*ZX_RAM_PAGE_SIZE,usedBytes);
	ptr=0;
	do{
		printf("[%04X]",5*ZX_RAM_PAGE_SIZE+ptr);
		for (uint8_t col=0;col<16;col++){
			printf("\t%02X",RAM[5*ZX_RAM_PAGE_SIZE+ptr]);
			ptr++;
		}
		printf("\n");
	} while(ptr<0x4000);
	printf("\n");
*/

bool load_image_z80(char *file_name){
	FIL f;
	int res;
	size_t bytesRead;
	UINT bytesToRead;    
	int fd=0;
	//#ifdef DUMP_PAGES_V1 DUMP_BUFFER
	uint16_t ptr=0;
	//#endif
	uint16_t usedBytes=0;
	uint16_t unusedBytes=0;
	uint8_t im_ver = 0;         // версия образа .z80 (1,2,3,4) версия v3 почти равна v4. Заголовок длиньше на 1 байт
	uint8_t im_hw = 0;              //Тип оборудования для V2 V3
	uint8_t last_out_7ffd;
	uint16_t unpackSize=0;
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint8_t* basePage;
	long pageSize;
	uint16_t destPageSize=0;
	int8_t pageNumber;
	uint16_t pagePtr;
	uint16_t destSize=0;
	DWORD f_pos=0;
	printf("------------------------------------------------------------------------------------\n");
	printf("load_image_z80\n");
	
	//resetAY();//error

	memset(header_buf, 0, sizeof(header_buf));
	
	fd = sd_open_file(&f,file_name,FA_READ);
    printf("file=%s\n",file_name);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	
	//printf("Begin read\n");
	bytesToRead = 30;
	
	fd = sd_read_file(&f,header_buf,bytesToRead,&bytesRead);
	//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
	
	FileHeader* header = (FileHeader*)header_buf;
	//printf("Header assign\n");
	
	uint8_t pagingState;
	
	
	//printf("Header PC=%04X\n",header->PC);
	//printf("Header A=%02X\n",header->A);
	
	if (header->PC != 0){
		// version 1
		im_hw = 48;
		pagingState = 0;
		zx_RAM_bank_active = 0;
		im_ver = 1;
		//readCPUstate(header);
		printf("*V1\n");
	} else {
		bytesToRead=2;
		fd = sd_read_file(&f,&header_buf[30],bytesToRead,&bytesRead);
		if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		
		printf("header->AdditionalBlockLength=%d\n",header->AdditionalBlockLength);
		
		bytesToRead = header->AdditionalBlockLength;
		fd = sd_read_file(&f,&header_buf[32],bytesToRead,&bytesRead);
        if (fd!=FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		
		if (header->AdditionalBlockLength == 23){
			// version 2
			printf("*V2\n");
			im_hw = (header->HardwareMode >= 3)==true ? 128 : 48;
			im_ver = 2;
		} else if (header->AdditionalBlockLength == 54){
			// version 3
			printf("*V3\n");
			im_hw = (header->HardwareMode >= 4)==true ? 128 : 48;
			im_ver = 3;
		} else if (header->AdditionalBlockLength == 55) {
			// version 4
			printf("*V4\n");
			im_hw = (header->HardwareMode >= 4)==true ? 128 : 48;
			im_ver = 4;
		} else {
			// Invalid
			im_ver = -1;
			im_hw = 128;
			sd_close_file(&f);
			return false;
		}
	}

	bool isCompressed;
	if (im_ver==1) {
		isCompressed = (header->Flags1 & 0x20) != 0;
		printf("isCompressed=%d\n",isCompressed);
		memset(sd_buffer, 0, sizeof(sd_buffer));
		bufferIn = (uint8_t *)&sd_buffer;//error
		int bytesToRead = 0;
		int pageIndex = 0;
		//printf(">bufferIn[%08X]\n",bufferIn);
		do{ //for (int pageIndex = 0; pageIndex < 3; pageIndex++)
			//fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
			//if (fd != FR_OK){sd_close_file(&f);return false;}
				#ifdef DUMP_BUFFER
                printf("-------------[buff_dump]-------------\n");
                ptr=0;
                do{
					printf("BF--[%04X]",ptr);
					for (uint8_t col=0;col<16;col++){
						printf("     %02X",sd_buffer[ptr]);
						ptr++;
                    }
                    printf("\n");
            	} while(ptr<sizeof(sd_buffer));
                printf("-------------[buff_dump]-------------\n");
				#endif
			//if (!isCompressed && bytesRead != bytesToRead) {sd_close_file(&f);return false;}
			pageSize = ZX_RAM_PAGE_SIZE;
			switch (pageIndex) {
				case 0:
				printf("Page:5\n");
 				bufferOut = &RAM[5*ZX_RAM_PAGE_SIZE];
				basePage = &RAM[5*ZX_RAM_PAGE_SIZE];
				break;
				case 1:
				printf("Page:2\n");
				bufferOut = &RAM[2*ZX_RAM_PAGE_SIZE];
				basePage = &RAM[2*ZX_RAM_PAGE_SIZE];
				break;
				case 2:
				printf("Page:%d\n",zx_RAM_bank_active);
				bufferOut = &RAM[zx_RAM_bank_active*ZX_RAM_PAGE_SIZE];
				basePage = &RAM[zx_RAM_bank_active*ZX_RAM_PAGE_SIZE];
				break;
			}
			//printf("pageSize[%04X], bytesToRead[%04X]\n",pageSize,bytesToRead);
			do{
				if(bytesToRead==0){
					if (pageSize>sizeof(sd_buffer)){
						bytesToRead = sizeof(sd_buffer);
					} else {
						bytesToRead = pageSize;
					}				
				}
				//printf("pageSize[%04X], bytesToRead[%04X]\n",pageSize,bytesToRead);				
				f_pos = sd_file_pos(&f);				
				//printf(">bufferIn[%08X]\n",bufferIn);
				fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
				if (fd != FR_OK){sd_close_file(&f);return false;}
				//printf("bytesToRead[%04X], bytesRead[%04X], pos[%04X]\n",bytesToRead,bytesRead,f_pos);
				//if (f_pos == sd_file_size(&f)){sd_close_file(&f);return false;}
				#ifdef DUMP_BUFFER
					printf("-------------[buff_dump]-------------\n");
        	        ptr=0;
            	    do{
						printf("----[%04X]",ptr);
						for (uint8_t col=0;col<16;col++){
							printf("     %02X",sd_buffer[ptr]);
							ptr++;
    	                }
        	            printf("\n");
            		} while(ptr<sizeof(sd_buffer));
                	printf("-------------[buff_dump]-------------\n");
				#endif	

				
				if (isCompressed){
				
					//printf("*Buff_before[%08X]\n",bufferOut);
					//usedBytes = DecompressPage(sd_buffer, sizeof(sd_buffer), isCompressed, pageSize, bufferOut, &destSize,basePage);
					usedBytes = DecompressPage(sd_buffer, sizeof(sd_buffer), isCompressed, ZX_RAM_PAGE_SIZE, bufferOut, &destSize,basePage);	
					bufferOut+=destSize;
					//printf("*Buff_after[%08X][%04X]\n",bufferOut,destSize);
					unusedBytes = sizeof(sd_buffer) - usedBytes; // part of next page(s)
					pageSize-=sizeof(sd_buffer);
					bytesToRead = usedBytes;
					for (int i = 0; i < unusedBytes; i++){sd_buffer[i] = sd_buffer[i + usedBytes];}
					for (int i = unusedBytes; i < sizeof(sd_buffer); i++){sd_buffer[i] = 0;}
					//bufferIn+=usedBytes;
					bufferIn = &sd_buffer[unusedBytes];
					#ifdef DUMP_PAGES_V1
					printf("*Used bytes[%04X]\n",usedBytes);
					printf("*Unused bytes[%04X]\n",unusedBytes);
					#endif
					unusedBytes=0;
					usedBytes=0;
				} else {
					memcpy(bufferOut,bufferIn,bytesRead);
					bufferOut+=pageSize;
					pageSize-=sizeof(sd_buffer);
					bufferIn = (uint8_t *)&sd_buffer;//error
					bytesToRead = sizeof(sd_buffer);
				}

			}while(pageSize>0);
			#ifdef DUMP_PAGES_V1
    			printf("Page%d Used bytes[%04X]\n",pageIndex,usedBytes);
				ptr=0;
				uint8_t* memory = bufferOut-ZX_RAM_PAGE_SIZE;
				do{
					printf("Page[%04X]",ptr);
					for (uint8_t col=0;col<16;col++){
						printf("     %02X",memory[ptr]);
						ptr++;
					}
					printf("\n");
				} while(ptr<ZX_RAM_PAGE_SIZE);
				printf("\n");
			#endif
			pageIndex++;
		}while(sd_file_pos(&f)<sd_file_size(&f));
		im_hw = 48;
		readCPUstate(header,im_ver,im_hw);
		sd_close_file(&f);
		return true;
	} else {
		pagingState = header->PagingState;
		bytesToRead = 3;
		fd = sd_read_file(&f,&buf,bytesToRead,&bytesRead);
        if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		MemBlock* block = (MemBlock*) buf;
		// Get pageSize and pageNumber
		pageSize = block->Size;
		pageNumber = block->PageNum;
		f_pos = sd_file_pos(&f);
		GetPageInfo(&buf, im_hw, pagingState, &pageNumber, &pageSize);
		//#ifdef DUMP_PAGES_V2
		//	printf("MemBlock: pageNumber[%02X], pageSize[%04X] f_pos[%04X]\n",pageNumber,pageSize,f_pos);
		//#endif

		do{
			//printf(">DATA: page[%02X],size[%04X]\n",pageNumber,pageSize);
			isCompressed = (pageSize<0xFFFF);
			if (!isCompressed){
				pageSize = ZX_RAM_PAGE_SIZE;
			}
			//printf(">MemPtr[%04X]\n",pageNumber*ZX_RAM_PAGE_SIZE);
			if (pageNumber<8){
				bufferOut = &RAM[pageNumber*ZX_RAM_PAGE_SIZE];
				basePage = &RAM[pageNumber*ZX_RAM_PAGE_SIZE];
				bufferIn = (uint8_t *)&sd_buffer;//error
				bytesToRead = 0;
				do{ // Read page into tempBuffer and unpack
					if (pageSize>sizeof(sd_buffer)){
						bytesToRead = sizeof(sd_buffer);
					} else {
						bytesToRead = pageSize;
					}
					//printf(">bufferIn[%08X]\n",bufferIn);
					fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
					if (fd != FR_OK){sd_close_file(&f);return false;}
					f_pos = sd_file_pos(&f);
					//printf("bytesToRead[%04X], bytesRead[%04X], pos[%04X]\n",bytesToRead,bytesRead,f_pos);
					//if (f_pos == sd_file_size(&f)){sd_close_file(&f);return false;}
					if (isCompressed){
						//printf("*Buff_before[%08X]\n",bufferOut);
						usedBytes = DecompressPage(sd_buffer, sizeof(sd_buffer), isCompressed, ZX_RAM_PAGE_SIZE, bufferOut, &destSize,basePage);
						//usedBytes = DecompressPage(sd_buffer, sizeof(sd_buffer), isCompressed, pageSize, bufferOut, &destSize,basePage);
						bufferOut+=destSize;
						//printf("*Buff_after[%08X][%04X]\n",bufferOut,destSize);
						unusedBytes = sizeof(sd_buffer) - usedBytes; // part of next page(s)
						pageSize-=sizeof(sd_buffer);
						bytesToRead = usedBytes;
						for (int i = 0; i < unusedBytes; i++){sd_buffer[i] = sd_buffer[i + usedBytes];}
						for (int i = unusedBytes; i < sizeof(sd_buffer); i++){sd_buffer[i] = 0;}
						bufferIn = &sd_buffer[unusedBytes];
						#ifdef DUMP_PAGES_V2
						printf("*Used bytes[%04X]\n",usedBytes);
						printf("*Unused bytes[%04X]\n",unusedBytes);
						#endif
						unusedBytes=0;
						usedBytes=0;
						//printf("*pageSize[%04X]\n",pageSize);
					} else {
						memcpy(bufferOut,sd_buffer,sizeof(sd_buffer));
						bufferOut+=sizeof(sd_buffer);
						pageSize-=sizeof(sd_buffer);
						bytesToRead=sizeof(sd_buffer);
					}
				}while (pageSize>0);
				
				#ifdef DUMP_PAGES_V2
					printf("------\n");
    				printf("Page[%d]\n",pageNumber);
					ptr=0;
					uint8_t* memory = bufferOut-ZX_RAM_PAGE_SIZE;
					do{
						printf("Page[%04X]",ptr);
						for (uint8_t col=0;col<32;col++){
							printf("     %02X",memory[ptr]);
							ptr++;
						}
						printf("\n");
					} while(ptr<ZX_RAM_PAGE_SIZE);
					printf("\n");
				#endif
			} else {
				// Move forward without reading
				printf("Move forward without reading\n");
				if (sd_seek_file(&f,sd_file_pos(&f)+pageSize) != FR_OK){
					sd_close_file(&f);
					return false;
				}
			}
			
			bytesToRead = 3;
			fd = sd_read_file(&f,&buf,bytesToRead,&bytesRead);
            if (fd != FR_OK){sd_close_file(&f);return false;}
			//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
			if ((bytesRead != bytesToRead) &&(sd_file_pos(&f)<sd_file_size(&f)) ){sd_close_file(&f);return false;}
			if (bytesRead == 3){
				pageSize = block->Size;
				pageNumber = block->PageNum;
				f_pos = sd_file_pos(&f);
				GetPageInfo(&buf, im_hw, pagingState, &pageNumber, &pageSize);
			//	printf("MemBlock: pageNumber[%02X], pageSize[%02X] f_pos[%02X]\n",pageNumber,pageSize,f_pos);
				if (pageNumber<0){
					return false;
				}
			} else  {
				pageSize = 0;
			}
		} while (pageSize > 0);
		
		if ( ((header->HWModeState&0b00000100) && ((im_hw == 48) &&  (im_ver > 1)))  || im_hw == 128 ){
			readAYState(header);
		} else {
		//	printf("No AY Init\n");
		}
		readCPUstate(header,im_ver,im_hw);
	}
	sd_close_file(&f);
	return true;
} // Конец процедуры

bool LoadScreenFromZ80Snapshot(char *file_name){
	FIL f;
	int res;
	size_t bytesRead;
	UINT bytesToRead;    
	int fd=0;
	uint16_t ptr=0;
	char fileinfo[30];
	char sound[8];
	uint8_t im_ver = 0;         // версия образа .z80 (1,2,3,4) версия v3 почти равна v4. Заголовок длиньше на 1 байт
	uint8_t im_hw;              //Тип оборудования для V2 V3

	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint16_t pageSize = 0;
	int8_t pageNumber = 0;
	uint16_t destSize=0;
	uint16_t usedBytes=0;
	uint16_t unusedBytes=0;

	printf("load_screen_z80\n");
	memset(header_buf, 0, sizeof(header_buf));
	memset(sd_buffer, 0, sizeof(sd_buffer));
	memset(fileinfo, 0, sizeof(fileinfo));
	memset(sound, 0, sizeof(sound));

	
	fd = sd_open_file(&f,file_name,FA_READ);
    //printf("sd_open_file=%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&f);return false;}

	//printf("Begin read\n");
	bytesToRead = 30;
	//sleep_ms(100);
	fd = sd_read_file(&f,header_buf,bytesToRead,&bytesRead);
    if (fd!=FR_OK){sd_close_file(&f);return false;}
	//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
	if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		
	FileHeader* header = (FileHeader*)header_buf;
		
	//printf("Header assign\n");
	//uint8_t borderColor = (header->Flags1 & 0x0E) >> 1;
		
	bool isCompressed;
	if (header->PC != 0){
		// version 1
		im_hw = 48;
		im_ver = 1;
		printf("Version: %d, Hardware: %d\n",im_ver,im_hw);
		isCompressed = (header->Flags1 & 0x20) != 0;
		bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
		bufferIn = (uint8_t *)&sd_buffer;//error
		//memset(bufferOut, 0, 0x1B00);
		pageSize = 0x1B00;
		do{
			if (pageSize>sizeof(sd_buffer)){
				bytesToRead = sizeof(sd_buffer);
			} else {
				bytesToRead = pageSize;
			}
			fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
			if (fd != FR_OK){sd_close_file(&f);return false;}
			//printf("*bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
			if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
			//printf("*Buff_before[%08X]\n",bufferOut);
			usedBytes = DecompressPage(sd_buffer, bytesToRead, isCompressed, 0x1B00, bufferOut, &destSize,bufferOut);
			if(destSize>=0x1B00){break;}
			bufferOut+=destSize;
			//printf("*Buff_after[%08X][%04X]\n",bufferOut,destSize);
			unusedBytes = sizeof(sd_buffer) - usedBytes; // part of next page(s)
			pageSize-=bytesToRead;
			if((sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE)&&(pageSize<=0)){break;};
			bytesToRead = usedBytes;
			for (int i = 0; i < unusedBytes; i++){sd_buffer[i] = sd_buffer[i + usedBytes];}
			for (int i = unusedBytes; i < sizeof(sd_buffer); i++){sd_buffer[i] = 0;}
			bufferIn = &sd_buffer[unusedBytes];
			#ifdef DUMP_PAGES_V1
			printf("*Used bytes[%04X]\n",usedBytes);
			printf("*Unused bytes[%04X]\n",unusedBytes);
			#endif
			unusedBytes=0;
			usedBytes=0;
		//	printf("*PS[%04X]\n",pageSize);
		}while (pageSize>0);
		bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
		//printf("buffer[%04X]\n",bufferOut);
		ShowScreenshot(bufferOut);
	//	memset(fileinfo, 0, sizeof(fileinfo));
	//	sprintf(fileinfo,"Type:.z80 Ver:%d",im_ver);
	//	draw_text_len(18+FONT_W*14,208, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	//	memset(fileinfo, 0, sizeof(fileinfo));
	//	sprintf(fileinfo,"Mem:%dk Snd:%s",im_hw,"Buzzer");
	//	draw_text_len(18+FONT_W*14,216, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	//	memset(fileinfo, 0, sizeof(fileinfo));
		//strncpy(file_name,"A:/",3);
		//strncpy(fileinfo,file_name,22);
		//draw_text_len(18+FONT_W*14,224, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
		
	} else {
		uint8_t pagingState;
		bytesToRead=2;
		fd = sd_read_file(&f,&header_buf[30],bytesToRead,&bytesRead);
		if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}

		bytesToRead = header->AdditionalBlockLength;
		fd = sd_read_file(&f,&header_buf[32],bytesToRead,&bytesRead);
        if (fd!=FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}        
		
		if (header->AdditionalBlockLength == 23){
			// version 2
			//printf("*V2\n");
			im_hw = (header->HardwareMode >= 3)==true ? 128 : 48;
			im_ver = 2;
		} else if (header->AdditionalBlockLength == 54){
			// version 3
			//printf("*V3\n");
			im_hw = (header->HardwareMode >= 4)==true ? 128 : 48;
			im_ver = 3;
		} else if (header->AdditionalBlockLength == 55) {
			// version 4
			//printf("*V4\n");
			im_hw = (header->HardwareMode >= 4)==true ? 128 : 48;
			im_ver = 4;
		} else {
			// Invalid
			im_ver = -1;
			im_hw = 128;
			return false;
		}
		
		if ( ((header->HWModeState&0b00000100) && ((im_hw == 48) &&  (im_ver > 1)))  || im_hw == 128 ){
			strncpy(sound, "AY8910", 6);
		} else {
			strncpy(sound, "Buzzer", 6);
		}

		memset(fileinfo, 0, sizeof(fileinfo));
		sprintf(fileinfo,"Type:.z80 Ver:%d",im_ver);
		draw_text_len(18+FONT_W*14,208, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
		
		memset(fileinfo, 0, sizeof(fileinfo));
		sprintf(fileinfo,"Mem:%dk Snd:%s",im_hw,sound);
		draw_text_len(18+FONT_W*14,216, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);

		memset(fileinfo, 0, sizeof(fileinfo));
		//strncpy(fileinfo,file_name,22);
		//strncpy(fileinfo,"A:/",3);
		/*printf("FileInfo\t");
		for(uint8_t chr=0; chr<sizeof(fileinfo);chr++){
			printf(" [%02X] ",fileinfo[chr]);
		};printf("\n");*/

		//draw_text_len(18+FONT_W*14,224, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
		/*printf("Buf\n");
		for(uint8_t chr=0; chr<sizeof(buf);chr++){
			printf(" [%02X] ",buf[chr]);
		};printf("\n");*/

		pagingState = header->PagingState;
		bytesToRead = 3;
		fd = sd_read_file(&f,&buf,bytesToRead,&bytesRead);
        if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		MemBlock* block = (MemBlock*) buf;
		// Get pageSize and pageNumber
		pageSize = block->Size;
		pageNumber = block->PageNum;
		GetPageInfo(&buf, im_hw, pagingState, &pageNumber, &pageSize);
		do{
			isCompressed = (pageSize<0xFFFF);
			if (!isCompressed){
				pageSize = ZX_RAM_PAGE_SIZE;
			}
		//	printf("DATA: Size:%04X, page:%02X, compressed:%d\n",pageSize,pageNumber,isCompressed);
			//printf("MemPtr=%04X\n",pageNumber*ZX_RAM_PAGE_SIZE);
			if (pageNumber == 5){// This page contains screenshoot
				pageSize = 0x1B00;
				bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
				//memset(bufferOut, 0, 0x2000);
			//	printf("SDB[%08X],SDBI[%08X],RAMB[%08X]\n",&sd_buffer,&sd_buffer[0x2000],&RAM[5*ZX_RAM_PAGE_SIZE]);
				bufferIn = (uint8_t *)&sd_buffer;	//error			
				if (isCompressed){
					do{ // Read page into tempBuffer and unpack
						if (pageSize>sizeof(sd_buffer)){
							bytesToRead = sizeof(sd_buffer);
						} else {
							bytesToRead = pageSize;
						}
						DWORD f_pos = sd_file_pos(&f);
						//printf(">bufferIn[%08X]\n",bufferIn);
						fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
						if (fd != FR_OK){sd_close_file(&f);return false;}
						//printf("bytesToRead[%04X], bytesRead[%04X], pos[%04X]\n",bytesToRead,bytesRead,f_pos);
						//if (f_pos == sd_file_size(&f)){sd_close_file(&f);return false;}
					//	printf("\n*Buff_before[%08X]\n",bufferOut);
						usedBytes = DecompressPage(sd_buffer, bytesToRead, isCompressed, 0x1B00, bufferOut, &destSize,bufferOut);
						if(destSize>=0x1B00){break;}
						bufferOut+=destSize;
					//	printf("\n*Buff_after[%08X][%04X]\n",bufferOut,destSize);
						unusedBytes = sizeof(sd_buffer) - usedBytes; // part of next page(s)
						pageSize-=bytesToRead;
						if((sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE)&&(pageSize<=0)){break;};
						bytesToRead = usedBytes;
						for (int i = 0; i < unusedBytes; i++){sd_buffer[i] = sd_buffer[i + usedBytes];}
						for (int i = unusedBytes; i < sizeof(sd_buffer); i++){sd_buffer[i] = 0;}
						bufferIn = &sd_buffer[unusedBytes];
						//#ifdef DUMP_PAGES_V1
					//	printf("*Used bytes[%04X]\n",usedBytes);
					//	printf("*Unused bytes[%04X]\n",unusedBytes);
						//#endif
						unusedBytes=0;
						usedBytes=0;
					//	printf("*pageSize[%04X]\n",pageSize);
					}while (pageSize>0);
					//bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];					
				} else {
					//bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
					//printf("*Buff_before[%08X]\n",bufferOut);
					bytesToRead=pageSize;
					DWORD f_pos = sd_file_pos(&f);
					fd = sd_read_file(&f,bufferOut,bytesToRead,&bytesRead);
					if (fd != FR_OK){sd_close_file(&f);return false;}
					//printf("bytesToRead[%04X], bytesRead[%04X], pos[%04X]\n",bytesToRead,bytesRead,f_pos);
					//printf("*Buff_after[%08X]\n",bufferOut);
				}
				//printf("BFIN[%04X],BFOUT[%04X]\n",bufferIn,bufferOut);
				
				//printf("BFIN[%04X],BFOUT[%04X]\n",bufferIn,bufferOut);
				
				//printf("Page%d Used bytes[%04X]\n",pageNumber,usedBytes);
				/*uint16_t ptr=0;
				do{
					printf("Page%d[%04X]",pageNumber,sd_buffer+ptr);
					for (uint8_t col=0;col<32;col++){
						printf("     %02X",sd_buffer[ptr]);
						ptr++;
					}
					printf("\n");
				} while(ptr<0x4000);
				printf("\n");*/
				
				bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
				//printf("BF[%04X],BF[%04X],BF[%04X]\n",sd_buffer,&sd_buffer[0x2000],bufferOut);
		        ShowScreenshot(bufferOut);
				//printf("Show screen\n");
				sd_close_file(&f);
		    	return true;
		    } else	{
		        // Move forward without reading
				//printf("Move forward without reading to [%08X]\n",sd_file_pos(&f)+pageSize);
				if (sd_seek_file(&f,sd_file_pos(&f)+pageSize) != FR_OK){
					sd_close_file(&f);
					return false;
				}
		    }
		
			bytesToRead = 3;
			fd = sd_read_file(&f,&buf,bytesToRead,&bytesRead);
            if (fd != FR_OK){sd_close_file(&f);return false;}
			//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
			if ((bytesRead != bytesToRead) &&(sd_file_pos(&f)<sd_file_size(&f)) ){sd_close_file(&f);return false;}
			if (bytesRead == 3){
				pageSize = block->Size;
				pageNumber = block->PageNum;                
				GetPageInfo(&buf, im_hw, pagingState, &pageNumber, &pageSize);
				//printf("PageInfo: pageNumber=%02X, pageSize=%02X\n",pageNumber,pageSize);
			} else  {
				pageSize = 0;
			}
		} while (pageSize > 0);
	}
	//zx_Border_color=0;
	sd_close_file(&f);
	return true;
}

bool LoadScreenshot(char *file_name, bool open_file){
    FIL f;
	size_t bytesRead;
    int fd=0;
	char fileinfo[30];
	UINT bytesToRead;
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	uint16_t pageSize = 0;
	int8_t pageNumber = 0;
	uint16_t destSize=0;
	uint16_t usedBytes=0;

	bufferIn = &sd_buffer;

	bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
	if (open_file){bufferOut = &RAM[5*ZX_RAM_PAGE_SIZE];}
	memset(bufferOut, 0, 0x1B00);

	pageSize = 0x1B00;
	fd = sd_open_file(&f,file_name,FA_READ);
    //printf("sd_open_file=%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&f);return false;}	
	
	do{
		if (pageSize>sizeof(sd_buffer)){
			bytesToRead = sizeof(sd_buffer);
		} else {
			bytesToRead = pageSize;
		}
		fd = sd_read_file(&f,bufferIn,bytesToRead,&bytesRead);
		if (fd != FR_OK){sd_close_file(&f);return false;}
		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
		if (bytesRead != bytesToRead){sd_close_file(&f);return false;}
		memcpy(bufferOut,bufferIn,bytesRead);
		bufferIn = &sd_buffer;
		bufferOut += bytesRead;
		pageSize-=bytesRead;
	}while (pageSize>0);
	if (!open_file){
		bufferOut = (sizeof(sd_buffer)>=ZX_RAM_PAGE_SIZE) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
		ShowScreenshot(bufferOut);
	//	sprintf(fileinfo,"Type:.SCR - ZX Screen");
	//	draw_text_len(18+FONT_W*14,209, fileinfo,COLOR_TEXT,COLOR_BACKGOUND,22);
	}
	zx_Border_color=0;
	//last_out_7ffd = 0x30;
	//zx_video_ram  = zx_ram_bank[5];
	sd_close_file(&f);
	return true;
}

bool save_image_z80(char *file_name){
	FIL f;
	int res;
	size_t bytesToWrite;
	UINT bytesWritten;    
	int fd=0;
	uint16_t ptr=0;
	uint16_t usedBytes=0;
	uint16_t writeBytes=0;
	DWORD f_pos_size = 0;
	DWORD f_pos_data = 0;
	
	printf("save_image_z80\n");

	memset(header_buf, 0, sizeof(header_buf));

	FileHeader* header = (FileHeader*)header_buf;
	saveCPUstate(header);
	saveAYState(header);
	
	uint8_t last_out_7ffd = zx_machine_get_7ffd_lastOut();
	/*if (last_out_7ffd & 16){ //5bit = {1 - 48k[R0], 0 - 128k[R1]}
		header->HardwareMode = 0; 
	} else {
		header->HardwareMode = 3;
	}*/

	header->HardwareMode = 3; 

    uint8_t pageCount;
    uint8_t pagesToSave[8];
    if (header->HardwareMode == 0){
        // Save as 48K snaphot
        pageCount = 3;
        pagesToSave[0] = 5;
        pagesToSave[1] = 2;
        pagesToSave[2] = 0;
    } else {
        // Save as 128K snaphot
        pageCount = 8;
    	for (int i = 0; i < pageCount; i++){
            pagesToSave[i] = i;
        }
    }

	fd = sd_open_file(&f,file_name,FA_CREATE_ALWAYS|FA_WRITE);
    //printf("sd_open_file=%d\n",fd);
	if (fd!=FR_OK){sd_close_file(&f);return false;}

	bytesToWrite = 32 + header->AdditionalBlockLength;

	//printf("Header Write %d\n",bytesToWrite);

	fd = sd_write_file(&f,header_buf,bytesToWrite,&bytesWritten);
	//printf("bytesToWrite=%d, bytesWritten=%d\n",bytesToWrite,bytesWritten);
	if (fd!=FR_OK){sd_close_file(&f);return false;}
	if (bytesWritten != bytesToWrite){sd_close_file(&f);return false;}

	for (int i = 0; i < pageCount; i++){
        uint8_t pageNumber = pagesToSave[i];
		uint8_t* buffer;

        switch (pageNumber){
            case 0:
				//buffer = &RAM[zx_RAM_bank_active*ZX_RAM_PAGE_SIZE];
                //break;
            case 2:
            case 5:
				buffer = &RAM[pageNumber*ZX_RAM_PAGE_SIZE];
                break;
            case 1:
            case 3:
            case 4:
            case 6:
            case 7:
				buffer = &RAM[pageNumber*ZX_RAM_PAGE_SIZE];
                break;
        }


		MemBlock* block = (MemBlock*) buf;
		//buffer = (uint8_t*)sd_buffer;
		block->Size = 0;

        if (pageCount == 3){ //48K mode
            switch (pageNumber){
            case 2:
				block->PageNum = 4;
                break;
            case 5:
    		    block->PageNum =  8;
                break;
            case 0:
    		    block->PageNum =  5;
                break;

            default:
    		    block->PageNum =  5;
                break;
            }
        } else { //128K mode
		    block->PageNum = pageNumber + 3;
        }


		f_pos_size = sd_file_pos(&f);
		bytesToWrite = 3;
		fd = sd_write_file(&f,buf,bytesToWrite,&bytesWritten);
	//	printf("bytesToWrite=%d, bytesWritten=%d\n",bytesToWrite,bytesWritten);
		if (fd!=FR_OK){sd_close_file(&f);return false;}
		if (bytesWritten != bytesToWrite){sd_close_file(&f);return false;}		


		uint16_t pageSize = ZX_RAM_PAGE_SIZE;
		usedBytes=0;
		do{
			writeBytes = CompressPage(buffer, sd_buffer, sizeof(sd_buffer));
			//printf("Compress: buffer[%08X]   sd_buffer[%08X]   sizeof[%04X]   usedBytes[%04X] \n",buffer,sd_buffer,sizeof(sd_buffer),usedBytes);

			/*
			printf("-------------[buff_dump]-------------\n");
			ptr=0;
			do{
				printf("----[%04X]",ptr);
				for (uint8_t col=0;col<16;col++){
					printf("     %02X",sd_buffer[ptr]);
					ptr++;
				}
				printf("\n");
			} while(ptr<sizeof(sd_buffer));
			printf("-------------[buff_dump]-------------\n");
			*/
			usedBytes+=writeBytes;
			buffer+=sizeof(sd_buffer);
			pageSize-=sizeof(sd_buffer);
	        bytesToWrite = writeBytes;
			fd = sd_write_file(&f,sd_buffer,bytesToWrite,&bytesWritten);
			//printf("bytesToWrite=%d, bytesWritten=%d\n",bytesToWrite,bytesWritten);
			if (fd!=FR_OK){sd_close_file(&f);return false;}
			if (bytesWritten != bytesToWrite){sd_close_file(&f);return false;}		
		} while (pageSize > 0);	

		f_pos_data = sd_file_pos(&f);
		block->Size =usedBytes;
		printf("Block Write Page[%d] Size[%04X]\n",block->PageNum,block->Size);
		//printf("Block size: usedBytes[%04X]\n",usedBytes);
		sd_seek_file(&f,f_pos_size);
		bytesToWrite = 3;
		fd = sd_write_file(&f,buf,bytesToWrite,&bytesWritten);
		//printf("bytesToWrite=%d, bytesWritten=%d\n",bytesToWrite,bytesWritten);
		if (fd!=FR_OK){sd_close_file(&f);return false;}
		if (bytesWritten != bytesToWrite){sd_close_file(&f);return false;}
		sd_seek_file(&f,f_pos_data);
	}
	sd_close_file(&f);
	return true;
}

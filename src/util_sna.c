#include "util_sna.h"
#include "util_sd.h"
#include "ff.h"
#include <fcntl.h>
#include <string.h>
#include "zx_emu/Z80.h"

#include "zx_emu/zx_machine.h"
#include "screen_util.h"
extern uint8_t zx_RAM_bank_7ffd;
/*
	Формат файлов .sna
	
	Материал из Энциклопедии ZX Spectrum.
	
	Образ программы в  формате  .sna  (от  английского  snapshot) -
	один из наиболее широко поддерживаемых форматов  для  сохранения
	состояния работающей программы для последующего её выполнения.
	
	Версия для 48K
	Основным недостатком этого формата является использование  стэка
	для сохранения программного счётчика (содержимого  регистра  PC),
	для последующего восстановления работы программ инструкцией RETN.
	В общем случае это не сказывается на  работе  программы,  однако
	если вершина стэка в момент сохранения образа находилась  в  об-
	ласти ПЗУ или критически близко к исполняемому коду, последствия
	могут быть фатальными для работаюшей программы.
	
	Смещение Размер   Описание
	---------------------------
	0		1		Регистр I.
	1		2		Регистровая пара HL'.
	3		2		Регистровая пара DE'.
	5		2		Регистровая пара BC'.
	7		2		Регистровая пара AF'.
	9		2		Регистровая пара HL.
	11		2		Регистровая пара DE.
	13		2		Регистровая пара BC.
	15		2		Регистровая пара IY.
	17		2		Регистровая пара IX.
	19		1		Состояние прерываний. Бит 2 содержит состояние триггера IFF2, бит 1 - IFF1 (0=DI, 1=EI).
	20		1		Регистр R.
	21		2		Регистровая пара AF.
	23		2		Указатель на вершину стэка (SP).
	25		1		Режим прерываний: 0=IM0, 1=IM1, 2=IM2.
	26		1		Цвет бордюра, 0-7.
	27		49152	Содержимое памяти с адреса 16384 (4000h).
	
	Версия для 128K
	Этот формат расширяет предыдущий, позволяя  хранить  расширенную
	память ZX Spectrum 128 и +3, а также  решает  проблему  хранения
	регистра PC: теперь он сохраняется не в стэке,  а  в  специально
	отведённой области в файле.
	
	Полный формат файла:
	
	Смещение Размер   Описание
	---------------------------
	0		27	   Заголовок SNA, см. выше.
	27	   16384	Страница памяти #5.
	16411	16384	Страница памяти #2.
	32795	16384	Активная страница памяти. Если активна страни-
	ца #2 или #5 -  она,  фактически,  хранится  в
	файле дважды.
	49179	2		Содержимое регистра PC.
	49181	1		Содержимое порта 7FFDh.
	49182	1		1 если активно ПЗУ TR-DOS, 0 - если нет.
	49183	16384	Оставшиеся страницы памяти.
	
	Общая длина файла составляет 131103 или 147487 байтов.  Дополни-
	тельные страницы памяти хранятся в порядке возврастания, за  ис-
	ключением страниц #2, #5 и текущей активной.
	
	Получено с http://zx.org.ru/db/.sna
	
*/

extern Z80 cpu;
//extern uint32_t zx_RAM_bank_active;
extern uint8_t* zx_cpu_ram[4];//Адреса 4х областей памяти CPU при использовании страниц
extern uint8_t* zx_ram_bank[8];
extern uint8_t* zx_rom_bank[4];//Адреса 4х областей ПЗУ (48к 128к TRDOS и резерв для какого либо режима(типа тест))
extern uint8_t* zx_video_ram;

extern bool zx_state_48k_MODE_BLOCK;
extern uint8_t zx_Border_color;

//extern uint8_t zx_machine_last_out_7ffd;
extern uint8_t zx_machine_last_out_fffd;
extern void zx_machine_set_7ffd_out(uint8_t val);

extern int last_error;

extern int fd;					    // индикатор ошибки чтения файла
extern char buf[10];			    // временный буфер
extern char header_buf[87];		    // буфер для чтения заголовка
static uint8_t last_out_7ffd;       // порт банков памяти
extern uint32_t zx_RAM_bank_active;

/*
	0		1		Регистр I.
	1		2		Регистровая пара HL'.
	3		2		Регистровая пара DE'.
	5		2		Регистровая пара BC'.
	7		2		Регистровая пара AF'.
	9		2		Регистровая пара HL.
	11		2		Регистровая пара DE.
	13		2		Регистровая пара BC.
	15		2		Регистровая пара IY.
	17		2		Регистровая пара IX.
	19		1		Состояние прерываний. Бит 2 содержит состояние триггера IFF2, бит 1 - IFF1 (0=DI, 1=EI).
	20		1		Регистр R.
	21		2		Регистровая пара AF.
	23		2		Указатель на вершину стэка (SP).
	25		1		Режим прерываний: 0=IM0, 1=IM1, 2=IM2.
	26		1		Цвет бордюра, 0-7.
*/

typedef struct FileHeader{
	uint8_t IR;  //0
	uint8_t L_Dash; //1
	uint8_t H_Dash; //2
	uint8_t E_Dash; //3
	uint8_t D_Dash; //4
	uint8_t C_Dash; //5
	uint8_t B_Dash; //6
	uint8_t F_Dash; //8
	uint8_t A_Dash; //7
	uint8_t L; //9  HL register pair
	uint8_t H; //10  HL register pair
	uint8_t E; //11
	uint8_t D; //12
	uint8_t C; //13  BC register pair (LSB, i.e. C, first)
	uint8_t B; //14  BC register pair (LSB, i.e. C, first)
	uint16_t IY; //15<<16
	uint16_t IX; //17<<18
	uint8_t IFF; //19
	uint8_t RR;  //20
	uint8_t F; //21  F register
	uint8_t A; //22  A register
	uint16_t SP; //24<<23 Stack pointer
	uint8_t IMode; //25
	uint8_t Border; //26
} __attribute__((packed)) FileHeader;

uint8_t read_byte_48k(uint16_t addr){ // Чтение памяти
	if (addr<16384) return zx_cpu_ram[0][addr];
	if (addr<32768) return zx_cpu_ram[1][addr-16384];
	if (addr<49152) return zx_cpu_ram[2][addr-32768];
	return zx_cpu_ram[3][addr-49152];	
}

static inline uint16_t read_word_48k(uint16_t addr) {
	return (read_byte_48k(addr + 1) << 8) |
			read_byte_48k(addr);
}

void readCPUstateSNA(FileHeader* header,uint8_t im_hw){

	Z80_A(z1->cpu) = header->A;      
	Z80_F(z1->cpu) = header->F;
	Z80_B(z1->cpu) = header->B;     
    Z80_C(z1->cpu) = header->C;    
	Z80_H(z1->cpu) = header->H; 
	Z80_L(z1->cpu) = header->L; 
	Z80_D(z1->cpu) = header->D;  
	Z80_E(z1->cpu) = header->E;	


	Z80_SP(z1->cpu) = header->SP; //Z80_SP(z1->cpu).W;         // SP
	z1->cpu.i = header->IR;
    z1->cpu.r = header->RR;
	
	zx_Border_color = ((header->Border & 0x07) << 4) | (header->Border & 0x07); // дублируем для 4 битного видеобуфера
		
	Z80_B_(z1->cpu) = header->B_Dash; 
	Z80_C_(z1->cpu) = header->C_Dash;

	Z80_D_(z1->cpu) = header->D_Dash; 
	Z80_E_(z1->cpu) = header->E_Dash;

	Z80_H_(z1->cpu) = header->H_Dash; 
	Z80_L_(z1->cpu) = header->L_Dash;
	
	Z80_A_(z1->cpu) = header->A_Dash;
	Z80_F_(z1->cpu) = header->F_Dash;
	
	
	Z80_IX(z1->cpu)  = header->IX;
	Z80_IY(z1->cpu) = header->IY;
	
	z1->cpu.iff1 = (header->IFF & 0b00000001);
	z1->cpu.iff2 = (header->IFF & 0b00000010);
	
	z1->cpu.im = header->IMode; //Биты 0-1: режим прерываний (0-2)

    z1->cpu.request = 0; //cpu.Int_pending = 0;



	zx_RAM_bank_7ffd = last_out_7ffd & 0x7;
	zx_RAM_bank_active=zx_RAM_bank_7ffd;
	zx_cpu_ram[3]=zx_ram_bank[zx_RAM_bank_7ffd]; // 0xC000 - 0x7FFF 
	
	if (last_out_7ffd &  8) zx_video_ram=zx_ram_bank[7];  else zx_video_ram  = zx_ram_bank[5];
	/////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (last_out_7ffd & 16) zx_cpu_ram[0]=zx_rom_bank[1]; else zx_cpu_ram[0] = zx_rom_bank[0]; //5bit = {1 - 48k[R0], 0 - 128k[R1]}
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if (last_out_7ffd & 32) zx_state_48k_MODE_BLOCK=true; // 6bit = 1 48k mode block // && (im_hw==48)
}

bool load_image_sna(char *file_name){
	FIL f;
	//int res;
	size_t bytesRead;
	UINT bytesToRead;	
	int fd=0;

	uint8_t im_hw = 0;			  //Тип оборудования для V2 V3
	int pageNumber = 0;
	uint16_t pageSize;
	uint8_t* bufferIn;
	uint8_t* bufferOut;
	
	//printf("load_image_sna\n");
	
	memset(header_buf, 0, sizeof(header_buf));
	
	fd = f_open(&f,file_name,FA_READ);
	//printf("f_open=%d\n",fd);
	if (fd!=FR_OK){f_close(&f);return false;}
	
	
	if (sd_file_size(&f)<SNA_48K_SIZE){
		f_close(&f);return false;
	}
	if (sd_file_size(&f)>SNA_128K_SIZE2){
		f_close(&f);return false;
	} 
	
	bytesToRead = 27;
	
	fd = f_read(&f,header_buf,bytesToRead,&bytesRead);
	
	if (fd!=FR_OK){f_close(&f);return false;}
	if (bytesRead != bytesToRead){f_close(&f);return false;}
	
	FileHeader* header = (FileHeader*)header_buf;

	if (sd_file_size(&f) == SNA_48K_SIZE){
		// version 1
		im_hw = 48;
		zx_RAM_bank_active = 0;
		last_out_7ffd = 0x30;

		//printf("*V48\n");
	} else if (sd_file_size(&f) > SNA_48K_SIZE){
		im_hw = 128;
		//printf("*V128\n");
	} else {
		// Invalid
		im_hw = 128;
		f_close(&f);
		return false;
	}
	
	do{ 
		memset(sd_buffer, 0, sizeof(sd_buffer));
		pageSize=ZX_RAM_PAGE_SIZE;
		bufferIn = (uint8_t *)&sd_buffer;//error
		switch (pageNumber) {
		case 0:
			//printf("Page:5\n");
			bufferOut = &RAM[5*ZX_RAM_PAGE_SIZE];
			break;
		case 1:
			//printf("Page:2\n");
			bufferOut = &RAM[2*ZX_RAM_PAGE_SIZE];
			break;
		case 2:
			if(im_hw == 48){
				//printf("Page:%d\n",zx_RAM_bank_active);
				bufferOut = &RAM[zx_RAM_bank_7ffd*ZX_RAM_PAGE_SIZE];
			} else {
				bufferOut = NULL;
			}	
			break;
		}
		do{ // Read page into tempBuffer and unpack
			if (pageSize>sizeof(sd_buffer)){
				bytesToRead = sizeof(sd_buffer);
			} else {
				bytesToRead = pageSize;
			}

			fd = f_read(&f,bufferIn,bytesToRead,&bytesRead);
           	if (fd != FR_OK){f_close(&f);return false;}
			//printf("*bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
			if (bytesRead != bytesToRead){f_close(&f);return false;}
			if(bufferOut != NULL){
				memcpy(bufferOut,bufferIn,bytesRead);
			}
			bufferOut += bytesRead;
			bufferIn = (uint8_t *)&sd_buffer;//error
			pageSize-=bytesRead;
		}while (pageSize>0);
		pageNumber++;
	}while(pageNumber<3);
	if (im_hw==48){
		//printf("48K\n");
		readCPUstateSNA(header,im_hw);
	    uint16_t SP = Z80_SP(z1->cpu);
		//printf("SP_1:%04x\n",SP);
		Z80_PC(z1->cpu)=read_word_48k(SP);
        Z80_SP(z1->cpu) = SP + 2;
		//printf("PC:%04x,SP:%04x\n",Z80_PC(z1->cpu),Z80_SP(z1->cpu));
	} else if (im_hw==128) {
		//printf("128K\n");
		bytesToRead=4;
		fd = f_read(&f,buf,bytesToRead,&bytesRead);
		if (fd != FR_OK){f_close(&f);return false;}

		//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);            
		if (bytesRead != bytesToRead){f_close(&f);return false;}
		last_out_7ffd = buf[2];
		zx_RAM_bank_7ffd= last_out_7ffd & 0x7;
		zx_RAM_bank_active=zx_RAM_bank_7ffd;
		zx_machine_set_7ffd_out(last_out_7ffd);
		//printf("Page:%d\n",zx_RAM_bank_active);

		fd = f_lseek(&f,sd_file_pos(&f)-ZX_RAM_PAGE_SIZE-4);
		if (fd != FR_OK){f_close(&f);return false;}
		pageSize=ZX_RAM_PAGE_SIZE;
		bufferIn = (uint8_t *)&sd_buffer;//error
		bufferOut = &RAM[zx_RAM_bank_7ffd*ZX_RAM_PAGE_SIZE];
		do{ // Read page into tempBuffer and unpack
			if (pageSize>sizeof(sd_buffer)){
				bytesToRead = sizeof(sd_buffer);
			} else {
				bytesToRead = pageSize;
			}

			fd = f_read(&f,bufferIn,bytesToRead,&bytesRead);
           	if (fd != FR_OK){f_close(&f);return false;}
			//printf("*bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
			if (bytesRead != bytesToRead){f_close(&f);return false;}
			if(bufferOut != NULL){
				memcpy(bufferOut,bufferIn,bytesRead);
			}
			bufferOut += bytesRead;
			bufferIn = (uint8_t *)&sd_buffer;//error
			pageSize-=bytesRead;
		}while (pageSize>0);
		readCPUstateSNA(header,im_hw);
		Z80_PC(z1->cpu)=(buf[1] << 8) |buf[0]; // in 128K mode, recover stored PC
		//printf("PC:%06d,SP:%06d\n",Z80_PC(z1->cpu),Z80_SP(z1->cpu));
		/*
		49179	2		Содержимое регистра PC.
		49181	1		Содержимое порта 7FFDh.
		49182	1		1 если активно ПЗУ TR-DOS, 0 - если нет.
		49183	16384	Оставшиеся страницы памяти.
		*/
        // read remaining pages
		fd = f_lseek(&f,sd_file_pos(&f)+4);
		if (fd != FR_OK){f_close(&f);return false;}


        for (int page = 0; page < 8; page++) {
            if ((page != zx_RAM_bank_7ffd)&&(page != 2) && (page != 5)) {//
				//printf("Load Page: %d\n",page);
				memset(sd_buffer, 0, sizeof(sd_buffer));
				pageSize=ZX_RAM_PAGE_SIZE;
				bufferIn = (uint8_t *)&sd_buffer;//error
				bufferOut = &RAM[page*ZX_RAM_PAGE_SIZE];
				do{ // Read page into tempBuffer and unpack
					if (pageSize>sizeof(sd_buffer)){
						bytesToRead = sizeof(sd_buffer);
					} else {
						bytesToRead = pageSize;
					}
					fd = f_read(&f,bufferIn,bytesToRead,&bytesRead);
        		   	if (fd != FR_OK){f_close(&f);return false;}
					//printf("*bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
					if (bytesRead != bytesToRead){f_close(&f);return false;}
					if(bufferOut != NULL){
						memcpy(bufferOut,bufferIn,bytesRead);
					}
					bufferOut += bytesRead;
					bufferIn =(uint8_t *) &sd_buffer;//error
					pageSize-=bytesRead;
				}while (pageSize>0);				
            }
        }		
	}
	
	f_close(&f);
	return true;

}// Конец процедуры

bool LoadScreenFromSNASnapshot(char *file_name){
	FIL f;
//	int res;
	size_t bytesRead;
	UINT bytesToRead;	
	int fd=0;

	char sound[8];
	uint16_t pageSize = 0;
	uint8_t* bufferIn;
	uint8_t* bufferOut;

	
	//printf("load_screen_sna\n");
	
	memset(header_buf, 0, sizeof(header_buf));
	
	fd = f_open(&f,file_name,FA_READ);
	////printf("f_open=%d\n",fd);
	if (fd!=FR_OK){f_close(&f);return false;}
	
	
	if (sd_file_size(&f)<SNA_48K_SIZE){
		f_close(&f);return false;
	}
	if (sd_file_size(&f)>SNA_128K_SIZE2){
		f_close(&f);return false;
	} 
	
	//printf("Begin read\n");
	bytesToRead = 27;
	
	fd = f_read(&f,header_buf,bytesToRead,&bytesRead);
	//printf("bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
	if (fd!=FR_OK){f_close(&f);return false;}
	if (bytesRead != bytesToRead){f_close(&f);return false;}

	memset(sound, 0, sizeof(sound));
	
	if (sd_file_size(&f) == SNA_48K_SIZE){
		// version 1
		//im_hw = 48;
		zx_RAM_bank_active = 0;
		zx_RAM_bank_7ffd =0;
		strncpy(sound, "Buzzer", 7);
		//readCPUstate(header);
		//printf("*V48\n");
	} else if (sd_file_size(&f) > SNA_48K_SIZE){
		//im_hw = 128;
		zx_RAM_bank_active = 0;
		zx_RAM_bank_7ffd =0;
		strncpy(sound, "AY8910", 7);
		//printf("*V128\n");
	} else {
		// Invalid
		//im_hw = 128;
		f_close(&f);
		return false;
	}
	
	memset(sd_buffer, 0, sizeof(sd_buffer));

	bufferOut = (sizeof(sd_buffer)>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
	bufferIn = (uint8_t *)&sd_buffer;
	pageSize = 0x1B00;
	do{ // Read page into tempBuffer and unpack
		if (pageSize>sizeof(sd_buffer)){
			bytesToRead = sizeof(sd_buffer);
		} else {
			bytesToRead = pageSize;
		}
		fd = f_read(&f,bufferIn,bytesToRead,&bytesRead);
		if (fd != FR_OK){f_close(&f);return false;}
		//printf("*bytesToRead=%d, bytesRead=%d\n",bytesToRead,bytesRead);
		if (bytesRead != bytesToRead){f_close(&f);return false;}
		memcpy(bufferOut,bufferIn,bytesRead);
		bufferIn = (uint8_t *)&sd_buffer;//error
		bufferOut += bytesRead;
		pageSize-=bytesRead;
	}while (pageSize>0);
	//bufferOut = &RAM[5*ZX_RAM_PAGE_SIZE];
	bufferOut = (sizeof(sd_buffer)>0x2000) ? &sd_buffer[0x2000] : &RAM[5*ZX_RAM_PAGE_SIZE];
	ShowScreenshot(bufferOut);

	f_close(&f);
	return true;	
}

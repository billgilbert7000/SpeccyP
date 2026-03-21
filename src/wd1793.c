/*
	Terms:
	  * Sector - a tr-dos sector is 256 bytes length, has number 1-16
	  * Block - minimum amount of bytes on SD/TF card that can be read or written is 512 bytes

*/
//#define  DEBUG
#include "config.h"  
#ifdef TRDOS_1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stdbool.h"
#include "util_sd.h"

#include "ff.h"//#include "VFS.h"

//#include "wd1793.h"
#include "boot.h"

#include "fdi_stream.h"
uint8_t sectors_per_track;

  FDD_CTX fdd = {0};
//#########################################################
//uint8_t DiskBuf[256];
//uint8_t DiskBuf[1024];
#define DiskBuf sd_buffer
//uint32_t Delay; //in timer ticks

//volatile uint32_t Counter_WG93;
//volatile uint8_t  IndexCounter=0;
//bool index_mark =0;
uint64_t old_t;
//uint32_t TimerValue;
uint16_t BufferPos = 0;
uint16_t SectorPos = 0;
uint16_t pos;
uint8_t BufferUpdated = 0;
uint16_t FormatCounter = 0;
uint8_t NewDrive = 5;
uint8_t OldDrive =5;
FIL fileTRD;

FATFS fss;
DIR dir;
FILINFO finfo;
FRESULT res;
uint8_t FilesCount = 0;

unsigned int br;
uint8_t CmdType = 1;
uint16_t CurrentDiskPos = DEFULTDISKPOS; // Big enough to be outside of disk

WD1793_struct WD1793;

uint8_t Requests = 0; // выдает состояние контроллера 
//uint8_t SIDE = 0;
uint8_t DRV = 5;
uint8_t Prevwd1793_PortFF = 0xff;

uint8_t DiskSector;
bool DiskIndex;
uint8_t wd1793_PortFF;

void WD1793_CmdStartIdle();

uint8_t NewCommandReceived = 0;
typedef void (*Command) ();
Command CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
Command NextCommand = NULL; 
Command CommandTable[16] = {
		WD1793_Cmd_Restore, // 00 Restore 	Восстановление 					- %0000 hvrr - +
		WD1793_Cmd_Seek, 	// 10 Seak		Поиск/Позиционирование 			- %0001 hvrr |
		WD1793_Cmd_Step, 	// 20 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
		WD1793_Cmd_Step, 	// 30 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
		WD1793_Cmd_Step, 	// 40 Step In	Шаг вперед                   	- %010t hvrr |
		WD1793_Cmd_Step, 	// 50 Step In	Шаг вперед                   	- %010t hvrr |
		WD1793_Cmd_Step, 	// 60 Step Out	Шаг назад                    	- %011t hvrr - +
		WD1793_Cmd_Step, 	// 70 Step Out	Шаг назад                    	- %011t hvrr - +
		WD1793_Cmd_ReadSector, // 80		Чтение сектора               	- %100m seca
		WD1793_Cmd_ReadSector, // 90		Чтение сектора               	- %100m seca
		WD1793_Cmd_WriteSector, // A0		Запись сектора               	- %101m sec0
		WD1793_Cmd_WriteSector, // B0		Запись сектора               	- %101m sec0
		WD1793_Cmd_ReadAddress, // C0		Чтение адреса                	- %1100 0e00
		WD1793_CmdStartIdle, 	// D0		Принудительное прерывание   	- %1101 iiii
		WD1793_Cmd_ReadTrack, 	// E0		Чтение дорожки               	- %1110 0e00
		WD1793_Cmd_WriteTrack 	// F0		Запись дорожки/форматир-е   	- %1111 0e00
	};
/*
где rr - скорость позиционирования головки: 00 - 6мс, 01 - 12мс, 10 - 20мс, 11 - 30мс (1Mhz).
	v - проверка номера дорожки после позиционирования.
	h - загрузка головки (всегда должен быть в 1!).
	t - изменение номера дорожки в регистре дорожки после каждого шага.
	a - тип адресной метки (0 - #fb, стирание сектора запрещено 1 - #F8, стирание сектора разрешено).
	c - проверка номера стороны диска при идентификации индексной области.
	e - задержка после загрузки головки на 30мс (1Mhz).
	s - сторона диска (0 - верх/1 - низ).
	m - мультисекторная операция.
	i - условие прерывания:
	i0 - после перехода сигнала cprdy из 0 в 1 (готов);
	i1 - после перехода сигнала cprdy из 1 в 0 (не готов);
	i2 - при поступлении индексного импульса;
	i3 - немедленное прерывание команды, при:
	i=0 - intrq не выдается
	i=1 - intrq выдается.
*/
static uint8_t last_track=0xff;
static uint8_t last_side=0xff;

//#############################################################################
bool OpenTRDFile(char *sn,uint8_t  drv){
	
last_track =0xff;
last_side=0xff;

fdd.sector_size = 256;
fdd.data_offset = 0;
fdd.sectors_per_track = 16;
fdd.cylinders = 80;
fdd.heads =2;

	 strcpy(conf.Disks[drv & 0x03],sn);

strncpy(dir_patch_info,conf.Disks[drv & 0x03],(DIRS_DEPTH*(LENF+16)) );

	 f_open(&fileTRD,sn,FA_READ );   


//if (conf.sclConvertde != 0) // ЭТО НЕ SCL файл!
if (file_type[drv] == TRD)
{
// определение на нормальность TRD файла
 if (sd_file_size(&fileTRD)<655360) file_type[drv]=TRDS; // нестандарт
// else file_type[drv]=TRD; // стандарт
}


	DRV =5;
	NewDrive = drv;
	NoDisk = 0;
	return true;
}
//#############################################################################

static fdi_stream_ctx_t fdi_ctx;
static fdi_file_io_t fdi_io = {
    .open = fatfs_open,
    .close = fatfs_close,
    .seek = fatfs_seek,
    .read = fatfs_read,
    .tell = fatfs_tell,
    .size = fatfs_size
};

//


bool OpenFDI_File(char *sn,uint8_t  drv){

file_type[drv] = FDI;
last_track =0xff;
last_side=0xff;
//fdd.sector_size = 256;
fdd.data_offset = 0;
//fdd.sectors_per_track = 16;
//fdd.cylinders = 80;
//fdd.heads =2;

	 strcpy(conf.Disks[drv & 0x03],sn);

strncpy(dir_patch_info,conf.Disks[drv & 0x03],(DIRS_DEPTH*(LENF+16)) );

    fdi_stream_result_t res = fdi_stream_open(&fdi_ctx, &fdi_io, sn);

fdd.cylinders = fdi_ctx.cylinders;
fdd.heads = fdi_ctx.heads;
//fdd.sector_size = 256;


	DRV =5;
	NewDrive = drv;
	NoDisk = 0;
	return true;
}
//#############################################################################
void WD1793_Write(uint8_t Address, uint8_t Value){ // Z80 write to port 
	// 0 - 0x1F  1 - 0x3F  2 - 0x5F  3 - 0x7F  
	switch (Address & 0x03){
		case 0 : // 0x1F Write to Command Register
				Address = 4;
				NewCommandReceived = 1;
				break;
		case 2 :
			//	//printf("S:%d ",Value);
				break;
		case 3 : // 0x7F Write to Data Register 
				 // задание номера логического трека, для команды позиционирования или запись байта если была какая-либо команда на запись.
				WD1793.StatusRegister &= ~_BV(stsDRQ); // Запрос данных
				Requests &= ~_BV(rqDRQ); //When !WR
				break;
	}

	WD1793.Regs[Address] = Value; // 1 - 0x3F 2-0x5F 3-0x7F 4-0x1F
	
}

uint8_t WD1793_Read(uint8_t Address){ // Z80 read from port 
	Address &= 0x03;
	switch (Address){
		case 0 : // 0x1F Read from Status Register
				 // выдает состояние контроллера, где A:
				Requests &= ~_BV(rqINTRQ);
		break;
		case 3 : // 0x7F Read from Data Register
				 // чтение байта
				WD1793.StatusRegister &= ~_BV(stsDRQ); // Запрос данных
				Requests &= ~_BV(rqDRQ); //When !RD
				break;
	}

	return WD1793.Regs[Address];
}

//==========================================================================
/* void WD1793_timer(uint32_t dtcpu)
{					// Таймер для TR-DOS
return;
	Counter_WG93 += dtcpu; // количество тактов Z80
					//	Counter_WG93++;// количество попугаев
		//	printf("Counter_WG93: %d\n",dtcpu);		
	if (Counter_WG93 >= INDEX_COUNTER_TIME)
	{
		//printf("Counter_WG93: %d\n",Counter_WG93);	
		Counter_WG93 = Counter_WG93 - INDEX_COUNTER_TIME;
		if ((++IndexCounter > REVOLUTION_TIME))
		{
			IndexCounter = 0;
		}
		if (IndexCounter == REVOLUTION_TIME)
		{
			Counter_WG93 = 58752; // correction for 5rps, 200 ms per revolution
		}
	}
} */
//=====================================================================================
uint16_t HasTimePeriodExpired_old(uint16_t period){
	static uint16_t t = 0;	
	if(t >=  period){
		t =0;
		return 1;
	}
	t++;
	return 0;
}
// с реальным временем
uint16_t HasTimePeriodExpired(uint64_t period)
{
	static uint64_t t = 0;
	t = (time_us_64() - old_t);
	if (t >= period)
	{
		old_t = time_us_64(); // предыдущее время
		return 1;
	}
	return 0;
}
//========================================================================================
void printError(char* msg, uint8_t code){

           //Reading error:
                      msg_bar=18;//ERR TR:
                      wait_msg = 3000; 
	//printf(msg);
/* 	switch(code){
		case FR_DISK_ERR:
		printf("FR_DISK_ERR\n");
		break;
		case FR_NOT_READY:
		printf("FR_NOT_READY\n");
		break;
		case FR_NO_FILE:
		printf("FR_NO_FILE\n");
		break;
		//case FR_NOT_OPENED:
		//printf("FR_NOT_OPENED\n");
		//break;
		case FR_NOT_ENABLED:
		printf("FR_NOT_ENABLED\n");
		break;
		case FR_NO_FILESYSTEM:
		printf("FR_NO_FILESYSTEM\n");
		break; 
	}*/
}

void WD1793_FlushBuffer(){
	
	if (file_type[0] == FDI) return;

	if (BufferUpdated==1)
	{ 
			
          //    msg_bar=15;//WRITE TR:
          //   wait_msg = 3000; 
				
res = f_open(&fileTRD,(const char *)conf.Disks[DRV],FA_READ | FA_WRITE);
	
 
		f_lseek(&fileTRD, pos << 8); // return to start of sector on SD Card

		res = f_write(&fileTRD,DiskBuf, BUFFERSIZE, &br);

	res =f_sync(&fileTRD);// да это сработало! синхронизация файлов на SD
	       
		}

	BufferUpdated = 0;
		

}

// position is address 
uint32_t ComputeDiskPosition(uint8_t Sector, uint8_t Track, uint8_t Side)
{
		switch (file_type[DRV])
		{
		case SCL:
			return (((uint16_t)Track * fdd.heads + (uint16_t)Side) * fdd.sectors_per_track + (uint16_t)Sector);
			break;
		case TRD:
		case TRDS:
			return (((uint16_t)Track * fdd.heads + (uint16_t)Side) * fdd.sectors_per_track + (uint16_t)Sector);
//		case FDI:
//			return (((uint16_t)Track * fdd.heads + (uint16_t)Side) * fdd.sectors_per_track + (uint16_t)Sector);			
		default:
			return 0;
		}
}

void SetDiskPosition(uint16_t pos)
{

if (file_type[0] == FDI) return;

	f_lseek(&fileTRD, (uint32_t)pos << 8);
	res = f_read(&fileTRD, DiskBuf, fdd.sector_size, &br);
	if (res != 0)
		printError("Reading error: ", res);
	CurrentDiskPos = pos;
}
//============================================================
void SetDiskPositionSCL(uint16_t pos)
{
	SCL_read_sector();
	CurrentDiskPos = pos;
}
//============================================================
void WD1793_Reset(uint8_t drive){
	
	if (file_type[0] == FDI) 
	{

	//	fdi_stream_open(&fdi_ctx, &fdi_io, (const char *)conf.Disks[0]);
		NoDisk = 0;
		return;
	}
	
	uint16_t i, prev_space;
	uint8_t d, j; // drive index in config

	WD1793_FlushBuffer();
	res =f_sync(&fileTRD);// да это сработало! синхронизация файлов на SD
	CurrentDiskPos = DEFULTDISKPOS;
	res = f_open(&fileTRD,(const char *)conf.Disks[drive & 0x03],FA_READ );
   
	if (res){
		NoDisk = 1;
	//	printError("Error open: ", res);
	} else {
		NoDisk = 0;
	}	

}

/* void WD1793_CmdIdle()
{
	return;////////////
	switch (CmdType)
	{
	case 1:
		if (!NoDisk && WD1793_GetIndexMark()) // Speccy checks rotation
		{
			WD1793.StatusRegister |= _BV(stsIndexImpuls);
		}
		else
		{
			WD1793.StatusRegister &= ~_BV(stsIndexImpuls);
		}
		break;
	}
} */
void WD1793_CmdIdle()
{


 if (NoDisk==1)	return;

	switch (CmdType)
	{
	 case 1:
         // index every turn, len=4ms (if disk present)
		 
		if ( ((time_us_64()-tindex)<(200000-4000))) // 4ms = 4000 µs  checks rotation 20000 us 50 Гц <4001
		{
			WD1793.StatusRegister |= _BV(stsIndexImpuls);//set bit to 1
				
		}
		else
		{
			WD1793.StatusRegister &= ~_BV(stsIndexImpuls);//set  bit to 0
			DiskSector=0;
		}

         if ( ((time_us_64()-tindex)>(200000)))tindex = time_us_64();//???

        break;

	}  

}

//############################################################################
void WD1793_CmdStartIdle(){  // D0		Принудительное прерывание %1101 iiii
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	Requests &= ~_BV(rqDRQ); // CmdStartIdle clear   
	Requests |= _BV(rqINTRQ);

	WD1793_FlushBuffer();
	tindex = time_us_64();
	CurrentCommand = WD1793_CmdIdle;

}
//################################################################################

void WD1793_CmdStartIdleForceInt0(){
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	//Requests &= ~_BV(rqDRQ); // CmdStartIdleForcent0 clear  как не странно после того как убрал эдесь unreal работает проверить на scorpion с родным ПЗУ
	Requests &= ~_BV(rqINTRQ);
	
	WD1793_FlushBuffer();
	tindex = time_us_64();
	CurrentCommand = WD1793_CmdIdle;
}

void WD1793_CmdType1Status(){

	if (WD1793.RealTrack == 0)
	{
	WD1793.StatusRegister |= _BV(stsTrack0);
	
	}

	if(WD1793.CommandRegister & 0x08)// head load
	{ 
		WD1793.StatusRegister |= _BV(stsLoadHead);
	}

	CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	
}

void WD1793_CmdDelay()
{
//	if (!HasTimePeriodExpired(Delay)) return;
//if (!HasTimePeriodExpired_old(32)) return;



	CurrentCommand = NextCommand;
}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void SCL_read_sector(void)
{
		
  if (( WD1793.TrackRegister == 0) && (WD1793.Side == 0)) // если Track = 0  и Side = 0
        {
			
			if (WD1793.SectorRegister < 10) //   меньше 9 сектора 123456789 нулевого сектора нет
			{// копирование нулевой дорожки из буфера
				 int seekptr = (WD1793.SectorRegister-1) << 8;
                for (int i = 0; i < 256; i++)
                    DiskBuf[i] = track0[seekptr + i];
			}

            if (WD1793.SectorRegister == 10) //   10 сектор копирование boot
            {
                for (int i = 0; i < 256; i++) DiskBuf[i] = boot256[i];    
            }


            if (WD1793.SectorRegister > 11)
            {
                for (int i = 0; i < 256; i++)
                   DiskBuf[i] = 0;
            }
 		}		
			else		
				{
			    f_lseek(&fileTRD, (uint32_t)((( pos-16) << 8)+ sclDataOffset) );
				res = f_read(&fileTRD, DiskBuf, BUFFERSIZE, &br);
				if (res) printError("Reading error: ", res);	
				}
}				
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void WD1793_CmdReadingSectorSCL()
{
if(!HasTimePeriodExpired(BYTE_READ_TIME)) return;  // pause
if(SectorPos != 0 && Requests & _BV(rqDRQ))
	{
		WD1793.StatusRegister |= _BV(stsLostData);	
	}

	if (SectorPos >= fdd.sector_size){
	 	if(WD1793.Multiple)
		{
			if(BufferPos != 0 )  SCL_read_sector();
			SectorPos = 0;
			if(++WD1793.RealSector > fdd.sectors_per_track)
			{
				CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
				return;
			}
			CurrentCommand = WD1793_CmdStartReadingSector;
			return;
		}
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
	        

	WD1793.DataRegister = DiskBuf[BufferPos];//чтение байта из сектора
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	BufferPos++;
	SectorPos++;
}
//-------------------------------------------------------------------------
void WD1793_CmdReadingSectorTRD(){
 	if(!HasTimePeriodExpired(BYTE_READ_TIME)) return;  // 32us 

	 if(SectorPos != 0 && Requests & _BV(rqDRQ))
	 {
	 	WD1793.StatusRegister |= _BV(stsLostData);	
	 }

	if (SectorPos >= fdd.sector_size){
		if(WD1793.Multiple)
		{
		if(BufferPos != 0 )
			{
	l_povtor:	
	    	res = f_read(&fileTRD, DiskBuf, fdd.sector_size, &br);
	               if (res) goto l_povtor;
			//	if (res) printError("Reading error: ", res);	
			}
			SectorPos = 0;
			if(++WD1793.RealSector > fdd.sectors_per_track){
				CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
				return;
			}
			CurrentCommand = WD1793_CmdStartReadingSector;
			return;
		}
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
	        

	WD1793.DataRegister = DiskBuf[BufferPos];
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	BufferPos++;
	SectorPos++;
}
//-------------------------------------------------------------------------
// Чтение сектора для FDI
bool FDI_read_sector(uint8_t track, uint8_t side, uint8_t sector_num)
 {
	
    // Загружаем трек (если не загружен или другой)

  //  gpio_put(LED_BOARD, 1);
    if (last_track != track || last_side != side) 
	{
        if (fdi_stream_load_track(&fdi_ctx, track, side) != FDI_STREAM_OK)  return false;
        last_track = track;
        last_side = side;
    }
    
    // Получаем информацию о секторе
    fdi_sector_info_t *sec = fdi_stream_get_sector(&fdi_ctx, sector_num);
    if (!sec) return false;
	fdd.sector_size = sec->size;
	fdd.sector_n = sec->n;
	fdd.sector_f = sec->flags;
    // Читаем данные сектора
    if (fdi_stream_read_sector_data(&fdi_ctx, sec, DiskBuf, fdd.sector_size) != FDI_STREAM_OK) return false;

    // Для эмуляции WD1793 важны флаги
    if (sec->flags & FDI_FL_DELETED_DATA) {
        // Сектор с удаленными данными (метка F8)
        // wd1793_set_deleted_data(true);
    }
    
    uint8_t crc_bit = 1 << (sec->n & 3);
    if (!(sec->flags & crc_bit)) {
        // Ошибка CRC
        // wd1793_set_crc_error(true);
    }
    

    return true;
}

//-------------------------------------------------------------------------
void WD1793_CmdReadingSectorFDI(){
if(!HasTimePeriodExpired(BYTE_READ_TIME)) return;  // pause
if(SectorPos != 0 && Requests & _BV(rqDRQ))
	{
		WD1793.StatusRegister |= _BV(stsLostData);	
	}

	if (SectorPos >= fdd.sector_size){
	 	if(WD1793.Multiple)
		{
			if(BufferPos != 0 ) FDI_read_sector(WD1793.TrackRegister, WD1793.Side, WD1793.SectorRegister);
			SectorPos = 0;
			if(++WD1793.RealSector > sectors_per_track ) // если больше чем секторов есть
			{
				CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
				return;
			}  
			CurrentCommand = WD1793_CmdStartReadingSector;
			return;
		}
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}        
	WD1793.DataRegister = DiskBuf[BufferPos];//чтение байта из сектора
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	BufferPos++;
	SectorPos++;
}
//-------------------------------------------------------------------------
void WD1793_CmdReadingSector()
{
	switch (file_type[DRV])
	{
	case SCL:
		WD1793_CmdReadingSectorSCL();
		return;
	case TRD:
	case TRDS:
		WD1793_CmdReadingSectorTRD();
	case FDI:
		WD1793_CmdReadingSectorFDI();	
		return;
	}
}
//###############################################################
void WD1793_CmdWritingSector()
{
	if (!HasTimePeriodExpired_old(BYTE_WRITE_TIME ))
		return; // 32us
	if (Requests & _BV(rqDRQ))
	{
		WD1793.StatusRegister |= _BV(stsLostData);
	}
	if (!(WD1793.StatusRegister & _BV(stsLostData)))
	{
		DiskBuf[BufferPos] = WD1793.DataRegister;
		
	}
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
//************************************************ */
// read-only!
int msg_code = 0;

if (file_type[DRV] == SCL) {
    msg_code = 10;  // SCL files Read Only!
}
else if (file_type[DRV] == TRDS) {
    msg_code = 10;  // TRD the file is non-standard Read Only
}
else if (file_type[DRV] == FDI) {
    msg_code = 10;  // TRD the file is non-standard Read Only
}
else if ((file_attr[DRV] & AM_RDO) == AM_RDO) {
    msg_code = 10;  // This file is Read Only
}

if (msg_code != 0) {
    WD1793.StatusRegister = 0b01000000; // OpWrProt защита записи
    CurrentCommand = WD1793_CmdStartIdle; // Вызов D0 Принудительное прерывание - %1101 iiii
   // msg_bar = msg_code;
	msg_bar = 10;  // This file is Read Only
    wait_msg = 3000;
    return;
}

	/******************************* */

	BufferPos++;
	SectorPos++;

	if (SectorPos >= fdd.sector_size)
	{
		BufferUpdated = 1;// включить запись на SD
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	}
}
//#######################################################################################
// форматирование
void WD1793_CmdWritingTrack() // Calling from WD1793_Cmd_WriteTrack() //F0 base command
{
//************************************************ */
// read-only!
int msg_code = 0;

if (file_type[DRV] == SCL) {
    msg_code = 10;  // SCL files Read Only!
}
else if (file_type[DRV] == TRDS) {
    msg_code = 11;  // TRD the file is non-standard Read Only
}
else if ((file_attr[DRV] & AM_RDO) == AM_RDO) {
    msg_code = 12;  // This file is Read Only
}

if (msg_code != 0) {
    WD1793.StatusRegister = 0b01000000; // OpWrProt защита записи
    CurrentCommand = WD1793_CmdStartIdle; // Вызов D0 Принудительное прерывание - %1101 iiii
    msg_bar = msg_code;
    wait_msg = 3000;
    return;
}
/********************************/
	if ((WD1793.TrackRegister == 0) && (WD1793.Side == 0))
	{
		if (!HasTimePeriodExpired_old(BYTE_WRITE_TIME)) return;		   // 32us	
	}

	//else
	
		BufferUpdated = 0; // отключить запись на SD при форматировании всё равно там нули пишутся

	if (Requests & _BV(rqDRQ))
	{
		WD1793.StatusRegister |= _BV(stsLostData);
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}

	if (WD1793.DataRegister == 0x4E)
		FormatCounter++;
	else
		FormatCounter = 0;

	if (FormatCounter > 400)
	{
		if ((WD1793.TrackRegister == 0) && (WD1793.Side == 0))// реально нужно обнулить только Tr 0
			  {
		      BufferUpdated = 1; // 1 включить запись на SD при форматировании  стирание 0 дорожки
              memset(DiskBuf,0,256); // стирание буфера
			  }
	        else
		      BufferUpdated = 0; // отключить запись на SD при форматировании всё равно там нули пишутся
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
//	DiskBuf[BufferPos] = 0;//????
//	BufferPos++;
//	if (BufferPos>256) BufferPos=0;

//memset(DiskBuf,0,256); // стирание буфера


	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
}
//------------------------------------------------------------------------
uint8_t sector_table[] = {1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16};
//------------------------------------------------------------------------	
void ReadingAddressFDI() // Команда чтения адреса по стандарту FDI
{	
	      uint8_t crc_bit = 1 << (fdd.sector_f & 3);
    DiskSector = (DiskSector+1)&0x0f;

	if((SectorPos != 0) && Requests & _BV(rqDRQ))
	{
		//if (err_timeout--) return;
		WD1793.StatusRegister |= _BV(stsLostData);
	}	 

	if (BufferPos >= 6){
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
		
	switch(BufferPos){
		case 0: // N track (0-128)
	//WD1793.DataRegister = WD1793.TrackRegister+GET_SIDE();  // для проверки что двухсторонний диск при форматировании
       WD1793.DataRegister = WD1793.TrackRegister;  // для нормальной работы процедуры
	//	WD1793.SectorRegister =  WD1793.TrackRegister;
		break;
		       
		case 1: // N side always 0
		WD1793.DataRegister = WD1793.Side;
		break;
		       
		case 2: // N sector (0-128)

	//	WD1793.DataRegister =  sector_table[DiskSector]; // вычисленный сектор из таблицы
       WD1793.DataRegister =  WD1793.SectorRegister; //   сектор  
	
	//  WD1793.DataRegister = DiskSector+1;// номера секторов по порядку
	 //  DiskSector = (DiskSector+1)&0x0f;
	
	//  DiskSector = (DiskSector+1); if (DiskSector>16)  DiskSector =0;
	//	WD1793.DataRegister = 1;
	//	WD1793.SectorRegister = sector_table[DiskSector];; // узнать сектор 
	//	WD1793.DataRegister =  WD1793_GetCurrentSector(); // узнать сектор по текущему времени

		break;
		       
		case 3: // sector size 0-128 1-256 2-512 3-1024
		WD1793.DataRegister = fdd.sector_n; // 
		break;
		       
		case 4: // checksum

		/* if (fdd.sector_f & FDI_FL_GOOD_CRC_1024) WD1793.DataRegister = 0x00;
        else  */
       //  sector->crc wd93_crc_simple(buffer,sector->size)



    if (!(fdd.sector_f & crc_bit)) {
        // Ошибка CRC - данные читаются, но флаг ошибки устанавливается
        WD1793.DataRegister = 0x00;;  // Бит 3 = CRC error
	}
           else

		WD1793.DataRegister = 0x00; 

		case 5:

    if (!(fdd.sector_f & crc_bit)) {
        // Ошибка CRC - данные читаются, но флаг ошибки устанавливается
        WD1793.DataRegister = 0x00;;  // Бит 3 = CRC error
	}
           else

		WD1793.DataRegister = 0xff; 
		break;
	}
}
//----------------------------------------------------------------------
void ReadingAddressTRD() // Команда чтения адреса по стандарту TRD
{	
    DiskSector = (DiskSector+1)&0x0f;

	if((SectorPos != 0) && Requests & _BV(rqDRQ))
	{
		//if (err_timeout--) return;
		WD1793.StatusRegister |= _BV(stsLostData);
	}	 

	if (BufferPos >= 6){
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
		
	switch(BufferPos){
		case 0: // N track (0-128)
	//WD1793.DataRegister = WD1793.TrackRegister+GET_SIDE();  // для проверки что двухсторонний диск при форматировании
       WD1793.DataRegister = WD1793.TrackRegister;  // для нормальной работы процедуры
	//	WD1793.SectorRegister =  WD1793.TrackRegister;
		break;
		       
		case 1: // N side always 0
		WD1793.DataRegister = 0;
		break;
		       
		case 2: // N sector (0-128)
    	   WD1793.DataRegister =  sector_table[DiskSector]; // вычисленный сектор из таблицы
	 //  WD1793.DataRegister = DiskSector+1;// номера секторов по порядку
	 //  DiskSector = (DiskSector+1)&0x0f;
	
	//  DiskSector = (DiskSector+1); if (DiskSector>16)  DiskSector =0;
	//	WD1793.DataRegister = 1;
	//	WD1793.SectorRegister = sector_table[DiskSector];; // узнать сектор 
	//	WD1793.DataRegister =  WD1793_GetCurrentSector(); // узнать сектор по текущему времени

		break;
		       
		case 3: // sector size 0-128 1-256 2-512 3-1024
		WD1793.DataRegister = 1; // 256 bytes
		break;
		       
		case 4: // checksum
		case 5:
		WD1793.DataRegister = 0;
		break;
	}
}
//-------------------------------------
void WD1793_CmdReadingAddress()
{
	if(!HasTimePeriodExpired(ADDRESS_READ_TIME)) return; // 32us 
 
	if (file_type[DRV]==FDI) ReadingAddressFDI();
    else ReadingAddressTRD();

	BufferPos++;
	Requests |= _BV(rqDRQ); // Set rqDRQ
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных

}

void WD1793_Cmd_Restore(){ // 00 Restore 	Восстановление 					- %0000 hvrr - +
	WD1793.TrackRegister = 0;
	WD1793.RealTrack = 0;
	WD1793.Direction = 0;
	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_Cmd_Seek(){// 10 Seak		Поиск/Позиционирование 			- %0001 hvrr |
	if (WD1793.TrackRegister > WD1793.DataRegister)
	WD1793.Direction = 1;
	else
	WD1793.Direction = 0;

	WD1793.TrackRegister = WD1793.DataRegister;
	WD1793.RealTrack =  WD1793.DataRegister;

	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_Cmd_Step(){// 20 30 40 50 60 70 Step		Шаг в предыдущем направлении 	- %001t hvrr + - 
	switch (WD1793.CommandRegister & 0xF0){ // Command Decoder
		case 0x40:
		case 0x50: // Step In
		WD1793.Direction = 0;
		break;

		case 0x60:
		case 0x70: // Step Out
		WD1793.Direction = 1;
		break;
	}
	
	if (WD1793.Direction == 0 && WD1793.TrackRegister < 128){ // 80 увеличение до 128 дорожек на сторону
		WD1793.TrackRegister++;
	}
	if (WD1793.Direction == 1 && WD1793.TrackRegister > 0){
		WD1793.TrackRegister--;
	}

	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}


void WD1793_CmdStartReadingSector()
{
	            //  msg_bar=16;//READ TR:
                //  wait_msg = 3000; 
	CurrentCommand = WD1793_CmdReadingSector;
	
}

//-------------------------------------------------------------------------------------
void WD1793_Cmd_ReadSector() // 80	90	Чтение сектора               	- %100m seca
{ 
	switch (file_type[DRV])
{    
	case SCL:
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, WD1793.Side);
	WD1793.RealSector = WD1793.SectorRegister;
	BufferPos = 0;// Position in buffer
	SetDiskPositionSCL(pos);							 // пример [332]R:512 512 pos:0
	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
	return;

	case TRD:
	case TRDS:
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, WD1793.Side);
	WD1793.RealSector = WD1793.SectorRegister;
	BufferPos = 0;// Position in buffer
	SetDiskPosition(pos);							 // пример [332]R:512 512 pos:0
	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
    return;

	case FDI:
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	// вычисление позиции в файле для FDI не нужно там подругому
	WD1793.RealSector = WD1793.SectorRegister;// номер сектора
	BufferPos = 0;// Position in buffer
    // читаем один сектор
    FDI_read_sector(WD1793.TrackRegister, WD1793.Side, WD1793.SectorRegister);
	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
	return;
}	
}
//--------------------------------------------------------------------------------------------------------------------------------------
void WD1793_CmdStartWritingSector(){

	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	CurrentCommand = WD1793_CmdWritingSector;
	     //     msg_bar=15;//WRITE TR:
         //     wait_msg = 3000; 
}

void WD1793_Cmd_WriteSector(){// A0 B0		Запись сектора               	- %101m sec0
	WD1793.Multiple = WD1793.CommandRegister & 0x00;
	CmdType = 2;
	pos = ComputeDiskPosition(WD1793.SectorRegister-1, WD1793.TrackRegister, WD1793.Side);
	BufferPos = (pos & (BUFFERSIZEFACTOR-1)) << 8;
	SetDiskPosition(pos);
	SectorPos = 0;
	NextCommand = WD1793_CmdStartWritingSector;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 2;	

}

void WD1793_Cmd_ReadAddress(){// C0		Чтение адреса                	- %1100 0e00
//printf("[ReadAddress]RD  TrackRegister %d WD1793.DataRegister %d \n", WD1793.TrackRegister,WD1793.DataRegister);
	BufferPos = 0;
	CurrentCommand = WD1793_CmdReadingAddress; //
	CmdType = 3;
	
}
// для TRD и SCL не реализованно
void WD1793_Cmd_ReadTrack(){// E0		Чтение дорожки               	- %1110 0e00
	CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	CmdType = 3;
}

void WD1793_CmdStartWritingTrack(){ //Calling from WD1793_Cmd_WriteTrack() //F0 base command
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	CurrentCommand = WD1793_CmdWritingTrack;
}

void WD1793_Cmd_WriteTrack(){// F0		Запись дорожки/форматир-е   	- %1111 0e00
	BufferPos = 0;
	SectorPos = 0;
	FormatCounter = 0;
	CurrentCommand = WD1793_CmdStartWritingTrack;
	CmdType = 3;
}

void WD1793_CmdStartNewCommand(){
	uint8_t i = WD1793.CommandRegister >> 4;

	WD1793_FlushBuffer();
	if (DRV != NewDrive){ // Mount image only if drive is changed
		DRV = NewDrive;
		WD1793_Reset(DRV);
	}
	CommandTable[i]();

}
 /*	
bit out(PortFF)
7 - x
6 - метод записи (1 - fm, 0 - mfm)
5 - x
4 - выбор магнитной головки (0 - верх, 1 - низ)
3 - загрузка головки (всегда должен быть в 1)
2 - аппаратный сброс микроконтроллера (если 0)
1 - номер дисковода (00 - a, 01 - b, 10 - c, 11 - d)
0 - -/-
 */
//####################################################################################
void WD1793_Execute(void){
	
//	printf("[ReadSector] SEC %i, TRK %i, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, WD1793.Side, (uint32_t) (pos<<8));	

//WD1793_GetIndexMark();

if(Prevwd1793_PortFF != wd1793_PortFF){
	Prevwd1793_PortFF = wd1793_PortFF;
	//printf("wd1793_PortFF: %x\n",wd1793_PortFF);
		NewDrive = GET_DRIVE();  // (wd1793_PortFF & 0b11)
		WD1793.Side = GET_SIDE(); 		 // (~wd1793_PortFF & 0x000010000) >> 4
     //   NoDisk = 1;
//printf("NewDrive: %x %x \n",NewDrive, DRV);

}


	if(NewCommandReceived){
	//	printf("NewCR: %02X\n",WD1793.CommandRegister);

		NewCommandReceived = 0; // try to avoid disabling interrupts
			if ((WD1793.CommandRegister & 0xF0) == 0xD0){ // Force Interrupt
			////printf("Force Interrupt\n");	// In power up 2 demo can freeze 3d part at the end
			WD1793.StatusRegister &= ~_BV(stsBusy); // Cброс занятости
			CmdType = 1; // Exception
			CurrentCommand = WD1793.CommandRegister & 0x0F ? WD1793_CmdType1Status : WD1793_CmdStartIdleForceInt0;
		} else 
		if(!(WD1793.StatusRegister & _BV(stsBusy))){
			WD1793.StatusRegister = 0x01; // All bits clear but "Busy" set
			Requests = 0; // clear DRQ and INTRQ
			
			CurrentCommand = WD1793_CmdStartNewCommand;
		}

	}
	sound_track =WD1793.TrackRegister ; // звук перемешения головки ffd

   CurrentCommand();

   
}
//#######################################################################
void WD1793_Init(void){
   old_t = time_us_64()+999;// текущее время	
    wd1793_PortFF =0b00001000;
	Prevwd1793_PortFF = 0xff;// 0xff;//wd1793_PortFF;
	NewDrive = 0 ;//  // (wd1793_PortFF & 0b11)
		WD1793.Side = 0;// // (~wd1793_PortFF & 0x000010000) >> 4
		NoDisk = 0;
	Requests = 0; // clear DRQ and INTRQ
	tindex = time_us_64()+1000;;//???
}

#endif
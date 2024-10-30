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

#include <VFS.h>

#include "wd1793.h"
#include "boot.h"


//#define  MIN(a,b) ((a<b)?(a):(b))


uint8_t DiskBuf[256];
//uint32_t Delay; //in timer ticks

//volatile uint32_t Counter_WG93;
//volatile uint8_t  IndexCounter=0;
//bool index_mark =0;

//uint32_t TimerValue;
uint16_t BufferPos = 0;
uint16_t SectorPos = 0;
uint16_t pos;
uint8_t BufferUpdated = 0;
uint16_t FormatCounter = 0;
uint8_t NewDrive = 5;
uint8_t OldDrive =5;
FIL fTRD;

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
uint8_t SIDE = 0;
uint8_t DRV = 5;
uint8_t Prevwd1793_PortFF = 0xFF;

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

bool OpenTRDFile(char *sn,uint8_t  drv){
	printf("Selected TRDOS image: %s\n",sn);
//	memcpy(disk,sn,160);
	 strcpy(conf.Disks[drv & 0x03],sn);
	//memcpy(Path,file_name,3);
	 sd_open_file(&fTRD,sn,FA_READ | FA_WRITE);   
//sd_close_file(&fTRD);
	DRV =5;
	NewDrive = drv;
	NoDisk = 0;
	return true;
}

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
  uint8_t WD1793_GetIndexMark_E(){
      static	uint8_t    DskIndexCounter=1;
	  DskIndexCounter--;
           if ((DskIndexCounter & 0x0E) > 0) return 1;//*DataIO |= 0x02; //0x0000 0010
		return 0;   
	  if (DskIndexCounter==0)//250
	  { 
	//	DskIndexCounter== 0;
		return 0;
	  }
	//  DskIndexCounter== 1;
       return 1;

	//return IndexCounter; 
}    
  uint8_t WD1793_GetIndexMarkQQQQQQQQ(){//!!!!!!!!!!!!!!!!!
      static	uint8_t    DskIndexCounter=2;
	  DskIndexCounter--;
  //   return (DskIndexCounter & 0x01);


	  if (DskIndexCounter==0)//250
	  { 
		DskIndexCounter= 128;//2
		return 0;
	  }
	//  DskIndexCounter= 1;
       return 1;

	//return IndexCounter; 
}    
 uint8_t WD1793_GetIndexMark0(){
    //  static	uint64_t    t0=0;
	  static	uint64_t    t1=0;
	//uint64_t  t0=(time_us_64()-t1);

	uint64_t  t0=(time_us_64()-old_t);
	  
	if(t0 >=  200000)  // 5 Гц  200000 микросекунд
	{
	//	t1 = time_us_64();// предыдущее время
old_t = time_us_64();// предыдущее время
		return 0;
	  }
	  
       return 1;

	//return IndexCounter; 
}    
//

 uint8_t WD1793_GetIndexMark(){
	  static	uint64_t    t1=0;
      
	  if (t2<250) 
	  {
		t2++;
		t1 = time_us_64();// предыдущее время
		return 0;
	  }
	 
	uint64_t  t0=(time_us_64()-t1);

//	uint64_t  t0=(time_us_64()-old_t);
	
	if(t0 <  2000)  // 5 Гц  200000 микросекунд
	{
	//	t1 = time_us_64();// предыдущее время
//old_t = time_us_64();// предыдущее время
		return 1;
	  }
	  t1 = time_us_64();// предыдущее время
       return 0;

	//return IndexCounter; 
}   



//================================================================
/* void StartTimePeriod(){ // Задание нового тамера с нуля
	TimerValue = Counter_WG93;
} */
//===============================================================
/* uint16_t HasTimePeriodExpired(uint16_t period){
	//uint32_t t = GetTimerValue();
	if(Counter_WG93 - TimerValue >= period){
		TimerValue = Counter_WG93;
		return 1;
	}
	return 0;
} */
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

//================================================================
/* 
uint16_t GetTimerValue()
{
	uint16_t value;
	//cli();
	value = Counter_WG93;
	//sei();
	return value;
} */
//==========================================================================
/* uint8_t WD1793_GetCurrentSector()
{
	uint8_t sector;
	
	sector = ((IndexCounter << 8) | (GetTimerValue()>>8))/92U;
	
	
	if(sector > 15)
	{
		sector = 15;
	}
	
	// 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16
	return ((sector & 1) << 3 | (sector >> 1)) + 1; 
} */
//===========================================================================

/* uint8_t WD1793_GetCurrentSector(){ // узнать сектор по текущему времени
	uint8_t sector;
//	uint32_t get_time = GetTimerValue();
	//cli();
//	sector = ((IndexCounter << 8) | (get_time>>8))/92U; //
    sector = (IndexCounter << 8) ;

	////printf("[274]sec:%d IndC:%d GTV:%d\n", sector, IndexCounter, get_time);
	//sei();
 	if(sector > 15){
		sector = 15;
	} 
	////printf("[242]GetCurSec %d\n",sector, ((sector & 1) << 3) | (sector >> 1) + 1);
	// 1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16
		return ((sector & 1) << 3 | (sector >> 1)) + 1; 
}
   */

//========================================================================================
void printError(char* msg, uint8_t code){

	//printf(msg);
	switch(code){
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
	}
}

void WD1793_FlushBuffer(){
	

	if (BufferUpdated)
	{ // flush partially filled buffer

       if (file_type==1) // scl disk A
         {
            
			 WD1793.StatusRegister = 0b01000000; // OpWrProt защита записи
             return;
         }



     res = f_open(&fTRD,(const char *)conf.Disks[DRV],FA_READ | FA_WRITE);
if (res){
			printError("[354]Open file error: ", res);
			return;
		} 

		//sd_seek_file(&fTRD, (uint32_t)CurrentDiskPos << 8); // return to start of sector on SD Card
		sd_seek_file(&fTRD, pos << 8); // return to start of sector on SD Card
	//	sleep_ms(10);
	//	res = sd_write_file(&fTRD, DiskBuf, BUFFERSIZE, &br);
        res =f_write(&fTRD,DiskBuf,BUFFERSIZE,br);

		if (res){
			printError("[253]Writing error: ", res);
			return;
		}
/*   		sd_write_file(&fTRD, 0, 0, &br);
		if (res){
			printError("[259]Writing error on finish: ", res);
			return;
		}     */

		res =f_sync(&fTRD);// да это сработало! синхронизация файлов на SD
	
		BufferUpdated = 0;
	//	printf("[264]Flushed buffer\n");
	}
}

// position is address/256
uint16_t ComputeDiskPosition(uint8_t Sector, uint8_t Track, uint8_t Side){


 if (file_type == 1) // SCL
 { 
	uint16_t addr = (((uint16_t)Track * DISKSIDES + (uint16_t)Side) * SECTORPERTRACK + (uint16_t)Sector);
	
  // уменьшаем номер трека на 1 так как первый трек берется из буфера
// 4096kb  1 трек
 //addr = ( addr + sclDataOffset);// -4096;
//printf("[274] CompDiskPos Addr 0x%04X_%04X   %d\n", (uint16_t)(addr>>8), (uint16_t)(addr<<8),sclDataOffset);
//printf("[275] CompDiskPos Addr: %d sclDataOffset: %d\n", (uint16_t)(addr),sclDataOffset);
   return addr;
 }
else
 //if (file_type == 0) //TRD
 { 
	uint16_t addr = (((uint16_t)Track * DISKSIDES + (uint16_t)Side) * SECTORPERTRACK + (uint16_t)Sector);
//	printf("[274] CompDiskPos Addr 0x%04X_%04X\n", (uint16_t)(addr>>8), (uint16_t)(addr<<8));
   return addr;
 }


}
/* void SetDiskPosition(uint16_t pos){
	if(pos < CurrentDiskPos || pos >= (CurrentDiskPos + BUFFERSIZEFACTOR)){
		pos &= ~(BUFFERSIZEFACTOR-1); // round position to buffer size
		if(pos != (CurrentDiskPos + BUFFERSIZEFACTOR)){ // move position if only it is needed but it saves only 100 mcs
			f_lseek(&fTRD, (uint32_t)pos << 8);
		}
		res = sd_read_file(&fTRD, DiskBuf, BUFFERSIZE, &br);
		if (res !=0){
			printError("Reading error: ", res);					
		}
		//printf("\n[332]R:512 %d pos:%d\n",br,pos);
		CurrentDiskPos = pos;
	}
	////printf("[297]SetDiskPos %d\n",pos);
} */
void SetDiskPosition(uint16_t pos){
			f_lseek(&fTRD, (uint32_t)pos << 8);
	    	res = sd_read_file(&fTRD, DiskBuf, BUFFERSIZE, &br);
	    	if (res !=0) printError("Reading error: ", res);	
	        CurrentDiskPos = pos;	
            }
//============================================================
void SetDiskPositionSCL(uint16_t pos){

//printf("[SCL]RD SEC %d, TRK %d, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos));
                        SCL_read_sector();

		CurrentDiskPos = pos;
	}

//============================================================
void WD1793_Reset(uint8_t drive){
	uint16_t i, prev_space;
	uint8_t d, j; // drive index in config

    res = vfs_init(); 	

	WD1793_FlushBuffer();
	res =f_sync(&fTRD);// да это сработало! синхронизация файлов на SD
	CurrentDiskPos = DEFULTDISKPOS;
	res = f_open(&fTRD,(const char *)conf.Disks[drive & 0x03],FA_READ | FA_WRITE);
	if (res){
		NoDisk = 1;
		printError("Error open: ", res);
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
if (NoDisk)	return;
	switch (CmdType)
	{
	case 1:
         // index every turn, len=4ms (if disk present)
		if ( ((time_us_64()-tindex)<4001)) // 4ms = 4000 µs  checks rotation 20000 us 50 Гц
		{
			WD1793.StatusRegister |= _BV(stsIndexImpuls);
			
		}
		else
		{
			WD1793.StatusRegister &= ~_BV(stsIndexImpuls);
		}
		break;

	}

}

//===================================================================
void WD1793_CmdStartIdle(){  // D0		Принудительное прерывание   	- %1101 iiii
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	Requests &= ~_BV(rqDRQ); // CmdStartIdle clear
	Requests |= _BV(rqINTRQ);
	
	//printf("---[539]Lost data, cmd=%02X\n", WD1793.CommandRegister);
	//printf("[7777]RD:  TRK %d SEC %d, \n", WD1793.TrackRegister, WD1793.SectorRegister );
 	//if(CmdType != 1 && WD1793.StatusRegister & _BV(stsLostData)){
 	//	printf("---[541]Lost data, cmd=%02X\n", WD1793.CommandRegister);		
 	//}
	
	WD1793_FlushBuffer();
	tindex = time_us_64();
	CurrentCommand = WD1793_CmdIdle;
}


void WD1793_CmdStartIdleForceInt0(){
	WD1793.StatusRegister &= ~_BV(stsBusy); // clear bit
	Requests &= ~_BV(rqDRQ); // CmdStartIdleForcent0 clear
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
		
  if (( WD1793.TrackRegister == 0) && (SIDE == 0)) // если Track = 0  и Side = 0
        {
			
			if (WD1793.SectorRegister < 10) //   меньше 9 сектора 123456789 нулевого сектора нет
			{// копирование нулевой дорожки из буфера
				 int seekptr = (WD1793.SectorRegister-1) << 8;
                for (int i = 0; i < 256; i++)
                    DiskBuf[i] = track0[seekptr + i];
			}

            if (WD1793.SectorRegister == 10) //   10 сектор копирование boot
            {
                for (int i = 0; i < 256; i++) DiskBuf[i] = boot512[i];    
            }

              if (WD1793.SectorRegister == 11) //  11 сектор копирование boot
            {
                for (int i = 0; i < 256; i++) DiskBuf[i] = boot512[256 + i];    
            }
            if (WD1793.SectorRegister > 12)
            {
                for (int i = 0; i < 256; i++)
                   DiskBuf[i] = 0;
            }
 		}		
			else		
				{
			    f_lseek(&fTRD, (uint32_t)((( pos-16) << 8)+ sclDataOffset) );
				res = sd_read_file(&fTRD, DiskBuf, BUFFERSIZE, &br);
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
		//	printf("[SCL]RD SEC %d, TRK %d, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos<<8));	
		//	printf("[7777]RD:  TRK %d SEC %d, \n", WD1793.TrackRegister, WD1793.SectorRegister );
	}

	if (SectorPos >= SECTORLENGTH){
	 	if(WD1793.Multiple)
		{
		//	if(BufferPos != 0 && !(BufferPos)) 
			if(BufferPos != 0 )
			{
			//	printf("[SCL]RD SEC %d, TRK %d, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos));

                    SCL_read_sector();

			}
			SectorPos = 0;
			if(++WD1793.RealSector > SECTORPERTRACK){
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
void WD1793_CmdReadingSectorTRD(){
 	if(!HasTimePeriodExpired(BYTE_READ_TIME)) return;  // 32us 

	 if(SectorPos != 0 && Requests & _BV(rqDRQ))
	 {
	 	WD1793.StatusRegister |= _BV(stsLostData);	
	 }

	


	if (SectorPos >= SECTORLENGTH){
	//	printf("[TRD]RD  WD1793.Multiple) %d, SectorPos %x\n", WD1793.Multiple, SectorPos);
	//	printf("[TRD]RD SEC %d, TRK %d, SIDE %i, Multiple %x\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, WD1793.Multiple);
		if(WD1793.Multiple)
		{
	//		if(BufferPos != 0 && !(BufferPos))
		if(BufferPos != 0 )
			{
			//	printf("[TRD1]RD SEC %d, TRK %d, SIDE %i, Multiple %x\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, WD1793.Multiple);
				res = sd_read_file(&fTRD, DiskBuf, BUFFERSIZE, &br);
				if (res) printError("Reading error: ", res);	
			}
			SectorPos = 0;
			if(++WD1793.RealSector > SECTORPERTRACK){
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
void WD1793_CmdReadingSector(){

  //  if ((conf.sclConverted == 0) && (GET_DRIVE() == 0)) // если SCL файл на диске A:
  if ((conf.sclConverted == 0)) // если SCL файл на диске A:
	  {
         file_type = 1;
		WD1793_CmdReadingSectorSCL();
		return;
	  }
    else
      {
		file_type = 0;
		WD1793_CmdReadingSectorTRD();
		return;
	  }

}
//---------------------------------------------------------------
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

	
	
	BufferUpdated = 1;
	BufferPos++;
	SectorPos++;

	if (SectorPos >= SECTORLENGTH)
	{
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	}
}

// форматирование
void WD1793_CmdWritingTrack()  //Calling from WD1793_Cmd_WriteTrack() //F0 base command
{
//	if(!HasTimePeriodExpired_old(BYTE_WRITE_TIME )) //*3 96us 
//		return;

   if (WD1793.TrackRegister==0)
	  {
		BufferUpdated = 1;//включить запись на SD при форматировании  стирание 0 дорожки
        if (!HasTimePeriodExpired_old(BYTE_WRITE_TIME ))
		return; // 32us

	 }
	
      else BufferUpdated = 0;// отключить запись на SD при форматировании всё равно там нули пишутся

		
	if(Requests & _BV(rqDRQ))
	{
		WD1793.StatusRegister |= _BV(stsLostData);
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}	
	
	if(WD1793.DataRegister == 0x4E){
		FormatCounter++;
	} else {
		FormatCounter  = 0;
	}
	
	if (FormatCounter  > 400){


  /*         if (WD1793.TrackRegister==0)
		  {
			memset(sd_buffer,0,0x0900); // стирание 9 секторов
			res = f_open(&fTRD,(const char *)conf.Disks[DRV],FA_READ | FA_WRITE);
		sd_seek_file(&fTRD, 0); // return to start of sector on SD Card
		//	res = sd_write_file(&fTRD, sd_buffer, 0x0900, &br);
       res =f_write(&fTRD,sd_buffer,0x0900,br);
		res = f_sync(&fTRD);
		

//return;

		  }
 */
BufferUpdated = 1;
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
		return;
	}
	
	//DiskBuf[BufferPos] = WD1793.DataRegister;//
	DiskBuf[BufferPos] = 0;//
   //  if (WD1793.TrackRegister==0)
	//  {
	//BufferUpdated = 1;//включить запись на SD при форматировании  стирание 0 дорожки
	//  }
	//  else BufferUpdated = 0;//1 отключить запись на SD при форматировании всё равно там нули пишутся

	BufferPos++;
	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных

//BufferUpdated = 0;
}
//------------------------------------------------------------------------
uint8_t sector_table[] = {1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16};
//------------------------------------------------------------------------	        
void WD1793_CmdReadingAddress(){
	if(!HasTimePeriodExpired(ADDRESS_READ_TIME)) // 32us 
 //if(!HasTimePeriodExpired_old(250)) // 32us 
	return;
DiskSector = (DiskSector+1)&0x0f;

	if((SectorPos != 0) && Requests & _BV(rqDRQ))
	{
		//if (err_timeout--) return;
		WD1793.StatusRegister |= _BV(stsLostData);
	}	 

	if (BufferPos >= 6){
		CurrentCommand = WD1793_CmdStartIdle; // Вызов D0		Принудительное прерывание   	- %1101 iiii
	//printf("[ReadingAddress]RD  TrackRegister %d WD1793.DataRegister %d \n", WD1793.TrackRegister,WD1793.DataRegister);
//	printf("[TRD]RD SEC %d, TRK %d, SIDE %i, Multiple %x\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, WD1793.Multiple);
		return;
	}
		
	switch(BufferPos){
		case 0: // N track (0-128)
	//WD1793.DataRegister = WD1793.TrackRegister+GET_SIDE();  // для проверки что двухсторонний диск при форматировании
       WD1793.DataRegister = WD1793.TrackRegister;  // для нормальной работы процедуры
	//	WD1793.SectorRegister =  WD1793.TrackRegister;

	//printf("[ReadingAddres]RD  TRK %d\n", WD1793.TrackRegister);


		break;
		       
		case 1: // N side always 0
		WD1793.DataRegister = 0;
	//printf("[ADR] TRK %d, SIDE %i = %ld\n",  WD1793.TrackRegister, GET_SIDE(), (uint32_t) (pos));
		break;
		       
		case 2: // N sector (0-128)
	//	WD1793.DataRegister = WD1793.SectorRegister-1; // узнать сектор 
	//    WD1793.DataRegister =DiskSector+1; // вычисленный сектор по времкени индексного отверстия диска
	 
	   WD1793.DataRegister = sector_table[DiskSector]; // вычисленный сектор из таблицы
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

	BufferPos++;
	Requests |= _BV(rqDRQ); // Set rqDRQ
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных

}

void WD1793_Cmd_Restore(){ // 00 Restore 	Восстановление 					- %0000 hvrr - +
	WD1793.TrackRegister = 0;
	WD1793.RealTrack = 0;
	WD1793.Direction = 0;

//	Delay = STEP_TIME; // 3ms 6000 in procedure WD1793_Cmd_Restore()
	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}

void WD1793_Cmd_Seek(){// 10 Seak		Поиск/Позиционирование 			- %0001 hvrr |
	if (WD1793.TrackRegister > WD1793.DataRegister)
	WD1793.Direction = 1;
	else
	WD1793.Direction = 0;

//	Delay = abs(WD1793.TrackRegister - WD1793.DataRegister) * 800;// 1400;
//	printf("[Seek]RD  TrackRegister %d WD1793.DataRegister %d \n", WD1793.TrackRegister,WD1793.DataRegister);
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
	////printf("[1018]Step %s to track %i\n", WD1793.Direction ? "in" : "out", WD1793.TrackRegister);
	//Delay = STEP_TIME; // 3ms 6000 in procedure WD1793_Cmd_Step()
	NextCommand = WD1793_CmdType1Status;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 1;
}


void WD1793_CmdStartReadingSector()
{
	CurrentCommand = WD1793_CmdReadingSector;
	
}

//-------------------------------------------------------------------------------------
void WD1793_Cmd_ReadSector() // 80	90	Чтение сектора               	- %100m seca
{ 
    if (file_type==1)
{
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, SIDE);
	WD1793.RealSector = WD1793.SectorRegister;

	BufferPos = 0;// Position in buffer
	SetDiskPositionSCL(pos);							 // пример [332]R:512 512 pos:0

	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
	
}

	else 
	{
	WD1793.Multiple = WD1793.CommandRegister & 0x10;
	pos = ComputeDiskPosition(WD1793.SectorRegister - 1, WD1793.TrackRegister, SIDE);
	WD1793.RealSector = WD1793.SectorRegister;

	BufferPos = 0;// Position in buffer
	SetDiskPosition(pos);							 // пример [332]R:512 512 pos:0

	SectorPos = 0;
	CurrentCommand = WD1793_CmdStartReadingSector;
	CmdType = 2;
	
	}
}
//--------------------------------------------------------------------------------------------------------------------------------------
void WD1793_CmdStartWritingSector(){

	Requests |= _BV(rqDRQ);
	WD1793.StatusRegister |= _BV(stsDRQ); // Запрос данных
	CurrentCommand = WD1793_CmdWritingSector;
	
}

void WD1793_Cmd_WriteSector(){// A0 B0		Запись сектора               	- %101m sec0
	WD1793.Multiple = WD1793.CommandRegister & 0x00;
	CmdType = 2;
	pos = ComputeDiskPosition(WD1793.SectorRegister-1, WD1793.TrackRegister, SIDE);
	BufferPos = (pos & (BUFFERSIZEFACTOR-1)) << 8;
	SetDiskPosition(pos);
	SectorPos = 0;
	//Delay = BYTE_READ_TIME; // 32us 
	NextCommand = WD1793_CmdStartWritingSector;
	CurrentCommand = WD1793_CmdDelay;
	CmdType = 2;	
	
	//printf("[1115]WR SEC %i, TRK %i, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos<<8));

}

void WD1793_Cmd_ReadAddress(){// C0		Чтение адреса                	- %1100 0e00
//printf("[ReadAddress]RD  TrackRegister %d WD1793.DataRegister %d \n", WD1793.TrackRegister,WD1793.DataRegister);
	BufferPos = 0;
	CurrentCommand = WD1793_CmdReadingAddress; //
	CmdType = 3;
	
}

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
		//	printf("CommandTable: %d\n",i);
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
void WD1793_Execute(void){
	
//	printf("[ReadSector] SEC %i, TRK %i, SIDE %i - %ld\n", WD1793.SectorRegister, WD1793.TrackRegister, SIDE, (uint32_t) (pos<<8));	

//WD1793_GetIndexMark();

if(Prevwd1793_PortFF != wd1793_PortFF){
	Prevwd1793_PortFF = wd1793_PortFF;
	//printf("wd1793_PortFF: %x\n",wd1793_PortFF);
		NewDrive = GET_DRIVE();  // (wd1793_PortFF & 0b11)
		SIDE = GET_SIDE(); 		 // (~wd1793_PortFF & 0x000010000) >> 4
        NoDisk = 0;
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

void WD1793_Init(void){
   old_t = time_us_64();// текущее время	
    wd1793_PortFF =0b00001000;
	Prevwd1793_PortFF = 0xff;//wd1793_PortFF;
	NewDrive = 0 ;//  // (wd1793_PortFF & 0b11)
		SIDE = 0;// // (~wd1793_PortFF & 0x000010000) >> 4
		NoDisk = 0;
	
}

#endif
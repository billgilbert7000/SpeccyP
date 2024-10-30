#pragma once

#ifndef _WD1793_H_
#define _WD1793_H_

#include <stdint.h>
//=====================================================
#define  SECTORLENGTH		256U
#define  SECTORPERTRACK		16U
#define  DISKSIDES			2U
#define  S_SIZE_TO_CODE(x)  ((((x)>>8)|(((x)&1024)>>9)|(((x)&1024)>>10))&3)
#define  S_CODE_TO_SIZE(x)  (1<<(7+(x)))

#define  BUFFERSIZE 0x0100 //0x2000//512U
#define  BUFFERSIZEFACTOR (BUFFERSIZE/SECTORLENGTH)
#define  DEFULTDISKPOS 0xFFFFU

//#define  REVOLUTION_TIME 6U //6U in .h
//#define  TIME_PER_SECTOR 1U //1U

//Один такт Z80 = 0.2857142857142857 us
// boot  ENLIGHT очень чувствителен к этому числу если оно маленькое!
#define  BYTE_READ_TIME 	32//64 32      32uS    112 тактов Z80 64U

#define ADDRESS_READ_TIME   32//32

#define  BYTE_WRITE_TIME 	96//       32uS    112 тактов Z80 64U

//#define  STEP_TIME 32//  10500//	10500   // 3ms 3000uS  10500 тактов Z80 6000U
//#define  INDEX_COUNTER_TIME 2500//100000  //    28570uS 100000 тактов Z80

//#define START_TIME_PERIOD TimerValue = Counter_WG93 // Задание нового тамера с нуля
//=====================================================
uint64_t old_t;

// Status Bits
#define stsBusy            0
#define stsDRQ             1
#define stsIndexImpuls     1
#define stsTrack0          2
#define stsLostData        2 
#define stsCRCError        3
#define stsSeekError       4
#define stsRecordNotFound  4
#define stsLoadHead        5
#define stsRecordType      5
#define stsWriteError      5
#define stsWriteProtect    6
#define stsNotReady        7


#define rqDRQ   6
#define rqINTRQ 7

#define _BV(bit) (1 << (bit))

typedef struct
{
	union {
		uint8_t Regs[5];
		struct {
			uint8_t StatusRegister;
			uint8_t TrackRegister; // 0x3F 	- in/out in - текущий физический трек       / out - задание нового физического трека дорожки 
			uint8_t SectorRegister;// 0x5F  - in/out in - выдает номер текущего сектора / out - задание нового сектора (нумерация с 1!).
			uint8_t DataRegister;  // 0x7F	- in/out in - чтение байта / out -  задание номера логического трека, для команды позиционирования или запись байта если была какая-либо команда на запись.
										
			uint8_t CommandRegister; // 0x1F in - выдает состояние контроллера, где A: / out - выполнение команды, где A - код команды (см. раздел команды).
		};
	};
	
	uint8_t RealSector;      
	uint8_t RealTrack;        
	uint8_t Direction;        
	uint8_t Side;
	uint8_t Multiple;
}  WD1793_struct;


void WD1793_Reset(uint8_t drive);
void WD1793_Execute();
void WD1793_Init();


bool TRDOS_mode ;		// Информационный сигнал Текущий режим - ROM TRDOS или стандартный ROM 48k

uint8_t WD1793_Status;

extern uint8_t Requests;

inline uint8_t WD1793_GetRequests() // 7th bit - INTRQ, 6th - DRQ
{
	return Requests;
}
uint8_t wd1793_PortFF;
uint8_t NoDisk;
uint8_t file_type;

#define GET_DRIVE() (wd1793_PortFF & 0b11)
#define SIDE_PIN 4
#define GET_SIDE() ((~wd1793_PortFF & _BV(SIDE_PIN))>>SIDE_PIN)

#define GET_RES()  (wd1793_PortFF & _BV(RES_PIN)) 

void WD1793_Cmd_Restore();
void WD1793_Cmd_Seek();
void WD1793_Cmd_Step();
void WD1793_Cmd_ReadSector();
void WD1793_Cmd_WriteSector();
void WD1793_Cmd_ReadAddress();
void WD1793_Cmd_ReadTrack();
void WD1793_Cmd_WriteTrack();
void WD1793_CmdStartReadingSector();
//
void diskindex_callback();
//void WD1793_DiskIndex();
uint8_t DiskSector;
bool DiskIndex;
//
WD1793_struct WD1793;
uint8_t NewCommandReceived;

uint8_t WD1793_Read(uint8_t Address);
void WD1793_Write(uint8_t Address, uint8_t Value);
//void WD1793_timer(uint32_t dtcpu);
//void WD1793_IndexCounter(void);

bool OpenTRDFile(char* sn, uint8_t  drv);
//uint8_t GetWD1793_Status();

void SCL_read_sector(void);
#endif // _WD1793_H_

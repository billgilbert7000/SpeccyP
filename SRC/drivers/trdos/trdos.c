#include "config.h"
#ifdef TRDOS_0 

#include "trdos.h"
#include "util_sd.h"
#include <stdint.h>
#include <string.h>
#include "pico/stdlib.h"
#include "stdbool.h"
#include <VFS.h>
#include <ff.h>

#include "boot.h"

uint8_t FormatDatLen = 1;
bool Ready = true;
bool ReadIndexTrack = true;
//uint8_t DskStatus = 0;


const uint8_t DskIndex[5] = {0, 1, 1, 0, 0};
uint8_t DiskBuf[256];
char DskFileName[160];
FIL hTRDFile;



uint8_t DskIndexCounter;
uint8_t DskCountDatLost;
int16_t CntData;
int16_t DskDataSize;
uint16_t CntReady;
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
//NewDrive = GET_DRIVE();  // (wd1793_PortFF & 0b11)


int16_t GetFA(char *S) {
    return 0x20;
}
//---------- позиция в файле TRD --------------------------
uint32_t DiskPosition(uint16_t Sect) {
    uint32_t res;
   res = (((VG.RegTrack << 5) | ((VG.System & 0x10) ^ 0x10) | Sect) << 8);
  //        трек                    side                       сектор
    return res;
}
//---------- позиция в файле SCL --------------------------
uint32_t DiskPosSCL(uint16_t Sect) {
    uint32_t res;
   res = (((VG.RegTrack << 5) | ((VG.System & 0x10) ^ 0x10) | Sect) << 8);
  // уменьшаем номер трека на 1 так как первый трек берется из буфера
// 4096kb  1 трек
res = (res + sclDataOffset) -4096;
//res = res-4096;
    return res;
}
//---------------------------------------------------------------------------
void SetStateStep() {
    //printf("SetStateStep\n");
    if (VG.RegTrack < 0) {
        VG.TrackReal[VG.System & 0x03] = 0;
        VG.RegTrack = 0;
    }
    if (VG.RegTrack > 0x7f) { ///0x4F количество дорожек на стороне
        VG.TrackReal[VG.System & 0x03] = 0x7f;
        VG.RegTrack = 0x7f;
    }

    VG.RegStatus = DskStatus & 0x40;
    if (VG.RegCom & 0x08) {
        VG.RegStatus |= 0x20;
    }
    if (VG.RegTrack == 0) {
        VG.RegStatus |= 0x04;
    }
}

void OpCom() {
    //printf("OpCom\n");
    DskCountDatLost = 0x00;
    CntData = 0x0000;
    VG.RegStatus = 0x03;
    if (!Ready) {
        VG.RegStatus = 0x01;
        CntReady = 0x10;
    }
}

void StepRear() {
    //printf("StepRear\n");
    if (VG.RegTrack > 0) {
        VG.RegTrack--;
        VG.TrackReal[VG.System & 0x03] = VG.RegTrack;
    }
    VG.StepDirect = -1;
    SetStateStep();
}
//===============================================================================
void OpReadSector() // чтение сектора
{

static uint16_t pause_read = 0xff;
//if (pause_read == VG.RegSect  ) return;
//pause_read = VG.RegSect;
     //         printf("INFO #5f Tr:%d Sc:%d \n", VG.RegTrack,VG.RegSect ); 


    size_t numread;
    uint8_t fd;
    uint32_t Position;

    if ((VG.RegSect > 0x10) || (VG.RegTrack != VG.TrackReal[VG.System & 0x03]) || !(DskStatus & 0x04))
    {
        VG.RegStatus = 0x10; // OpNotFound
        return;
    }
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    if ((conf.sclConverted == 0) && ((VG.System & 0x03) == 0)) // если SCL файл на диске A:
    {
        if ((VG.RegTrack == 0) && ((VG.System & 0x10) == 0x10)) // если Track = 0  и Side = 1
        {
            if (VG.RegSect < 10) //   меньше 9 сектора 123456789 нулевого сектора нет
            {
                int seekptr = (VG.RegSect - 1) << 8;
                for (int i = 0; i < 256; i++)
                    DiskBuf[i] = track0[seekptr + i];
                   
            }
            
            if (VG.RegSect == 10) //   меньше 9 сектора 123456789 нулевого сектора нет
            {
                for (int i = 0; i < 256; i++) DiskBuf[i] = boot512[i];    
            }
              if (VG.RegSect == 11) //   меньше 9 сектора 123456789 нулевого сектора нет
            {
                for (int i = 0; i < 256; i++) DiskBuf[i] = boot512[256 + i];    
            }
            if (VG.RegSect > 11)
            {
                for (int i = 0; i < 256; i++)
                    DiskBuf[i] = 0;
            }
         
            numread = sizeof(DiskBuf);
        }
        else

        {
            Position = DiskPosSCL(VG.RegSect - 1);
            sd_seek_file(&hTRDFile, Position);
            memset(DiskBuf, 0, sizeof(DiskBuf)); //???????
            fd = sd_read_file(&hTRDFile, DiskBuf, sizeof(DiskBuf), &numread);
           
        }
     //   if (numread != sizeof(DiskBuf))
     //   {
     //       VG.RegStatus = 0x08; // OpContSum
     //   }
     //   else
       if (fd) // ошибка
        {
            VG.RegStatus = 0x08; // OpContSum
       }
        else

        {
            DskDataSize = 0xFF;
            VG.RegData = DiskBuf[0];
            OpCom();
        }
    }
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    else // если TRD файл
    {
        Position = DiskPosition(VG.RegSect - 1);
        sd_seek_file(&hTRDFile, Position);
     //   memset(DiskBuf, 0, sizeof(DiskBuf)); //???????
       // fd = sd_read_file(&hTRDFile, DiskBuf, sizeof(DiskBuf), &numread); // тут была ошибка R I A
          fd = f_read(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numread);

    //  printf("READ Tr:%d Sc:%d Len:%d Err:%d\n", VG.RegTrack,VG.RegSect ,numread, fd);

        if (fd) 
        {
            VG.RegStatus = 0x08; // OpContSum
          //   printf("READ Tr:%d Sc:%d Len:%d Err:%d\n", VG.RegTrack,VG.RegSect ,numread, fd);
        }
        else
        {

         //   printf("READ Tr:%d Sc:%d Len:%d Err:%d\n", VG.RegTrack,VG.RegSect ,numread, fd);
            DskDataSize = 0xFF;
            VG.RegData = DiskBuf[0];
       //     VG.RegSect++; 
       //     if (VG.RegSect >16) VG.RegSect =1;
            OpCom();
        }
    }
}
//==========================================================================
void OpWriteSector() {
       if ((write_protected==true) && ((DskStatus&0x03)==0)) // scl disk A
         {
             VG.RegStatus = 0b01000000; // OpWrProt защита записи
             return;
         }

    if (DskStatus & 0x40) {
        VG.RegStatus = 0x40; // OpWrProt
        return;
    }

   // printf("WRITE %X\n",VG.RegTrack);
    if ((VG.RegSect > 0x10) || (VG.RegTrack != VG.TrackReal[VG.System & 0x03]) || !(DskStatus & 0x04)) {
        VG.RegStatus = 0x10; // OpNotFound
    } else {
        DskDataSize = 0x0100;
        OpCom();
    }
}

//------------------------------------------------------------------------------
void __not_in_flash_func(ChipVG93)(uint8_t OperIO, uint8_t *DataIO){
    size_t numread;
    size_t numwritten;
    uint16_t i;
    uint16_t attr;
    uint16_t IntrqDrq;
    uint8_t Intrq;
    uint8_t Drq;
    uint8_t fd;

    //printf("OperIO=%d DataIO=%d\n",OperIO,*DataIO);
   // sound_track =VG.RegTrack &0x01;   // 
     sound_track =VG.RegTrack ; 


    if ((VG.RegStatus & 0x01) > 0) {
        DskCountDatLost--;
        if (DskCountDatLost == 0) VG.RegStatus = 0x04;
    }
    if (CntReady > 0) {
        CntReady--;
        if (CntReady == 0) VG.RegStatus = 0x03;
    }
 //printf("OperIO=%x DataIO=%x\n",OperIO,(*DataIO>> 4));
    switch (OperIO) {
//-------------------- #1F in
        case 0x00: // восстановление
//*DataIO = 0b00000010; // No Disk
//return;
           // conf.Disks[drv & 0x03],sn);
           if (DskFileName[0]==0)
            {
               *DataIO = 0b00000010; // No Disk
               return;
            } 
            *DataIO = VG.RegStatus;
            if ((VG.RegCom & 0x80) > 0) return;
            DskIndexCounter--;
           if ((DskIndexCounter & 0x0E) > 0) *DataIO |= 0x02; //0x0000 0010
         
            break;
 //----------- #1F
        case 0x01:

            if ((VG.RegStatus & 0x01) == 0) {

                if ((*DataIO & 0xF0) == 0xD0) return;
 
                VG.RegCom = *DataIO;

                switch (VG.RegCom >> 4) {
                    case 0x00:
                        VG.TrackReal[VG.System & 0x03] = 0;
                        VG.RegTrack = 0;
                        VG.StepDirect = -1;
                        SetStateStep();
                        break;

                    case 0x01:
                        VG.TrackReal[VG.System & 0x03] = VG.RegData;
                        VG.RegTrack = VG.RegData;
                        if ((VG.RegData - VG.RegTrack) < 0) VG.StepDirect = -1;
                        if ((VG.RegData - VG.RegTrack) > 0) VG.StepDirect = 1;
                        SetStateStep();
                        break;

                    case 0x02:
                    case 0x03:
                    case 0x04:
                    case 0x05:
                        if (((VG.RegCom >> 4) == 2 || (VG.RegCom >> 4) == 3) && (VG.StepDirect == -1)) {
                            StepRear();
                            return;
                        }
                        if (VG.RegTrack > 0) {
                            VG.RegTrack++;
                            VG.TrackReal[VG.System & 0x03] = VG.RegTrack;
                        }
                        VG.StepDirect = 1;
                        SetStateStep();
                        break;

                    case 0x06:
                    case 0x07:
                        StepRear();
                        break;

                    case 0x08:// чтение сектора

                        OpReadSector();
                    //     VG.RegSect++;//добавленно 29.04.2024
                        if (VG.RegSect>15) VG.RegSect =  16;//добавленно 29.04.2024
                    //    printf("INFO 0x08 Tr:%d Sc:%d \n", VG.RegTrack,VG.RegSect ); 
                        break;

                    case 0x09:// чтение сектора мульти

                        OpReadSector();
                        VG.RegSect++;
                        if (VG.RegSect>15) VG.RegSect =  16;// =15//добавленно 29.04.2024

                        break;

                    case 0x0A:
                    case 0x0B:
                        OpWriteSector();
                        break;

                    case 0x0C:
                        DskDataSize = 0x0005;
                        VG.RegData = 1;
                        if (ReadIndexTrack) VG.RegData = VG.RegTrack;
                        
                        OpCom();
                        break;


                    case 0x0E:
                        break;

                    case 0x0F:
                        if ((DskStatus & 0x40) > 0)
                            VG.RegStatus = 0x40; //OpWrProt
                        else {
                            DskDataSize = FormatDatLen;
                            OpCom();
                        }
                        break;
                }
            }
            break;//case 0x01:
//  #1F end-------------------------------------------
//----------------------------------------------------
//   #3F  Номер дорожки
        case 0x02: // чтение
            *DataIO = VG.RegTrack;
 
            break;

        case 0x03: // запись                              
            if ((VG.RegStatus & 0x01)==0) VG.RegTrack = *DataIO;
            break;
//-----------------------------------------------------------------------
// #5f Номер сектора
        case 0x04: // чтение
        *DataIO = VG.RegSect;

        break;

        case 0x05: // запись
            if ((VG.RegStatus & 0x01)==0) VG.RegSect = *DataIO;   
            break;
//------------------------------------------------------------------------
//  #7f  
         case 0x06:
            if ((VG.RegStatus & 0x01) > 0) {
                switch (VG.RegCom >> 4) {
                    case 0x08:       
                    case 0x09: 
                    {
                     //   printf("INFO 0x08 Tr:%d Sc:%d \n", VG.RegTrack,VG.RegSect );  
                        *DataIO = VG.RegData;
                        DskCountDatLost = 0x10;
                        if (CntData < DskDataSize) {
                            CntData++;
                            VG.RegData = DiskBuf[CntData];
                        } else {
                            if ((VG.RegCom & 0x10) > 0) {
                                VG.RegSect++;
                                OpReadSector();
                            } else {
                                VG.RegStatus = 0x00; // OpOk
                            }
                        }
                        break;
                    }
                    case 0x0C: {
                        *DataIO = VG.RegData;
                        DskCountDatLost = 0x10;
                        if (CntData < DskDataSize) {
                            VG.RegData = DskIndex[CntData];
                            CntData++;
                        } else {
                            
                            VG.RegStatus = 0x00; // OpOk
                        }
                        break;
                    }
                }
            } else {
                *DataIO = VG.RegData;
            }
            break;
//---------------------------------------------------------------------
        case 0x07:// out #7f
          if ((VG.RegStatus & 0x01) > 0) {
                switch (VG.RegCom >> 4) {
                    case 0x0A:
                    case 0x0B: {
                        DiskBuf[CntData] = *DataIO;
                        VG.RegData = *DataIO;
                        CntData++;
                        DskCountDatLost = 0x10;
                        if (CntData >= DskDataSize) {
                            sd_seek_file(&hTRDFile,DiskPosition(VG.RegSect - 1));
                            fd = sd_write_file(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numwritten);
                            if (numwritten != sizeof(DiskBuf)) {
                                VG.RegStatus = 0x08; // OpContSum
                                return;
                            }
                            if ((VG.RegCom & 0x10) > 0) {
                                VG.RegSect++;
                                OpWriteSector();
                            } else {
                                VG.RegStatus = 0x00; // OpOk
                            }
                        }
                        break;
                    }
//-------------------------------------------------------------------------------
// #FF                    
                    case 0x0F: {
                        VG.RegData = *DataIO;
                        DskCountDatLost = 0x10;
                        CntData++;
                        if (CntData >= DskDataSize) {
                            memset(DiskBuf, 0, sizeof(DiskBuf));
                            if ((DskStatus & 0x04) > 0) {
                                sd_seek_file(&hTRDFile,DiskPosition(0));
                                for (int i = 0; i < 0x10; i++) {
                                    fd = sd_write_file(&hTRDFile,DiskBuf,sizeof(DiskBuf),&numwritten);
                                }
                            } else {
                                fd = sd_open_file(&hTRDFile,DskFileName,FA_READ | FA_WRITE);
                                if (fd!=FR_OK) {
                                    VG.RegStatus = 0x00; // OpOk
                                    return;
                                }
                                for (int i = 0; i < 0xA00; i++) { 
                                    fd = sd_write_file(&hTRDFile, DiskBuf, sizeof(DiskBuf), &numwritten);
                                }
                                DskStatus = DskStatus | 0x04;
                              //  sd_close_file(&hTRDFile);
                            }
                            VG.RegStatus = 0x00; // OpOk
                        }
                        break;
                    }
                }
            } else {
                VG.RegData = *DataIO;
            }
            break;
//---------------------------------------
// in ff
        case 0x08:
            IntrqDrq = 0xBF;// 1011 1111
            if ((VG.RegStatus & 0x01) > 0) // 0b 0000 0001
             {
                IntrqDrq = 0x3F;// 0011 1111
            }
            if ((VG.RegStatus & 0x02) > 0) //0b 0000 0010
            {
                IntrqDrq = 0x7F;// 0111 1111
            }
            *DataIO = IntrqDrq;
            break;

        case 0x09:
            if (((VG.RegStatus & 0x01) > 0) && ((VG.RegCom & 0x80) > 0)) {
                VG.RegStatus = 0x08; // OpContSum
                return;
            }
            VG.System = *DataIO;
            if (((VG.System ^ DskStatus) & 0x03) > 0) {
                ChipVG93(0x0B, DataIO);
                ChipVG93(0x0A, DataIO);
            }
            break;

        case 0x0A:
            if ((DskStatus & 0x80) > 0) return;
                
         
             DskStatus = VG.System & 0x03;
       
           strcpy(DskFileName,conf.Disks[DskStatus]);
             
 
            attr = GetFA(DskFileName);
             if (attr != -1) 
            {
                attr &= 0x01;
                DskStatus |= attr << 6;

              fd = sd_open_file(&hTRDFile,DskFileName,FA_READ | FA_WRITE);
             
                if (fd==FR_OK) {
                    DskStatus =(DskStatus | 0x04);
                 //   sd_close_file(&hTRDFile);
                }
            }
            DskStatus = (DskStatus | 0x80);
            break;

        case 0x0B:
            if ((DskStatus & 0x80) == 0) {
                return;
            }
            if ((DskStatus & 0x04) > 0) {
                sd_close_file(&hTRDFile);
              }
            DskStatus = 0x00;
            break;

        case 0x0C:
            VG.RegStatus = 0x24;
            VG.RegCom = 0x00;
            VG.RegTrack = 0x00;
            VG.RegSect = 0x01;
            VG.RegData = 0x00;
            VG.System = 0x3C;
            VG.StepDirect = 0xFF;
            if (((DskStatus & 0x80)>0) && ((DskStatus & 0x03)>0)) {
               ChipVG93(0xB, DataIO);
               ChipVG93(0xA, DataIO);
            }
            break;

    }
}

bool OpenTRDFile(char* sn, uint8_t  drv){
 DriveNum = (drv &  0x03) + 'A';//DskStatus
 //  DskStatus = DskStatus | (drv & 0x03);
    strcpy(conf.Disks[drv & 0x03],sn);
    //printf("conf.conf.Disks[0]=%s\n",conf.Disks[0]);
   sd_close_file(&hTRDFile);
 //   sd_open_file(&hTRDFile,sn,FA_READ);   
    return true;
}

void InitTRDOS(){
    VG.RegStatus=0;
    VG.RegCom=0;
    VG.RegTrack=0;
    VG.RegSect=1;
    VG.RegData=0;
    VG.System=0;
    VG.StepDirect=0;
    VG.TrackReal[0]=0;
    VG.TrackReal[1]=0;
    VG.TrackReal[2]=0;
    VG.TrackReal[3]=0;
    VG.reserved[0]=0;
    VG.reserved[1]=0;    
    VG.reserved[2]=0;
    DskStatus = 0;
    write_protected = false; // false - защиты записи нет true - защита записи
    VG_PortFF =0x00;
    DriveNum = 'A';
}
////////////////////////////////////////////

void trdos_IndexCounter(void)
{
 
static uint8_t x = 0;
x++;
if (x==6) 
{index_mark  = 0;
 x = 0;
 
 return;
}
index_mark  = 1;
 //index_mark= !index_mark;
}

//===================================================================
bool __not_in_flash_func(diskindex_callback)(repeating_timer_t *rt)
 {     // checks rotation
/* 		if (NoDisk==1) return;
		 DiskSector ++;
		if (DiskSector>15) 
		{
			DiskSector = 0;
			DiskIndex = false; 
		}
		else
		{ 
			DiskIndex = true;
		} */
 }

//===================================================================







#endif
